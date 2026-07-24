extends Node3D

## The boss-rush stage. 3D geometry, gameplay locked to the Z=0 plane.
##
## Instant retry is implemented by resetting state in place rather than
## reloading the scene, which is the only way to hit the sub-second budget the
## rules assert.

signal run_finished(won: bool)

const GROUND_Y := 0.0
const ARENA_HALF_WIDTH := 1400.0

var player: TeacupPlayer
var boss: TeacupBoss
var boss_order := [TeacupRules.Boss.BOTTLECAP, TeacupRules.Boss.GRAMOPHONE,
		TeacupRules.Boss.TEAPOT]
var boss_index := 0
var deaths := 0
var grades: Array[String] = []
var run_complete := false
var _fight_time := 0.0
var _retry_timer := 0.0


func _ready() -> void:
	_build_arena()
	_build_camera()
	player = TeacupPlayer.new()
	add_child(player)
	player.reset_at(Vector3(-700, 120, 0))
	player.died.connect(_on_player_died)
	_spawn_boss()


func _build_arena() -> void:
	_solid(Vector3(0, -40, 0), Vector3(3400, 80, 600), Color(0.35, 0.28, 0.24))
	# Platforms give the fight vertical options.
	_solid(Vector3(-800, 260, 0), Vector3(500, 40, 400), Color(0.42, 0.33, 0.27))
	_solid(Vector3(800, 260, 0), Vector3(500, 40, 400), Color(0.42, 0.33, 0.27))
	_solid(Vector3(0, 470, 0), Vector3(420, 40, 400), Color(0.42, 0.33, 0.27))
	# Invisible side walls keep the player on stage.
	_solid(Vector3(-ARENA_HALF_WIDTH - 120, 400, 0), Vector3(120, 1200, 400),
			Color(0.2, 0.16, 0.15))
	_solid(Vector3(ARENA_HALF_WIDTH + 120, 400, 0), Vector3(120, 1200, 400),
			Color(0.2, 0.16, 0.15))

	# Backdrop: a deep plane so the world reads as 3D rather than a flat cutout.
	_solid(Vector3(0, 600, -400), Vector3(4000, 1600, 60), Color(0.20, 0.17, 0.22))

	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-42, 28, 0)
	sun.light_energy = 1.25
	sun.shadow_enabled = true
	add_child(sun)

	var env := WorldEnvironment.new()
	var e := Environment.new()
	e.background_mode = Environment.BG_COLOR
	# Sepia grade, for the 1930s look.
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


func _build_camera() -> void:
	var cam := Camera3D.new()
	# Near-orthographic on a fixed rail: the 2.5D signature.
	cam.projection = Camera3D.PROJECTION_ORTHOGONAL
	cam.size = 1500.0
	cam.position = Vector3(0, 420, 2200)
	cam.current = true
	add_child(cam)


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


func _spawn_boss() -> void:
	if boss != null and is_instance_valid(boss):
		boss.queue_free()
	boss = TeacupBoss.new()
	boss.boss_id = boss_order[boss_index]
	add_child(boss)
	boss.global_position = Vector3(760, 60, 0)
	boss.defeated.connect(_on_boss_defeated)
	_fight_time = 0.0


func _clear_projectiles() -> void:
	for n in get_tree().get_nodes_in_group("boss_projectiles"):
		n.queue_free()
	for n in get_tree().get_nodes_in_group("player_shots"):
		n.queue_free()


func _physics_process(delta: float) -> void:
	if run_complete:
		return

	if _retry_timer > 0.0:
		_retry_timer -= delta
		if _retry_timer <= 0.0:
			_do_retry()
		return

	_fight_time += delta

	# Keep the player inside the arena.
	if player != null and is_instance_valid(player):
		player.global_position.x = clampf(player.global_position.x,
				-ARENA_HALF_WIDTH, ARENA_HALF_WIDTH)


func _on_player_died() -> void:
	deaths += 1
	# Instant retry: under a second, per the rules the tests pin down.
	_retry_timer = TeacupRules.RETRY_SECONDS


func _do_retry() -> void:
	_clear_projectiles()
	player.reset_at(Vector3(-700, 120, 0))
	_spawn_boss()


func _on_boss_defeated() -> void:
	grades.append(TeacupRules.grade(_fight_time, player.hits_taken, player.parries))
	_clear_projectiles()
	boss_index += 1
	if boss_index >= boss_order.size():
		run_complete = true
		run_finished.emit(true)
		return
	player.reset_at(Vector3(-700, 120, 0))
	_spawn_boss()
