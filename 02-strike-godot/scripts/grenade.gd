class_name StrikeGrenade
extends Node3D

## Thrown utility. Three kinds, each with a real tactical effect rather than a
## cosmetic one:
##
##   HE     — damage in a radius, falling off with distance
##   SMOKE  — a volume that genuinely blocks bot line of sight
##   FLASH  — blinds anyone who could see it and was facing it
##
## Smoke blocking vision is the important one: without it, "utility" is a
## particle effect and the bots play the same game with or without it.

enum Kind { HE, SMOKE, FLASH }

const HE_RADIUS := 500.0
const HE_MAX_DAMAGE := 98.0
const SMOKE_RADIUS := 420.0
const SMOKE_DURATION := 15.0
const FLASH_RADIUS := 1400.0
const FLASH_MAX_SECONDS := 3.2
const FUSE := 1.9

var kind: int = Kind.HE
var thrower: Node = null
var velocity := Vector3.ZERO

var detonated := false
var smoke_time_left := 0.0

var _fuse := FUSE
var _mesh: MeshInstance3D
var _smoke_mesh: MeshInstance3D


func _ready() -> void:
	add_to_group("grenades")

	_mesh = MeshInstance3D.new()
	var sphere := SphereMesh.new()
	sphere.radius = 18.0
	sphere.height = 36.0
	_mesh.mesh = sphere
	var mat := StandardMaterial3D.new()
	match kind:
		Kind.HE: mat.albedo_color = Color(0.35, 0.42, 0.28)
		Kind.SMOKE: mat.albedo_color = Color(0.55, 0.58, 0.62)
		Kind.FLASH: mat.albedo_color = Color(0.92, 0.90, 0.72)
	_mesh.material_override = mat
	add_child(_mesh)


func _physics_process(delta: float) -> void:
	if not detonated:
		velocity.y -= 900.0 * delta
		global_position += velocity * delta
		# Crude bounce off the floor; good enough for a thrown object.
		if global_position.y < 30.0:
			global_position.y = 30.0
			velocity.y = -velocity.y * 0.35
			velocity.x *= 0.7
			velocity.z *= 0.7

		_fuse -= delta
		if _fuse <= 0.0:
			_detonate()
		return

	if kind == Kind.SMOKE:
		smoke_time_left -= delta
		if smoke_time_left <= 0.0:
			queue_free()
	# HE and flash are instantaneous; they free themselves in _detonate.


func _detonate() -> void:
	detonated = true
	match kind:
		Kind.HE: _detonate_he()
		Kind.SMOKE: _detonate_smoke()
		Kind.FLASH: _detonate_flash()


func _players() -> Array:
	return get_tree().get_nodes_in_group("players")


func _detonate_he() -> void:
	for node in _players():
		var p := node as StrikePlayer
		if p == null or not p.is_alive:
			continue
		var distance := global_position.distance_to(p.global_position + Vector3(0, 60, 0))
		if distance > HE_RADIUS:
			continue
		# Linear falloff to the edge of the radius.
		var damage: float = HE_MAX_DAMAGE * (1.0 - distance / HE_RADIUS)
		if damage > 0.0:
			p.take_damage(damage, thrower)
	queue_free()


func _detonate_smoke() -> void:
	smoke_time_left = SMOKE_DURATION
	if _mesh:
		_mesh.visible = false

	_smoke_mesh = MeshInstance3D.new()
	var sphere := SphereMesh.new()
	sphere.radius = SMOKE_RADIUS
	sphere.height = SMOKE_RADIUS * 2.0
	_smoke_mesh.mesh = sphere
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.78, 0.79, 0.81, 0.93)
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	_smoke_mesh.material_override = mat
	add_child(_smoke_mesh)


func _detonate_flash() -> void:
	for node in _players():
		var p := node as StrikePlayer
		if p == null or not p.is_alive:
			continue
		var to_flash := global_position - (p.global_position + Vector3(0, 140, 0))
		var distance := to_flash.length()
		if distance > FLASH_RADIUS:
			continue

		# Facing matters: turning away is the counterplay, so it must work.
		var forward := -p.global_transform.basis.z
		var angle := rad_to_deg(forward.normalized().angle_to(to_flash.normalized()))
		if angle > 100.0:
			continue

		# Occlusion: a flash behind a wall does nothing.
		var space := get_world_3d().direct_space_state
		var query := PhysicsRayQueryParameters3D.create(
				global_position, p.global_position + Vector3(0, 140, 0))
		query.exclude = [p.get_rid()]
		if not space.intersect_ray(query).is_empty():
			continue

		var by_distance: float = 1.0 - (distance / FLASH_RADIUS)
		var by_angle: float = 1.0 - (angle / 100.0)
		p.apply_flash(FLASH_MAX_SECONDS * by_distance * by_angle)
	queue_free()


## Does this grenade block a line between two points? Only active smoke does.
func blocks_line(from: Vector3, to: Vector3) -> bool:
	if kind != Kind.SMOKE or not detonated or smoke_time_left <= 0.0:
		return false
	# Distance from the smoke centre to the segment.
	var seg := to - from
	var len_sq := seg.length_squared()
	if len_sq < 0.001:
		return from.distance_to(global_position) < SMOKE_RADIUS
	var t: float = clampf((global_position - from).dot(seg) / len_sq, 0.0, 1.0)
	var closest := from + seg * t
	return closest.distance_to(global_position) < SMOKE_RADIUS
