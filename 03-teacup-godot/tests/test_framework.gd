class_name TestFramework
extends RefCounted

## Minimal assertion harness for headless runs.
##
## Deliberately tiny rather than pulling in a plugin: this project generates all
## its own content and the same rule applies to tooling. Reports every failure
## with actual-vs-expected, and treats "no tests ran" as a failure — a green run
## that asserted nothing is worse than a red one.

var _passed := 0
var _failures: Array[String] = []
var _suite := ""


func suite(name: String) -> void:
	_suite = name


func _fail(msg: String) -> void:
	_failures.append("%s: %s" % [_suite, msg])


func ok(condition: bool, what: String) -> void:
	if condition:
		_passed += 1
	else:
		_fail("%s (expected true)" % what)


func not_ok(condition: bool, what: String) -> void:
	ok(not condition, what)


func eq(actual, expected, what: String) -> void:
	if actual == expected:
		_passed += 1
	else:
		_fail("%s -- expected %s, got %s" % [what, expected, actual])


func near(actual: float, expected: float, what: String, tolerance := 0.01) -> void:
	if absf(actual - expected) <= tolerance:
		_passed += 1
	else:
		_fail("%s -- expected %.4f +/- %.4f, got %.4f" % [what, expected, tolerance, actual])


func less(actual: float, bound: float, what: String) -> void:
	if actual < bound:
		_passed += 1
	else:
		_fail("%s -- expected < %.4f, got %.4f" % [what, bound, actual])


func greater(actual: float, bound: float, what: String) -> void:
	if actual > bound:
		_passed += 1
	else:
		_fail("%s -- expected > %.4f, got %.4f" % [what, bound, actual])


func passed_count() -> int:
	return _passed


func failures() -> Array[String]:
	return _failures


## Minimum assertions a run must make. A suite whose calls all fail with
## "nonexistent function" produces a green report with almost nothing asserted
## -- which is exactly how a broken script slips through. This is the backstop.
var minimum_assertions := 0


func expect_at_least(n: int) -> void:
	minimum_assertions = n


## Print the report and return the process exit code.
func report() -> int:
	for f in _failures:
		print("FAIL  %s" % f)

	var total := _passed + _failures.size()
	print("")
	print("SUMMARY assertions=%d passed=%d failed=%d" % [total, _passed, _failures.size()])

	if total == 0:
		# Guard against a runner that silently matched nothing.
		print("FAIL  no assertions ran at all")
		return 1

	if total < minimum_assertions:
		print("FAIL  only %d assertions ran, expected at least %d -- a suite is"
				% [total, minimum_assertions])
		print("      probably erroring out rather than asserting")
		return 1
	return 1 if _failures.size() > 0 else 0
