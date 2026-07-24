class_name StrikePlayer
extends CharacterBody3D

## Player controller. Movement decisions come from StrikeMovement so the feel is
## the same thing the tests assert; this class only handles input, the camera,
## and pulling the trigger.

signal died(victim: Node, killer: Node)

const MOUSE_SENSITIVITY := 0.0022
const EYE_HEIGHT := 150.0
const CROUCH_EYE_HEIGHT := 95.0

@export var is_bot := false
@export var team: int = StrikeMatch.Team.T
@export var display_name := "Player"

var health := 100.0
var armor := 0.0
var has_helmet := false
var has_kit := false
var money := StrikeEconomy.START_MONEY
var weapon_id: int = StrikeWeapons.Id.USP
var ammo := 0
var shots_fired := 0          ## Index into the recoil pattern.
var is_alive := true
var carrying_bomb := false

## Cleared during freeze time by the match. Without it, bots open fire before
## the round is live and the whole match resolves during the buy phase.
var can_act := true

var _camera: Camera3D
var _collider: CollisionShape3D
var _cooldown := 0.0
var _reloading := 0.0
var _recoil_recovery := 0.0
var _is_crouching := false
var _is_walking := false

## Bots drive movement through the same wish-direction input a player uses,
## rather than writing velocity directly. Writing velocity does not work: the
## shared pipeline runs friction with a zero wish direction and decays it to
## nothing every tick. Routing bots through the input keeps their movement
## identical to the player's, which was the point of sharing the class.
var bot_wish_dir := Vector2.ZERO
var bot_wish_speed := StrikeMovement.WALK_SPEED
var _rng := RandomNumberGenerator.new()


func _ready() -> void:
	_rng.randomize()
	_build_body()
	equip(weapon_id)
	if not is_bot:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func _build_body() -> void:
	_collider = CollisionShape3D.new()
	var capsule := CapsuleShape3D.new()
	capsule.radius = 40.0
	capsule.height = 180.0
	_collider.shape = capsule
	_collider.position = Vector3(0, 90, 0)
	add_child(_collider)

	# A visible body so bots can be seen and shot.
	var mesh := MeshInstance3D.new()
	var capsule_mesh := CapsuleMesh.new()
	capsule_mesh.radius = 40.0
	capsule_mesh.height = 180.0
	mesh.mesh = capsule_mesh
	mesh.position = Vector3(0, 90, 0)
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.85, 0.55, 0.25) if team == StrikeMatch.Team.T \
			else Color(0.35, 0.55, 0.9)
	mesh.material_override = mat
	add_child(mesh)

	if not is_bot:
		_camera = Camera3D.new()
		_camera.position = Vector3(0, EYE_HEIGHT, 0)
		_camera.fov = 90.0
		_camera.current = true
		add_child(_camera)


func equip(id: int) -> void:
	weapon_id = id
	ammo = StrikeWeapons.spec(id).magazine
	shots_fired = 0
	_reloading = 0.0


func _unhandled_input(event: InputEvent) -> void:
	if is_bot or not is_alive:
		return
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		rotate_y(-event.relative.x * MOUSE_SENSITIVITY)
		if _camera:
			_camera.rotate_x(-event.relative.y * MOUSE_SENSITIVITY)
			_camera.rotation.x = clampf(_camera.rotation.x, -1.5, 1.5)


func _physics_process(delta: float) -> void:
	if not is_alive:
		return

	_cooldown = maxf(_cooldown - delta, 0.0)
	if _reloading > 0.0:
		_reloading -= delta
		if _reloading <= 0.0:
			ammo = StrikeWeapons.spec(weapon_id).magazine
			shots_fired = 0

	# Recoil recovers when not firing, so the pattern resets between bursts.
	_recoil_recovery = maxf(_recoil_recovery - delta, 0.0)
	if _recoil_recovery <= 0.0 and shots_fired > 0 and _reloading <= 0.0:
		shots_fired = 0

	if not is_bot:
		_handle_input(delta)

	_apply_movement(delta)


func _handle_input(delta: float) -> void:
	_is_crouching = Input.is_key_pressed(KEY_CTRL)
	_is_walking = Input.is_key_pressed(KEY_SHIFT)

	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
		try_fire()
	if Input.is_key_pressed(KEY_R):
		start_reload()

	if _camera:
		var target := CROUCH_EYE_HEIGHT if _is_crouching else EYE_HEIGHT
		_camera.position.y = lerpf(_camera.position.y, target, 12.0 * delta)


## Direction the player wants to move, in world space, flattened.
func wish_direction() -> Vector2:
	if is_bot:
		return bot_wish_dir
	# WASD read directly rather than through the input map, so the project needs
	# no input action assets and the bindings stay visible in this file.
	var x := 0.0
	var z := 0.0
	if Input.is_key_pressed(KEY_A): x -= 1.0
	if Input.is_key_pressed(KEY_D): x += 1.0
	if Input.is_key_pressed(KEY_W): z -= 1.0
	if Input.is_key_pressed(KEY_S): z += 1.0
	if x == 0.0 and z == 0.0:
		return Vector2.ZERO
	var local := Vector3(x, 0, z).normalized()
	var world := (transform.basis * local)
	return Vector2(world.x, world.z).normalized()


func _apply_movement(delta: float) -> void:
	var horizontal := Vector2(velocity.x, velocity.z)
	var wish := wish_direction()
	var wish_speed := bot_wish_speed if is_bot \
			else StrikeMovement.wish_speed_for(_is_crouching, _is_walking)

	if is_on_floor():
		horizontal = StrikeMovement.ground_move(horizontal, wish, wish_speed, delta)
		if not is_bot and Input.is_key_pressed(KEY_SPACE):
			velocity.y = StrikeMovement.JUMP_VELOCITY
		else:
			velocity.y = 0.0
	else:
		horizontal = StrikeMovement.air_move(horizontal, wish, wish_speed, delta)
		velocity.y -= StrikeMovement.GRAVITY * delta

	velocity.x = horizontal.x
	velocity.z = horizontal.y
	move_and_slide()


func current_speed() -> float:
	return Vector2(velocity.x, velocity.z).length()


func start_reload() -> void:
	if _reloading > 0.0:
		return
	var spec := StrikeWeapons.spec(weapon_id)
	if ammo >= spec.magazine:
		return
	_reloading = spec.reload_time


func is_reloading() -> bool:
	return _reloading > 0.0


## Fire one shot. Returns true if a round actually left the barrel.
func try_fire() -> bool:
	if not is_alive or not can_act or _cooldown > 0.0 or _reloading > 0.0:
		return false
	if ammo <= 0:
		start_reload()
		return false

	var spec := StrikeWeapons.spec(weapon_id)
	var penalty := StrikeMovement.accuracy_penalty(current_speed(), is_on_floor(), _is_crouching)
	var spread := StrikeWeapons.spread_for(weapon_id, penalty, shots_fired)
	var recoil := StrikeWeapons.recoil_offset(weapon_id, shots_fired)

	var origin := global_position + Vector3(0, EYE_HEIGHT, 0)
	var forward := -global_transform.basis.z
	if _camera:
		origin = _camera.global_position
		forward = -_camera.global_transform.basis.z

	# Recoil walks the aim point; spread adds a random cone on top.
	var dir := forward
	dir = dir.rotated(Vector3.UP, deg_to_rad(-recoil.x + _rng.randf_range(-spread, spread)))
	dir = dir.rotated(global_transform.basis.x.normalized(),
			deg_to_rad(recoil.y * 0.35 + _rng.randf_range(-spread, spread)))

	_trace_shot(origin, dir.normalized(), spec)

	ammo -= 1
	shots_fired += 1
	_cooldown = StrikeWeapons.seconds_per_shot(weapon_id)
	_recoil_recovery = 0.35
	if ammo <= 0:
		start_reload()
	return true


func _trace_shot(origin: Vector3, dir: Vector3, spec) -> void:
	var space := get_world_3d().direct_space_state
	var query := PhysicsRayQueryParameters3D.create(origin, origin + dir * 40000.0)
	query.exclude = [get_rid()]
	var hit := space.intersect_ray(query)
	if hit.is_empty():
		return

	var target = hit.get("collider")
	if target is StrikePlayer and target.is_alive:
		var distance := origin.distance_to(hit.position)
		# Head hits by height on the capsule.
		var relative_y: float = hit.position.y - target.global_position.y
		var head := relative_y > 155.0
		var damage := StrikeWeapons.compute_damage(weapon_id, distance, head, target.armor)
		target.take_damage(damage, self)


func take_damage(amount: float, attacker: Node) -> void:
	if not is_alive or amount <= 0.0:
		return

	# Armour soaks a share and degrades as it does its job.
	if armor > 0.0:
		armor = maxf(armor - amount * 0.5, 0.0)

	health -= amount
	if health <= 0.0:
		health = 0.0
		is_alive = false
		visible = false
		died.emit(self, attacker)


func reset_for_round(spawn: Vector3, new_team: int) -> void:
	team = new_team
	health = 100.0
	is_alive = true
	visible = true
	shots_fired = 0
	_reloading = 0.0
	_cooldown = 0.0
	velocity = Vector3.ZERO
	global_position = spawn
	carrying_bomb = false
