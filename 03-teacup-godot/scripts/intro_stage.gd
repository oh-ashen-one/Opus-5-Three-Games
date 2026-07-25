class_name TeacupIntroStage
extends Node3D

## The run-and-gun opener: a left-to-right platforming run with turret enemies,
## ending at a goal that hands off to the boss rush.
##
## Spec calls for a platforming stage before the bosses, and it earns its place:
## it teaches shooting, jumping and dashing against stakes low enough to learn on.

signal stage_cleared

const GOAL_X := 4200.0
const START := Vector3(-1500, 200, 0)

var player: TeacupPlayer
var cleared := false
var _camera: Camera3D


func _ready() -> void:
	_build()
	player = TeacupPlayer.new()
	add_child(player)
	player.reset_at(START)
	player.died.connect(_on_player_died)


func _build() -> void:
	# Ground in segments, with gaps that must be jumped or dashed.
	var segments := [
		Vector2(-1800, 900), Vector2(1100, 700), Vector2(2100, 600),
		Vector2(2950, 500), Vector2(3700, 900),
	]
	for seg in segments:
		_solid(Vector3(seg.x + seg.y * 0.5, -40, 0), Vector3(seg.y, 80, 600),
				Color(0.72, 0.60, 0.45))

	# Platforms to climb and shoot from.
	_solid(Vector3(-500, 260, 0), Vector3(360, 40, 400), Color(0.80, 0.68, 0.52))
	_solid(Vector3(500, 430, 0), Vector3(360, 40, 400), Color(0.80, 0.68, 0.52))
	_solid(Vector3(1500, 300, 0), Vector3(320, 40, 400), Color(0.80, 0.68, 0.52))
	_solid(Vector3(2500, 380, 0), Vector3(320, 40, 400), Color(0.80, 0.68, 0.52))
	_solid(Vector3(3300, 300, 0), Vector3(320, 40, 400), Color(0.80, 0.68, 0.52))

	# Turrets: stationary shooters that force movement rather than camping.
	for x in [-200.0, 900.0, 1900.0, 2800.0, 3600.0]:
		var turret := preload("res://scripts/turret.gd").new()
		add_child(turret)
		turret.global_position = Vector3(x, 60, 0)

	# Goal marker.
	var goal := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = Vector3(120, 500, 120)
	goal.mesh = box
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(1.0, 0.82, 0.28)
	mat.emission_enabled = true
	mat.emission = mat.albedo_color
	mat.emission_energy_multiplier = 1.4
	goal.material_override = mat
	goal.position = Vector3(GOAL_X, 250, 0)
	add_child(goal)

	_solid(Vector3(-1900, 400, 0), Vector3(120, 1400, 400), Color(0.45, 0.37, 0.30))
	_solid(Vector3(0, 700, -400), Vector3(8000, 1800, 60), Color(0.56, 0.49, 0.42))

	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-52, 24, 0)
	sun.light_energy = 1.5
	sun.light_color = Color(1.0, 0.94, 0.82)
	sun.shadow_enabled = true
	# Softened: hard black shadows fought the aged-film look.
	sun.directional_shadow_blend_splits = true
	sun.shadow_bias = 0.04
	add_child(sun)

	var env := WorldEnvironment.new()
	var e := Environment.new()
	e.background_mode = Environment.BG_COLOR
	e.background_color = Color(0.16, 0.13, 0.11)
	e.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	e.ambient_light_color = Color(0.55, 0.47, 0.38)
	e.ambient_light_energy = 0.7
	e.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	e.adjustment_enabled = true
	e.adjustment_saturation = 0.55
	e.adjustment_contrast = 1.15
	env.environment = e
	add_child(env)

	_camera = Camera3D.new()
	_camera.projection = Camera3D.PROJECTION_ORTHOGONAL
	_camera.size = 1500.0
	_camera.position = Vector3(START.x, 420, 2200)
	_camera.current = true
	add_child(_camera)


func _solid(centre: Vector3, size: Vector3, colour: Color) -> void:
	var body := StaticBody3D.new()
	body.position = centre
	var mesh := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = size
	mesh.mesh = box
	var mat := StandardMaterial3D.new()
	mat.albedo_color = colour
	mat.roughness = 0.9
	mesh.material_override = mat
	body.add_child(mesh)
	var shape := CollisionShape3D.new()
	var box_shape := BoxShape3D.new()
	box_shape.size = size
	shape.shape = box_shape
	body.add_child(shape)
	add_child(body)


func _physics_process(_delta: float) -> void:
	if cleared or player == null or not is_instance_valid(player):
		return

	# The camera follows on the X rail only; the stage stays on one plane.
	if _camera:
		_camera.position.x = lerpf(_camera.position.x, player.global_position.x, 0.12)

	# Falling off the bottom is a death, not a soft-lock.
	if player.global_position.y < -600.0:
		player.take_hit()
		player.take_hit()
		player.take_hit()

	if player.global_position.x >= GOAL_X:
		cleared = true
		stage_cleared.emit()


func _on_player_died() -> void:
	# Instant retry, same budget as the boss fights.
	await get_tree().create_timer(TeacupRules.RETRY_SECONDS).timeout
	for n in get_tree().get_nodes_in_group("boss_projectiles"):
		n.queue_free()
	if is_instance_valid(player):
		player.reset_at(START)
