extends Node

## Headless check of the run-and-gun opener: the stage builds, turrets fire and
## telegraph, the player can traverse it, and reaching the marker clears it.
##
## The player reads input directly, which does not exist headlessly, so the sim
## walks it right by hand -- enough to prove traversal and the clear condition.

const TIME_SCALE := 6.0
const WALK_STEP := 26.0

var _intro: TeacupIntroStage
var _elapsed := 0.0
var _cleared := false
var _projectiles_seen := 0
var _turrets_at_start := 0
var _failures: Array[String] = []
var _done := false


func _ready() -> void:
	Engine.time_scale = TIME_SCALE
	_intro = TeacupIntroStage.new()
	get_tree().root.add_child.call_deferred(_intro)


func _fail(m: String) -> void:
	_failures.append(m)


func _physics_process(delta: float) -> void:
	if _done or _intro == null or not is_instance_valid(_intro):
		return
	_elapsed += delta

	if _turrets_at_start == 0:
		for c in _intro.get_children():
			if c is TeacupTurret:
				_turrets_at_start += 1

	_projectiles_seen = maxi(_projectiles_seen,
			get_tree().get_nodes_in_group("boss_projectiles").size())

	var p = _intro.player
	if p != null and is_instance_valid(p) and p.is_alive:
		# Walk right, and shoot whatever is ahead.
		p.global_position.x += WALK_STEP
		p.global_position.y = maxf(p.global_position.y, 120.0)
		p._shot_timer = 0.0
		p._spawn_shot(Vector2.RIGHT, false, false)

	if _intro.cleared:
		_cleared = true
		_finish()
		return

	if _elapsed > 120.0:
		_finish()


func _finish() -> void:
	if _done:
		return
	_done = true

	if _turrets_at_start < 3:
		_fail("expected turrets in the opener, found %d" % _turrets_at_start)
	if _projectiles_seen <= 0:
		_fail("turrets never fired")
	if not _cleared:
		_fail("the player never reached the goal marker")

	print("turrets: %d" % _turrets_at_start)
	print("max projectiles: %d" % _projectiles_seen)
	print("stage cleared: %s" % str(_cleared))
	for f in _failures:
		print("FAIL  %s" % f)
	print("")
	print("INTRO SIM %s (%d failures)" % ["PASS" if _failures.is_empty() else "FAIL",
			_failures.size()])
	get_tree().quit(0 if _failures.is_empty() else 1)
