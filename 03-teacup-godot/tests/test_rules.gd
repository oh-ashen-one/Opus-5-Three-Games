extends RefCounted

## Rules tests. The theme running through them: this game is allowed to be very
## hard, but it is never allowed to be unfair.


static func run(t: TestFramework) -> void:
	t.suite("rules")
	_test_fairness(t)
	_test_phases(t)
	_test_meter(t)
	_test_grades(t)


static func _test_fairness(t: TestFramework) -> void:
	# Instant retry. A game this punishing is only tolerable if death costs
	# nothing but pride.
	t.less(TeacupRules.RETRY_SECONDS, 1.0, "retry is under one second")

	# Every attack in every phase must telegraph long enough to be reacted to.
	for boss in [TeacupRules.Boss.BOTTLECAP, TeacupRules.Boss.GRAMOPHONE,
			TeacupRules.Boss.TEAPOT]:
		for phase in 3:
			var tel := TeacupRules.telegraph_time(boss, phase)
			t.ok(tel >= TeacupRules.MIN_TELEGRAPH,
					"%s phase %d telegraphs readably (%.2fs)" % [
						TeacupRules.boss_name(boss), phase, tel])

	# Dash i-frames must exist, and must be shorter than the dash itself --
	# otherwise dashing is a free pass through everything.
	t.greater(TeacupRules.DASH_IFRAMES, 0.0, "dash grants i-frames")
	t.less(TeacupRules.DASH_IFRAMES, TeacupRules.DASH_DURATION,
			"i-frames end before the dash does")
	t.ok(TeacupRules.dash_invulnerable(0.0), "invulnerable at dash start")
	t.ok(TeacupRules.dash_invulnerable(TeacupRules.DASH_IFRAMES - 0.01),
			"invulnerable just before the window closes")
	t.not_ok(TeacupRules.dash_invulnerable(TeacupRules.DASH_IFRAMES + 0.01),
			"vulnerable once the window closes")
	t.not_ok(TeacupRules.dash_invulnerable(-0.1), "not invulnerable before dashing")

	# Dash cooldown must exceed the i-frame window, or you could chain
	# invulnerability forever.
	t.greater(TeacupRules.DASH_COOLDOWN, TeacupRules.DASH_IFRAMES,
			"i-frames cannot be chained back to back")

	# Parry window.
	t.ok(TeacupRules.parry_connects(0.0, 0.0), "a perfectly timed parry connects")
	t.ok(TeacupRules.parry_connects(0.0, TeacupRules.PARRY_WINDOW - 0.01),
			"a late-but-inside parry connects")
	t.not_ok(TeacupRules.parry_connects(0.0, TeacupRules.PARRY_WINDOW + 0.05),
			"a parry that expired does not connect")
	t.not_ok(TeacupRules.parry_connects(0.5, 0.2), "you cannot parry the past")

	# Three hit points, no regen. Non-negotiable for the genre.
	t.eq(TeacupRules.MAX_HP, 3, "three hit points")


static func _test_phases(t: TestFramework) -> void:
	for boss in [TeacupRules.Boss.BOTTLECAP, TeacupRules.Boss.GRAMOPHONE,
			TeacupRules.Boss.TEAPOT]:
		var name := TeacupRules.boss_name(boss)
		t.greater(name.length(), 0.0, "boss has a name")

		var previous_health := 99999.0
		var previous_interval := 99999.0
		var previous_attacks := 0
		for phase in 3:
			var hp := TeacupRules.phase_health(boss, phase)
			var interval := TeacupRules.attack_interval(boss, phase)
			var attacks := TeacupRules.attack_count(boss, phase)

			t.greater(hp, 0.0, "%s phase %d has health" % [name, phase])
			# Later phases are shorter, so the fight accelerates rather than drags.
			t.less(hp, previous_health, "%s phase %d is shorter" % [name, phase])
			# ...and denser.
			t.less(interval, previous_interval, "%s phase %d attacks faster" % [name, phase])
			# Each phase must bring something new, or it is the same phase recoloured.
			t.greater(float(attacks), float(previous_attacks),
					"%s phase %d has a distinct attack set" % [name, phase])

			previous_health = hp
			previous_interval = interval
			previous_attacks = attacks

		# Three phases each, exactly as specified.
		t.eq(TeacupRules.attack_count(boss, 2), 4, "final phase has the most attacks")

	# Phase transitions by remaining health.
	t.eq(TeacupRules.phase_for_progress(1.0), 0, "full health is phase 0")
	t.eq(TeacupRules.phase_for_progress(0.5), 1, "half health is phase 1")
	t.eq(TeacupRules.phase_for_progress(0.1), 2, "nearly dead is phase 2")
	t.eq(TeacupRules.phase_for_progress(0.0), 2, "zero health stays in phase 2")


static func _test_meter(t: TestFramework) -> void:
	# Parrying is the only way to build meter, so it must actually pay.
	var m := 0.0
	m = TeacupRules.meter_after_parry(m)
	t.near(m, TeacupRules.SUPER_PER_PARRY, "a parry builds meter")

	# Four parries fill the bar exactly -- a clean, learnable relationship.
	var full := 0.0
	for i in 4:
		full = TeacupRules.meter_after_parry(full)
	t.near(full, TeacupRules.SUPER_MAX, "four parries fill the super")
	t.ok(TeacupRules.can_super(full), "a full bar can super")

	# The meter never overflows.
	for i in 10:
		full = TeacupRules.meter_after_parry(full)
	t.near(full, TeacupRules.SUPER_MAX, "meter is capped")

	# EX costs a quarter, so one parry funds one EX.
	t.ok(TeacupRules.can_ex(TeacupRules.EX_COST), "one parry's worth funds an EX")
	t.not_ok(TeacupRules.can_ex(TeacupRules.EX_COST - 1.0), "not quite enough is refused")
	t.near(TeacupRules.meter_after_spend(TeacupRules.SUPER_MAX, false),
			TeacupRules.SUPER_MAX - TeacupRules.EX_COST, "EX spends a quarter")
	t.near(TeacupRules.meter_after_spend(TeacupRules.SUPER_MAX, true), 0.0,
			"super spends everything")
	t.near(TeacupRules.meter_after_spend(0.0, true), 0.0, "meter never goes negative")

	# Damage ordering: a super must be worth saving for.
	t.greater(TeacupRules.shot_damage(false, true), TeacupRules.shot_damage(true, false),
			"super out-damages EX")
	t.greater(TeacupRules.shot_damage(true, false), TeacupRules.shot_damage(false, false),
			"EX out-damages a normal shot")


static func _test_grades(t: TestFramework) -> void:
	# A perfect no-hit run with parries is an A.
	t.eq(TeacupRules.grade(45.0, 0, 6), "A", "flawless fast run is an A")

	# Taking hits dominates the grade -- this is a game about not getting hit.
	t.not_ok(TeacupRules.grade(45.0, 2, 6) == "A", "two hits costs the A")

	# A slow but clean run still grades well.
	var slow_clean := TeacupRules.grade(150.0, 0, 4)
	t.ok(slow_clean == "A" or slow_clean == "B", "slow but untouched still grades well")

	# A disaster is an F.
	t.eq(TeacupRules.grade(300.0, 3, 0), "F", "slow and battered is an F")

	# Grades must be monotonic in hits taken: more hits is never a better grade.
	var order := {"A": 5, "B": 4, "C": 3, "D": 2, "E": 1, "F": 0}
	var previous := 99
	for hits in 4:
		var g: int = order[TeacupRules.grade(60.0, hits, 2)]
		t.ok(g <= previous, "taking more hits never improves the grade")
		previous = g
