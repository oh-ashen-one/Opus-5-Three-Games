extends SceneTree

## Headless test entry point.
##
##   Godot --headless --path 02-strike-godot --script res://tests/run_tests.gd
##
## Exits non-zero if anything fails, so it can gate a commit.

const SUITES := [
	preload("res://tests/test_movement.gd"),
	preload("res://tests/test_weapons.gd"),
	preload("res://tests/test_economy.gd"),
	preload("res://tests/test_match.gd"),
]


func _initialize() -> void:
	var t := TestFramework.new()

	for suite in SUITES:
		suite.run(t)

	var code := t.report()
	print("STRIKE TESTS %s" % ("PASS" if code == 0 else "FAIL"))
	quit(code)
