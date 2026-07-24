class_name TeacupTurret
extends Node3D

## A stationary enemy for the intro stage. Fires on an interval, telegraphing
## first, so the opener teaches the same read the bosses demand.

const HEALTH := 26.0
const FIRE_INTERVAL := 1.9
const TELEGRAPH := 0.5

var health := HEALTH
var _timer := 0.0
var _telegraphing := false
var _mesh: MeshInstance3D
var _hurtbox: Area3D


func _ready() -> void:
	_timer = randf_range(0.4, FIRE_INTERVAL)

	_mesh = MeshInstance3D.new()
	var mesh := BoxMesh.new()
	mesh.size = Vector3(90, 120, 90)
	_mesh.mesh = mesh
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.62, 0.34, 0.30)
	_mesh.material_override = mat
	_mesh.position = Vector3(0, 60, 0)
	add_child(_mesh)

	_hurtbox = Area3D.new()
	_hurtbox.add_to_group("boss_hurtbox")   # reuses the player's shot handling
	var shape := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = Vector3(90, 120, 90)
	shape.shape = box
	shape.position = Vector3(0, 60, 0)
	_hurtbox.add_child(shape)
	add_child(_hurtbox)


func apply_damage(amount: float) -> void:
	health -= amount
	if health <= 0.0:
		queue_free()


func _physics_process(delta: float) -> void:
	_timer -= delta

	var mat: StandardMaterial3D = _mesh.material_override
	if _timer <= TELEGRAPH and not _telegraphing:
		_telegraphing = true
	if _telegraphing:
		var progress: float = 1.0 - clampf(_timer / TELEGRAPH, 0.0, 1.0)
		mat.albedo_color = Color(0.62, 0.34, 0.30).lerp(Color(1.0, 0.95, 0.5), progress)

	if _timer > 0.0:
		return

	_timer = FIRE_INTERVAL
	_telegraphing = false
	mat.albedo_color = Color(0.62, 0.34, 0.30)

	var player = get_tree().get_first_node_in_group("player")
	var dir := Vector2.LEFT
	if player != null and is_instance_valid(player):
		var to: Vector3 = player.global_position + Vector3(0, 46, 0) - global_position
		dir = Vector2(to.x, to.y).normalized()

	var proj := preload("res://scripts/projectile.gd").new()
	proj.direction = dir
	# Every third shot is parryable, so meter is earnable in the opener too.
	proj.parryable = randi() % 3 == 0
	proj.speed = 480.0
	get_parent().add_child(proj)
	proj.global_position = global_position + Vector3(0, 70, 0)
