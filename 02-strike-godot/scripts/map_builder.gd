class_name StrikeMapBuilder
extends RefCounted

## Builds the competitive map in code.
##
## Blockout first, art pass later — the layout is what decides whether the game
## is any good, and generating it means the layout is a reviewable diff rather
## than an opaque binary scene.
##
## Layout (viewed from above, T spawn at the bottom):
##
##        [ A SITE ]                    [ B SITE ]
##            |                              |
##        A long ---- MID (open, AWP) ---- B tunnels
##            |            |                 |
##        A short          |             B connector
##            \            |                /
##             \-------- T SPAWN ----------/
##
## Three genuinely different routes to each site: A via long, A via short/CT,
## and A through mid; the mirror for B.

const WALL_HEIGHT := 320.0
const WALL_THICK := 40.0

## Site and spawn positions, in world units (cm).
const T_SPAWN := Vector3(0, 0, 2600)
const CT_SPAWN := Vector3(0, 0, -2600)
const SITE_A := Vector3(-1800, 0, -1200)
const SITE_B := Vector3(1800, 0, -1200)
const MID := Vector3(0, 0, 0)


static func build(parent: Node3D) -> void:
	_floor(parent, Vector3(0, 0, 0), Vector3(6000, 40, 6400), Color(0.28, 0.27, 0.25))

	# Outer boundary.
	_wall(parent, Vector3(0, 0, 3200), Vector3(6000, WALL_HEIGHT, WALL_THICK))
	_wall(parent, Vector3(0, 0, -3200), Vector3(6000, WALL_HEIGHT, WALL_THICK))
	_wall(parent, Vector3(-3000, 0, 0), Vector3(WALL_THICK, WALL_HEIGHT, 6400))
	_wall(parent, Vector3(3000, 0, 0), Vector3(WALL_THICK, WALL_HEIGHT, 6400))

	_build_mid(parent)
	_build_a_side(parent)
	_build_b_side(parent)
	_build_sites(parent)
	_lighting(parent)


static func _build_mid(parent: Node3D) -> void:
	# Mid is a corridor between two walls that stop short at both ends, so it can
	# be entered from T side and exited toward either site. Full-length walls
	# would make the lanes unreachable, which is exactly the bug this replaces.
	_wall(parent, Vector3(-700, 0, 1200), Vector3(WALL_THICK, WALL_HEIGHT, 1600))
	_wall(parent, Vector3(700, 0, 1200), Vector3(WALL_THICK, WALL_HEIGHT, 1600))

	# Lane separators, same z-span so every lane is open at both ends.
	_wall(parent, Vector3(-1400, 0, 1200), Vector3(WALL_THICK, WALL_HEIGHT, 1600))
	_wall(parent, Vector3(1400, 0, 1200), Vector3(WALL_THICK, WALL_HEIGHT, 1600))

	# Cover in mid, offset from the walking line so it breaks sightlines without
	# blocking the route.
	_box(parent, Vector3(-380, 0, 900), Vector3(260, 220, 260))
	_box(parent, Vector3(380, 0, 300), Vector3(260, 220, 260))

	# Full-height blocker on the x=0 line. Without it the two spawns can see
	# each other straight down mid and the teams trade kills during freeze time,
	# before anyone has moved. Routes bend around it on both sides.
	_wall(parent, Vector3(0, 0, 150), Vector3(760, WALL_HEIGHT, 300))


static func _build_a_side(parent: Node3D) -> void:
	# Cover on the approach and on the site, all clear of the route lines.
	_box(parent, Vector3(-2600, 0, 1400), Vector3(300, 200, 300))
	_box(parent, Vector3(-1150, 0, 1500), Vector3(240, 200, 240))
	_box(parent, Vector3(-2500, 0, -300), Vector3(320, 240, 320))


static func _build_b_side(parent: Node3D) -> void:
	# Mirrored but not identical -- a perfectly symmetric map is a boring map.
	_box(parent, Vector3(2600, 0, 1100), Vector3(300, 200, 300))
	_box(parent, Vector3(1150, 0, 1700), Vector3(240, 200, 240))
	_box(parent, Vector3(2400, 0, -500), Vector3(300, 260, 300))
	_box(parent, Vector3(2000, 0, 400), Vector3(380, 180, 620))


static func _build_sites(parent: Node3D) -> void:
	# Site markers: raised plates you plant on. Boxes give the defender angles
	# and the attacker something to plant behind.
	_site_plate(parent, SITE_A, Color(0.75, 0.25, 0.2))
	_box(parent, SITE_A + Vector3(500, 0, 300), Vector3(300, 240, 300))
	_box(parent, SITE_A + Vector3(-400, 0, -300), Vector3(280, 200, 280))

	_site_plate(parent, SITE_B, Color(0.2, 0.45, 0.8))
	_box(parent, SITE_B + Vector3(-500, 0, 200), Vector3(300, 240, 300))
	_box(parent, SITE_B + Vector3(350, 0, -400), Vector3(280, 320, 280))


static func _site_plate(parent: Node3D, pos: Vector3, colour: Color) -> void:
	var mesh := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = Vector3(900, 20, 900)
	mesh.mesh = box
	var mat := StandardMaterial3D.new()
	mat.albedo_color = colour
	mesh.material_override = mat
	mesh.position = pos + Vector3(0, 12, 0)
	parent.add_child(mesh)


static func _floor(parent: Node3D, pos: Vector3, size: Vector3, colour: Color) -> void:
	var body := StaticBody3D.new()
	body.position = pos - Vector3(0, size.y * 0.5, 0)

	var mesh := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = size
	mesh.mesh = box
	var mat := StandardMaterial3D.new()
	mat.albedo_color = colour
	mesh.material_override = mat
	body.add_child(mesh)

	var shape := CollisionShape3D.new()
	var box_shape := BoxShape3D.new()
	box_shape.size = size
	shape.shape = box_shape
	body.add_child(shape)

	parent.add_child(body)


static func _wall(parent: Node3D, pos: Vector3, size: Vector3) -> void:
	_solid(parent, pos + Vector3(0, size.y * 0.5, 0), size, Color(0.42, 0.40, 0.38))


static func _box(parent: Node3D, pos: Vector3, size: Vector3) -> void:
	_solid(parent, pos + Vector3(0, size.y * 0.5, 0), size, Color(0.52, 0.44, 0.32))


static func _solid(parent: Node3D, centre: Vector3, size: Vector3, colour: Color) -> void:
	var body := StaticBody3D.new()
	body.position = centre

	var mesh := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = size
	mesh.mesh = box
	var mat := StandardMaterial3D.new()
	mat.albedo_color = colour
	mesh.material_override = mat
	body.add_child(mesh)

	var shape := CollisionShape3D.new()
	var box_shape := BoxShape3D.new()
	box_shape.size = size
	shape.shape = box_shape
	body.add_child(shape)

	parent.add_child(body)


static func _lighting(parent: Node3D) -> void:
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-52, -38, 0)
	sun.light_energy = 1.1
	sun.shadow_enabled = true
	parent.add_child(sun)

	var env := WorldEnvironment.new()
	var e := Environment.new()
	e.background_mode = Environment.BG_SKY
	var sky := Sky.new()
	sky.sky_material = ProceduralSkyMaterial.new()
	e.sky = sky
	e.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	e.ambient_light_energy = 0.45
	e.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	env.environment = e
	parent.add_child(env)


## Spawn points, spread so a team doesn't stack on one tile.
static func spawn_points(is_ct: bool, count: int) -> Array[Vector3]:
	var base := CT_SPAWN if is_ct else T_SPAWN
	var out: Array[Vector3] = []
	for i in count:
		var offset := Vector3((i - (count - 1) * 0.5) * 180.0, 100.0, 0.0)
		out.append(base + offset)
	return out


## ── Routes ──────────────────────────────────────────────────────────────────
##
## Bots follow waypoint routes rather than a navmesh. The map was authored with
## three distinct approaches per site, so the routes ARE the design: encoding
## them explicitly is more reliable than baking navigation and hoping the agent
## picks an interesting path.

const ROUTE_T_A_LONG := [
	Vector3(-2200, 0, 2600), Vector3(-2200, 0, 400), Vector3(-2000, 0, -600), SITE_A,
]
const ROUTE_T_A_SHORT := [
	Vector3(-1050, 0, 2400), Vector3(-1050, 0, 400), Vector3(-1400, 0, -700), SITE_A,
]
const ROUTE_T_A_MID := [
	Vector3(0, 0, 2400), Vector3(0, 0, 700), Vector3(-600, 0, 100),
	Vector3(-900, 0, -500), SITE_A,
]
const ROUTE_T_B_TUNNELS := [
	Vector3(2200, 0, 2600), Vector3(2200, 0, 400), Vector3(2000, 0, -600), SITE_B,
]
const ROUTE_T_B_SHORT := [
	Vector3(1050, 0, 2400), Vector3(1050, 0, 400), Vector3(1400, 0, -700), SITE_B,
]
const ROUTE_T_B_MID := [
	Vector3(0, 0, 2400), Vector3(0, 0, 700), Vector3(600, 0, 100),
	Vector3(900, 0, -500), SITE_B,
]
const ROUTE_CT_A := [
	Vector3(-700, 0, -2300), Vector3(-1500, 0, -1900),
	Vector3(-2300, 0, -1600),
]
const ROUTE_CT_B := [
	Vector3(700, 0, -2300), Vector3(1500, 0, -1900),
	Vector3(2300, 0, -1600),
]


## A route to a site. `variant` spreads a team across the three approaches, so an
## execute is not five bots single-file down one corridor.
static func route_to(is_ct: bool, site_is_a: bool, variant: int) -> Array:
	if is_ct:
		return (ROUTE_CT_A if site_is_a else ROUTE_CT_B).duplicate()
	var pick: int = variant % 2
	if site_is_a:
		return ROUTE_T_A_LONG.duplicate() if pick == 0 else ROUTE_T_A_MID.duplicate()
	return ROUTE_T_B_TUNNELS.duplicate() if pick == 0 else ROUTE_T_B_MID.duplicate()
