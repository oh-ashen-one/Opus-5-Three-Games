class_name TeacupProjectile
extends Area3D

## A boss projectile. Pink ones are parryable, which is the player's only way
## to build super meter.

var direction := Vector2.LEFT
var speed := 520.0
var parryable := false

const PINK := Color(1.0, 0.42, 0.72)
const PLAIN := Color(0.25, 0.22, 0.28)

var _life := 0.0


func _ready() -> void:
	add_to_group("boss_projectiles")
	monitoring = true

	var shape := CollisionShape3D.new()
	var sphere := SphereShape3D.new()
	sphere.radius = 26.0
	shape.shape = sphere
	add_child(shape)

	var mesh := MeshInstance3D.new()
	var sphere_mesh := SphereMesh.new()
	sphere_mesh.radius = 26.0
	sphere_mesh.height = 52.0
	mesh.mesh = sphere_mesh
	var mat := StandardMaterial3D.new()
	mat.albedo_color = PINK if parryable else PLAIN
	mat.emission_enabled = parryable
	mat.emission = PINK
	mat.emission_energy_multiplier = 2.0
	mesh.material_override = mat
	add_child(mesh)


func _physics_process(delta: float) -> void:
	_life += delta
	global_position += Vector3(direction.x, direction.y, 0.0) * speed * delta
	global_position.z = TeacupPlayer.PLANE_Z

	if _life > 6.0:
		queue_free()
		return

	var player = get_tree().get_first_node_in_group("player")
	if player == null or not player.is_alive:
		return

	if global_position.distance_to(player.global_position + Vector3(0, 46, 0)) > 60.0:
		return

	# Parry beats the projectile; otherwise it hurts.
	if parryable and player.parry_window_open():
		player.register_parry()
		queue_free()
		return

	player.take_hit()
	queue_free()
