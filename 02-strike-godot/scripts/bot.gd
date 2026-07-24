class_name StrikeBot
extends StrikePlayer

## Bot behaviour. Extends the player so a bot's damage, falloff, recoil and
## movement are literally the player's — no secret bot numbers.
##
## Bots hold angles rather than running at you. That single decision is the
## difference between a tactical shooter and a shooting gallery.

enum State { HOLD, ROTATE, PUSH, PLANT, DEFUSE, RETREAT }

@export var difficulty := 0.65      ## 0..1. Scales reaction, aim and discipline.

var state: int = State.HOLD
var target_position := Vector3.ZERO
var _path: Array = []
var _path_index := 0
var _enemy: StrikePlayer = null
var _reaction_timer := 0.0
var _think_timer := 0.0
var _hold_angle := Vector3.ZERO
var _bot_rng := RandomNumberGenerator.new()


func _ready() -> void:
	is_bot = true
	super()
	_bot_rng.randomize()
	_hold_angle = global_position


func set_objective(pos: Vector3, new_state: int) -> void:
	target_position = pos
	state = new_state
	_path = []
	_path_index = 0


## Follow an explicit waypoint route. Straight-line movement walks into walls;
## the map's routes are the whole point of its layout, so bots use them.
func set_route(points: Array, new_state: int) -> void:
	_path = points.duplicate()
	_path_index = 0
	state = new_state
	target_position = _path[-1] if not _path.is_empty() else global_position


## Current waypoint, or the final objective once the route is exhausted.
func _current_waypoint() -> Vector3:
	if _path.is_empty() or _path_index >= _path.size():
		return target_position
	return _path[_path_index]


func _physics_process(delta: float) -> void:
	if not is_alive:
		return

	if not can_act:
		# Frozen: hold position and do not shoot.
		bot_wish_dir = Vector2.ZERO
		super._physics_process(delta)
		return

	_think_timer -= delta
	if _think_timer <= 0.0:
		# Bots re-decide a few times a second, not every frame. Per-frame
		# decisions make them twitch between actions on noise.
		_think_timer = 0.2
		_think()

	_act(delta)
	super._physics_process(delta)


func _think() -> void:
	_enemy = _find_visible_enemy()

	if _enemy != null:
		# Reaction delay scaled by difficulty, so they are beatable by being
		# faster rather than by being luckier.
		if _reaction_timer <= 0.0:
			_reaction_timer = lerpf(0.45, 0.12, difficulty)
	else:
		_reaction_timer = 0.0

	# Health discipline: a hurt bot backs off rather than trading.
	if health < 35.0 and _enemy != null:
		state = State.RETREAT


func _act(delta: float) -> void:
	if _enemy != null and _enemy.is_alive:
		_reaction_timer -= delta
		_face(_enemy.global_position + Vector3(0, 140, 0))
		if _reaction_timer <= 0.0:
			# Stop before shooting: firing on the move is punished by the same
			# accuracy penalty the player suffers, and good bots know it.
			bot_wish_dir = Vector2.ZERO
			try_fire()
		return

	match state:
		State.HOLD:
			# Hold the angle. Do not wander into the open.
			bot_wish_dir = Vector2.ZERO
			_face(_hold_angle + Vector3(0, 0, -400))
		State.ROTATE, State.PUSH, State.PLANT, State.DEFUSE:
			var waypoint := _current_waypoint()
			# Advance along the route as each waypoint is reached.
			if global_position.distance_to(waypoint) < 220.0 and _path_index < _path.size():
				_path_index += 1
				waypoint = _current_waypoint()
			_move_toward(waypoint, delta)
		State.RETREAT:
			var away := global_position - (_enemy.global_position if _enemy else target_position)
			_move_toward(global_position + away.normalized() * 600.0, delta)


func _move_toward(pos: Vector3, _delta: float) -> void:
	var to := pos - global_position
	to.y = 0.0
	if to.length() < 120.0:
		bot_wish_dir = Vector2.ZERO
		return
	_face(pos)
	var dir := to.normalized()
	# Bots move at a controlled pace rather than sprinting into sightlines they
	# cannot react to. Fed as a wish direction, exactly like player input.
	bot_wish_dir = Vector2(dir.x, dir.z).normalized()
	bot_wish_speed = StrikeMovement.WALK_SPEED * lerpf(0.9, 1.6, difficulty)


func _face(pos: Vector3) -> void:
	var to := pos - global_position
	to.y = 0.0
	if to.length_squared() < 1.0:
		return
	# Aim error shrinks with difficulty but never reaches zero — perfect aim is
	# not difficulty, it is just unfair.
	var error := lerpf(0.25, 0.03, difficulty)
	var yaw := atan2(-to.x, -to.z) + _bot_rng.randf_range(-error, error)
	rotation.y = lerp_angle(rotation.y, yaw, 0.35)


func _find_visible_enemy() -> StrikePlayer:
	var best: StrikePlayer = null
	var best_distance := 99999.0
	var space := get_world_3d().direct_space_state
	var eye := global_position + Vector3(0, 140, 0)

	for node in get_tree().get_nodes_in_group("players"):
		var other := node as StrikePlayer
		if other == null or other == self or not other.is_alive:
			continue
		if other.team == team:
			continue

		var distance := eye.distance_to(other.global_position)
		if distance >= best_distance:
			continue

		# Line of sight: walls and boxes genuinely block vision, which is what
		# makes holding an angle and using cover meaningful.
		var query := PhysicsRayQueryParameters3D.create(
				eye, other.global_position + Vector3(0, 140, 0))
		query.exclude = [get_rid(), other.get_rid()]
		var hit := space.intersect_ray(query)
		if not hit.is_empty():
			continue

		best = other
		best_distance = distance

	return best


## Buy for the round, using the shared economy planner.
func do_buy(round_number: int) -> void:
	var plan := StrikeEconomy.plan_buy(money, team == StrikeMatch.Team.CT, round_number)
	if plan.weapon >= 0:
		equip(plan.weapon)
	if plan.armor:
		armor = 100.0
		has_helmet = plan.helmet
	has_kit = plan.kit
	money = StrikeEconomy.clamp_money(money - plan.spend)
