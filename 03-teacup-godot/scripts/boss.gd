class_name TeacupBoss
extends Node3D

## A boss: a hierarchical state machine over three phases, each with its own
## data-driven attack table.
##
## The rule that makes the fight fair: every attack goes TELEGRAPH -> COMMIT ->
## RECOVER, and the telegraph length comes from TeacupRules, which the tests
## pin to a readable minimum.

signal defeated
signal phase_changed(phase: int)

enum State { IDLE, TELEGRAPH, COMMIT, RECOVER, DEAD }

@export var boss_id: int = TeacupRules.Boss.BOTTLECAP

var phase := 0
var health := 0.0
var max_health := 0.0
var state: int = State.IDLE
var is_dead := false

var _timer := 0.0
var _attack := 0
var _rng := RandomNumberGenerator.new()
var _mesh: MeshInstance3D
var _hurtbox: Area3D
var _base_colour := Color(0.85, 0.35, 0.3)


func _ready() -> void:
	_rng.randomize()
	max_health = 0.0
	for p in 3:
		max_health += TeacupRules.phase_health(boss_id, p)
	health = max_health
	_build()
	_enter_idle()


func _build() -> void:
	match boss_id:
		TeacupRules.Boss.BOTTLECAP: _base_colour = Color(0.86, 0.30, 0.26)
		TeacupRules.Boss.GRAMOPHONE: _base_colour = Color(0.55, 0.35, 0.72)
		TeacupRules.Boss.TEAPOT: _base_colour = Color(0.30, 0.55, 0.48)

	_mesh = MeshInstance3D.new()
	var mesh := SphereMesh.new()
	mesh.radius = 150.0
	mesh.height = 300.0
	_mesh.mesh = mesh
	var mat := StandardMaterial3D.new()
	mat.albedo_color = _base_colour
	mat.roughness = 0.6
	_mesh.material_override = mat
	_mesh.position = Vector3(0, 170, 0)
	add_child(_mesh)

	_hurtbox = Area3D.new()
	_hurtbox.add_to_group("boss_hurtbox")
	var shape := CollisionShape3D.new()
	var sphere := SphereShape3D.new()
	sphere.radius = 150.0
	shape.shape = sphere
	shape.position = Vector3(0, 170, 0)
	_hurtbox.add_child(shape)
	add_child(_hurtbox)


func apply_damage(amount: float) -> void:
	if is_dead:
		return
	health = maxf(health - amount, 0.0)

	var new_phase := TeacupRules.phase_for_progress(health / maxf(max_health, 1.0))
	if new_phase != phase:
		phase = new_phase
		phase_changed.emit(phase)
		# Phase transitions must be readable: the boss stops, changes colour,
		# and re-telegraphs rather than continuing mid-attack.
		_enter_state(State.RECOVER, 1.0)
		if _mesh:
			var mat: StandardMaterial3D = _mesh.material_override
			mat.albedo_color = _base_colour.lerp(Color(1, 1, 1), 0.22 * float(phase))

	if health <= 0.0:
		is_dead = true
		state = State.DEAD
		defeated.emit()


func _enter_idle() -> void:
	_enter_state(State.IDLE, TeacupRules.attack_interval(boss_id, phase))


func _enter_state(next: int, duration: float) -> void:
	state = next
	_timer = duration


func _physics_process(delta: float) -> void:
	if is_dead:
		return
	_timer -= delta
	if _timer > 0.0:
		_tick_state(delta)
		return

	match state:
		State.IDLE:
			_attack = _rng.randi_range(0, TeacupRules.attack_count(boss_id, phase) - 1)
			_enter_state(State.TELEGRAPH, TeacupRules.telegraph_time(boss_id, phase))
		State.TELEGRAPH:
			_commit_attack()
			_enter_state(State.COMMIT, 0.45)
		State.COMMIT:
			_enter_state(State.RECOVER, 0.5)
		State.RECOVER:
			_enter_idle()


func _tick_state(_delta: float) -> void:
	if _mesh == null:
		return
	var mat: StandardMaterial3D = _mesh.material_override
	if state == State.TELEGRAPH:
		# Flash toward white as the attack winds up. This IS the tell.
		var progress := 1.0 - clampf(_timer / maxf(TeacupRules.telegraph_time(boss_id, phase), 0.01), 0.0, 1.0)
		mat.albedo_color = _base_colour.lerp(Color(1.0, 0.95, 0.5), progress * 0.85)
	elif state == State.IDLE:
		mat.albedo_color = _base_colour.lerp(Color(1, 1, 1), 0.22 * float(phase))


func _commit_attack() -> void:
	# Each attack spawns a different projectile pattern. Attack 1 of every phase
	# is parryable (pink) so the player always has a way to build meter.
	var parryable := (_attack % 2 == 1)
	match _attack:
		0: _spawn_spread(5, parryable)
		1: _spawn_aimed(parryable)
		2: _spawn_wall(parryable)
		_: _spawn_spiral(parryable)


func _spawn_projectile(dir: Vector2, parryable: bool, speed := 520.0) -> void:
	var proj := preload("res://scripts/projectile.gd").new()
	proj.direction = dir.normalized()
	proj.parryable = parryable
	proj.speed = speed
	get_parent().add_child(proj)
	proj.global_position = global_position + Vector3(0, 170, 0)


func _spawn_spread(count: int, parryable: bool) -> void:
	for i in count:
		var angle := lerpf(-0.7, 0.7, float(i) / maxf(float(count - 1), 1.0))
		_spawn_projectile(Vector2(-cos(angle), sin(angle)), parryable)


func _spawn_aimed(parryable: bool) -> void:
	var target := get_tree().get_first_node_in_group("player")
	var dir := Vector2(-1, 0)
	if target != null:
		var to: Vector3 = target.global_position - global_position
		dir = Vector2(to.x, to.y).normalized()
	_spawn_projectile(dir, parryable, 700.0)


func _spawn_wall(parryable: bool) -> void:
	for i in 4:
		_spawn_projectile(Vector2(-1, 0), parryable and i == 2, 420.0 + i * 60.0)


func _spawn_spiral(parryable: bool) -> void:
	for i in 8:
		var angle := TAU * float(i) / 8.0
		_spawn_projectile(Vector2(cos(angle), sin(angle)), parryable and i == 0, 460.0)
