class_name TeacupShot
extends Area3D

## A player bullet. Travels on the gameplay plane and dies on contact or range.

var direction := Vector2.RIGHT
var damage := TeacupRules.SHOT_DAMAGE
var is_ex := false
var speed := 1600.0

var _travelled := 0.0
const MAX_RANGE := 3000.0


func _ready() -> void:
	add_to_group("player_shots")
	monitoring = true

	var shape := CollisionShape3D.new()
	var sphere := SphereShape3D.new()
	sphere.radius = 22.0 if is_ex else 12.0
	shape.shape = sphere
	add_child(shape)

	var mesh := MeshInstance3D.new()
	var sphere_mesh := SphereMesh.new()
	sphere_mesh.radius = 22.0 if is_ex else 12.0
	sphere_mesh.height = sphere_mesh.radius * 2.0
	mesh.mesh = sphere_mesh
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(1.0, 0.85, 0.35) if is_ex else Color(0.98, 0.96, 0.9)
	mat.emission_enabled = true
	mat.emission = mat.albedo_color
	mat.emission_energy_multiplier = 1.6
	mesh.material_override = mat
	add_child(mesh)


func _physics_process(delta: float) -> void:
	var step := speed * delta
	global_position += Vector3(direction.x, direction.y, 0.0) * step
	global_position.z = TeacupPlayer.PLANE_Z
	_travelled += step
	if _travelled > MAX_RANGE:
		queue_free()
		return

	for body in get_overlapping_areas():
		if body.is_in_group("boss_hurtbox"):
			body.get_parent().apply_damage(damage)
			queue_free()
			return
