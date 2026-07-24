extends Node

## Headless match simulation.
##
##   Godot --headless --path 02-strike-godot --script res://tests/sim_match.gd
##
## The unit tests prove the rules are right. This proves the game actually runs:
## the map builds, 10 actors spawn, bots move and shoot, rounds resolve, scores
## advance, and nothing crashes. Time is accelerated so a full match fits in a
## short run.

const SIM_SECONDS := 900.0       ## Simulated seconds. One round is ~127s,
                                 ## so this covers a full MR6 match.
const TIME_SCALE := 12.0         ## Run the match this much faster than real time.

var _scene: Node = null
var _elapsed := 0.0
var _last_round := 1
var _rounds_seen := 0
var _failures: Array[String] = []
var _saw_movement := false
var _start_positions := {}
var _done := false
var _report_timer := 0.0
var _plants := 0
var _defuses := 0
var _was_planted := false


func _ready() -> void:
	Engine.time_scale = TIME_SCALE
	var packed := load("res://scenes/match.tscn")
	if packed == null:
		_fail("match.tscn failed to load")
		_finish()
		return
	_scene = packed.instantiate()
	get_tree().root.add_child.call_deferred(_scene)


func _fail(msg: String) -> void:
	_failures.append(msg)


func _process(delta: float) -> void:
	if _done:
		return
	if _scene == null or not is_instance_valid(_scene):
		_fail("match scene disappeared mid-run")
		_finish()
		return

	_elapsed += delta

	# One-time structural checks, once the scene has had a frame to build.
	if _elapsed > 0.5 and _start_positions.is_empty():
		_check_structure()

	# Did anybody actually move? A match where every bot stands still would
	# otherwise pass every rule test and be completely broken.
	if not _saw_movement and _elapsed > 2.0:
		for p in _players():
			if _start_positions.has(p) and \
					p.global_position.distance_to(_start_positions[p]) > 60.0:
				_saw_movement = true
				break

	# Count objective events; sampling every 60s would miss most of them.
	if _scene.bomb_planted and not _was_planted:
		_plants += 1
	if _scene.bomb_defused and _was_planted:
		_defuses += 1
	_was_planted = _scene.bomb_planted

	_report_timer -= delta
	if _report_timer <= 0.0:
		_report_timer = 60.0
		var ts := 0
		var cts := 0
		for p in _players():
			if is_instance_valid(p) and p.is_alive:
				if p.team == StrikeMatch.Team.T:
					ts += 1
				else:
					cts += 1
		print("t=%4.0fs round=%d phase=%d alive %dT/%dCT planted=%s" % [
				_elapsed, _scene.round_number, _scene.phase, ts, cts,
				str(_scene.bomb_planted)])

	if _scene.round_number != _last_round:
		_rounds_seen += 1
		_last_round = _scene.round_number

	if _scene.match_over:
		print("match ended: T %d - CT %d" % [_scene.t_score, _scene.ct_score])
		_finish()
		return

	if _elapsed >= SIM_SECONDS:
		_finish()


func _players() -> Array:
	if _scene == null:
		return []
	var out: Array = [_scene.player]
	out.append_array(_scene.bots)
	return out


func _check_structure() -> void:
	var players := _players()
	if players.size() != 10:
		_fail("expected 10 actors, found %d" % players.size())

	var ts := 0
	var cts := 0
	for p in players:
		if not is_instance_valid(p):
			_fail("an actor was invalid at startup")
			continue
		_start_positions[p] = p.global_position
		if p.team == StrikeMatch.Team.T:
			ts += 1
		else:
			cts += 1

	if ts != 5 or cts != 5:
		_fail("expected 5v5, got %dT %dCT" % [ts, cts])

	# The map must have real geometry, or bots have nothing to path around.
	var bodies := 0
	for child in _scene.get_children():
		if child is StaticBody3D:
			bodies += 1
	if bodies < 15:
		_fail("map has only %d solid bodies -- geometry missing" % bodies)
	else:
		print("map solids: %d" % bodies)

	# Somebody must be carrying the bomb, or the round cannot be won by plant.
	var carriers := 0
	for p in players:
		if is_instance_valid(p) and p.carrying_bomb:
			carriers += 1
	if carriers != 1:
		_fail("expected exactly 1 bomb carrier, found %d" % carriers)


func _finish() -> void:
	if _done:
		return
	_done = true
	if not _saw_movement:
		_fail("nobody moved during the whole simulation")
	if _rounds_seen < 1 and (_scene != null and not _scene.match_over):
		_fail("no round ever resolved in %.0fs of simulation" % SIM_SECONDS)

	print("")
	print("rounds resolved: %d" % _rounds_seen)
	print("bomb plants: %d   defuses: %d" % [_plants, _defuses])
	print("score: T %d - CT %d" % [
			_scene.t_score if _scene != null else -1,
			_scene.ct_score if _scene != null else -1])
	print("movement observed: %s" % ("yes" if _saw_movement else "NO"))

	for f in _failures:
		print("FAIL  %s" % f)

	print("")
	print("SIM %s (%d failures)" % ["PASS" if _failures.is_empty() else "FAIL",
			_failures.size()])
	get_tree().quit(0 if _failures.is_empty() else 1)
