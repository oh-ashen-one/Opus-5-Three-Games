class_name TeacupPlayer
extends CharacterBody3D

## The player. Fully 3D and lit, but locked to the Z=0 plane so it plays as 2D.
## That constraint is the whole "2.5D" of the project: real depth and shadows,
## flat gameplay.

signal hit_taken(remaining: int)
signal died
signal parried

const PLANE_Z := 0.0

var hp := TeacupRules.MAX_HP
var meter := 0.0
var facing := 1.0                ## +1 right, -1 left.
var is_alive := true
var parries := 0
var hits_taken := 0

var _aim := Vector2.RIGHT
var _shot_timer := 0.0
var _dash_time := -99.0          ## Seconds since the dash started.
var _dash_cooldown := 0.0
var _dash_dir := Vector2.RIGHT
var _parry_time := -99.0
var _lock_aim := false
var _invuln_timer := 0.0         ## Post-hit mercy invulnerability.
var _mesh: MeshInstance3D


func _ready() -> void:
	add_to_group("player")
	_build_body()


func _build_body() -> void:
	var shape := CollisionShape3D.new()
	var capsule := CapsuleShape3D.new()
	capsule.radius = 26.0
	capsule.height = 92.0
	shape.shape = capsule
	shape.position = Vector3(0, 46, 0)
	add_child(shape)

	_mesh = MeshInstance3D.new()
	var capsule_mesh := CapsuleMesh.new()
	capsule_mesh.radius = 26.0
	capsule_mesh.height = 92.0
	_mesh.mesh = capsule_mesh
	_mesh.position = Vector3(0, 46, 0)
	var mat := StandardMaterial3D.new()
	# Rubber-hose palette: warm cream body with a hard ink-ish rim.
	mat.albedo_color = Color(0.96, 0.90, 0.78)
	mat.roughness = 0.75
	_mesh.material_override = mat
	add_child(_mesh)


func is_invulnerable() -> bool:
	return TeacupRules.dash_invulnerable(_dash_time) or _invuln_timer > 0.0


func _physics_process(delta: float) -> void:
	if not is_alive:
		return

	_dash_time += delta
	_dash_cooldown = maxf(_dash_cooldown - delta, 0.0)
	_shot_timer = maxf(_shot_timer - delta, 0.0)
	_invuln_timer = maxf(_invuln_timer - delta, 0.0)
	_parry_time += delta

	_read_input(delta)
	_move(delta)

	# Hard-lock to the gameplay plane. Any drift in Z and the game stops being 2D.
	global_position.z = PLANE_Z
	velocity.z = 0.0

	# Flash while invulnerable so the state is readable.
	if _mesh:
		_mesh.visible = not (_invuln_timer > 0.0 and fmod(_invuln_timer, 0.16) < 0.08)


func _read_input(_delta: float) -> void:
	var x := 0.0
	var y := 0.0
	if Input.is_key_pressed(KEY_A) or Input.is_key_pressed(KEY_LEFT): x -= 1.0
	if Input.is_key_pressed(KEY_D) or Input.is_key_pressed(KEY_RIGHT): x += 1.0
	if Input.is_key_pressed(KEY_W) or Input.is_key_pressed(KEY_UP): y += 1.0
	if Input.is_key_pressed(KEY_S) or Input.is_key_pressed(KEY_DOWN): y -= 1.0

	# 8-directional aim, independent of movement when lock-aim is held.
	_lock_aim = Input.is_key_pressed(KEY_SHIFT)
	if x != 0.0 or y != 0.0:
		_aim = Vector2(x, y).normalized()
		if x != 0.0 and not _lock_aim:
			facing = signf(x)

	if Input.is_key_pressed(KEY_X) or Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
		_try_shoot()

	if Input.is_key_pressed(KEY_C):
		_try_parry()

	if Input.is_key_pressed(KEY_SHIFT) and Input.is_key_pressed(KEY_Z):
		pass  # reserved

	if Input.is_action_just_pressed("ui_accept") or Input.is_key_pressed(KEY_SPACE):
		if is_on_floor():
			velocity.y = TeacupRules.JUMP_VELOCITY

	if Input.is_key_pressed(KEY_Z):
		_try_dash()


func _move(delta: float) -> void:
	var moving := 0.0
	if Input.is_key_pressed(KEY_A) or Input.is_key_pressed(KEY_LEFT): moving -= 1.0
	if Input.is_key_pressed(KEY_D) or Input.is_key_pressed(KEY_RIGHT): moving += 1.0

	# The >= 0.0 half matters: _dash_time starts at a large negative "long ago"
	# sentinel, and a bare "< DASH_DURATION" is true for negative values, so the
	# player spawned permanently dashing and flew across the arena.
	if _dash_time >= 0.0 and _dash_time < TeacupRules.DASH_DURATION:
		# Dashing overrides normal movement entirely.
		velocity.x = _dash_dir.x * TeacupRules.DASH_SPEED
		velocity.y = 0.0
	else:
		velocity.x = moving * TeacupRules.MOVE_SPEED
		if _lock_aim:
			velocity.x = 0.0
		velocity.y -= TeacupRules.GRAVITY * delta

	move_and_slide()


func _try_dash() -> void:
	if _dash_cooldown > 0.0 or (_dash_time >= 0.0 and _dash_time < TeacupRules.DASH_DURATION):
		return
	_dash_time = 0.0
	_dash_cooldown = TeacupRules.DASH_COOLDOWN
	_dash_dir = Vector2(facing, 0.0)


func _try_parry() -> void:
	# A parry is an attempt with a window; the pink object decides if it lands.
	if _parry_time >= 0.0 and _parry_time < TeacupRules.PARRY_WINDOW:
		return
	_parry_time = 0.0


func parry_window_open() -> bool:
	return _parry_time >= 0.0 and _parry_time <= TeacupRules.PARRY_WINDOW


## Called by a pink object that the player successfully parried.
func register_parry() -> void:
	parries += 1
	meter = TeacupRules.meter_after_parry(meter)
	# A parry always gives a small hop, so it doubles as a movement tool.
	velocity.y = TeacupRules.JUMP_VELOCITY * 0.8
	parried.emit()


func _try_shoot() -> void:
	if _shot_timer > 0.0:
		return
	_shot_timer = TeacupRules.SHOT_INTERVAL

	var is_super := TeacupRules.can_super(meter) and Input.is_key_pressed(KEY_V)
	var is_ex := not is_super and TeacupRules.can_ex(meter) and Input.is_key_pressed(KEY_B)
	if is_super or is_ex:
		meter = TeacupRules.meter_after_spend(meter, is_super)

	var dir := _aim if (_aim.x != 0.0 or _aim.y != 0.0) else Vector2(facing, 0.0)
	_spawn_shot(dir, is_ex, is_super)


func _spawn_shot(dir: Vector2, is_ex: bool, is_super: bool) -> void:
	var shot := preload("res://scripts/shot.gd").new()
	shot.direction = dir.normalized()
	shot.damage = TeacupRules.shot_damage(is_ex, is_super)
	shot.is_ex = is_ex or is_super
	get_parent().add_child(shot)
	shot.global_position = global_position + Vector3(0, 60, 0)


func take_hit() -> void:
	if not is_alive or is_invulnerable():
		return
	hp -= 1
	hits_taken += 1
	_invuln_timer = 1.1
	hit_taken.emit(hp)
	if hp <= 0:
		is_alive = false
		died.emit()


func reset_at(pos: Vector3) -> void:
	hp = TeacupRules.MAX_HP
	meter = 0.0
	is_alive = true
	parries = 0
	hits_taken = 0
	velocity = Vector3.ZERO
	global_position = Vector3(pos.x, pos.y, PLANE_Z)
	_invuln_timer = 0.0
	_dash_time = -99.0
	if _mesh:
		_mesh.visible = true
