extends Node

## Headless boss-rush simulation.
##
## Proves the game runs: the stage builds, the boss cycles its phases, the
## player can damage it, phases advance, and a boss can actually be defeated.
## Drives the player synthetically since there is no input in a headless run.

const SIM_SECONDS := 240.0
const TIME_SCALE := 6.0

var _stage: Node = null
var _elapsed := 0.0
var _failures: Array[String] = []
var _phases_seen := {}
var _projectiles_seen := 0
var _damage_dealt := 0.0
var _last_health := -1.0
var _done := false
var _report := 0.0


func _ready() -> void:
	Engine.time_scale = TIME_SCALE
	_stage = preload("res://scripts/stage.gd").new()
	get_tree().root.add_child.call_deferred(_stage)


func _fail(m: String) -> void:
	_failures.append(m)


func _physics_process(delta: float) -> void:
	if _done or _stage == null or not is_instance_valid(_stage):
		return
	_elapsed += delta

	var boss = _stage.boss
	var player = _stage.player
	if boss == null or not is_instance_valid(boss):
		return
	if player == null or not is_instance_valid(player):
		return

	_phases_seen[boss.phase] = true
	_projectiles_seen = maxi(_projectiles_seen,
			get_tree().get_nodes_in_group("boss_projectiles").size())

	if _last_health >= 0.0 and boss.health < _last_health:
		_damage_dealt += _last_health - boss.health
	_last_health = boss.health

	# Synthetic player: shoot the boss continuously. Enough to prove damage and
	# phase transitions work end to end.
	if player.is_alive:
		player._shot_timer = 0.0
		var to: Vector3 = (boss.global_position + Vector3(0, 170, 0)) - player.global_position
		player._spawn_shot(Vector2(to.x, to.y).normalized(), false, false)

	_report -= delta
	if _report <= 0.0:
		_report = 30.0
		print("t=%4.0fs hp=%.0f/%.0f player=(%.0f,%.0f,%.0f) alive=%s shots=%d proj=%d" % [
				_elapsed, boss.health, boss.max_health,
				player.global_position.x, player.global_position.y, player.global_position.z,
				str(player.is_alive),
				get_tree().get_nodes_in_group("player_shots").size(),
				get_tree().get_nodes_in_group("boss_projectiles").size()])
		print("       boss at (%.0f,%.0f,%.0f) state=%d" % [
				boss.global_position.x, boss.global_position.y,
				boss.global_position.z, boss.state])

	if _stage.run_complete or _elapsed >= SIM_SECONDS:
		_finish()


func _finish() -> void:
	if _done:
		return
	_done = true

	if _damage_dealt <= 0.0:
		_fail("the boss never took any damage")
	if _phases_seen.size() < 2:
		_fail("boss never left its first phase (saw %d phases)" % _phases_seen.size())
	if _projectiles_seen <= 0:
		_fail("the boss never fired a projectile")
	if _stage.boss_index < 1 and not _stage.run_complete:
		_fail("no boss was ever defeated")

	print("")
	print("bosses defeated: %d" % _stage.boss_index)
	print("phases observed: %d" % _phases_seen.size())
	print("damage dealt: %.0f" % _damage_dealt)
	print("max projectiles on screen: %d" % _projectiles_seen)
	print("deaths: %d   grades: %s" % [_stage.deaths, str(_stage.grades)])

	for f in _failures:
		print("FAIL  %s" % f)
	print("")
	print("SIM %s (%d failures)" % ["PASS" if _failures.is_empty() else "FAIL",
			_failures.size()])
	get_tree().quit(0 if _failures.is_empty() else 1)
