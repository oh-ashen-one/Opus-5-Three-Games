extends SceneTree

## Headless test entry point.
##   Godot --headless --path 03-teacup-godot --script res://tests/run_tests.gd

const SUITES := [
	preload("res://tests/test_rules.gd"),
]


func _initialize() -> void:
	var t := TestFramework.new()
	t.expect_at_least(60)
	for suite in SUITES:
		suite.run(t)
	var code := t.report()
	print("TEACUP TESTS %s" % ("PASS" if code == 0 else "FAIL"))
	quit(code)
