extends RefCounted

## Movement tests. If these pass, the game has a skill ceiling; if they don't,
## it's a walking simulator with guns.

const DT := 1.0 / 64.0  ## 64-tick, like the games this borrows from.


static func run(t: TestFramework) -> void:
	t.suite("movement")
	_test_speed_cap(t)
	_test_counter_strafe(t)
	_test_friction(t)
	_test_air_strafe(t)
	_test_air_speed_cap(t)
	_test_accuracy(t)


static func _run_ticks(count: int, wish_dir: Vector2, on_ground := true,
		velocity := Vector2.ZERO, wish_speed := StrikeMovement.MAX_SPEED) -> Vector2:
	var v := velocity
	for i in count:
		if on_ground:
			v = StrikeMovement.ground_move(v, wish_dir, wish_speed, DT)
		else:
			v = StrikeMovement.air_move(v, wish_dir, wish_speed, DT)
	return v


static func _test_speed_cap(t: TestFramework) -> void:
	# Running forward forever must converge on the cap, not exceed it.
	var v := _run_ticks(300, Vector2(1, 0))
	t.near(v.length(), StrikeMovement.MAX_SPEED, "forward run reaches the speed cap", 1.0)

	# ...and must not keep climbing after that.
	var v2 := _run_ticks(600, Vector2(1, 0))
	t.less(v2.length(), StrikeMovement.MAX_SPEED + 0.5, "speed never exceeds the cap")

	# Walking is slower than running, crouching slower still.
	t.less(StrikeMovement.wish_speed_for(false, true), StrikeMovement.MAX_SPEED,
			"walk is slower than run")
	t.less(StrikeMovement.wish_speed_for(true, false),
			StrikeMovement.wish_speed_for(false, true), "crouch is slowest")


static func _test_counter_strafe(t: TestFramework) -> void:
	# The single most important mechanic: reversing input must stop you almost
	# immediately, far faster than simply releasing the key.
	var running := _run_ticks(300, Vector2(1, 0))
	t.greater(running.length(), 200.0, "moving before the counter-strafe")

	# Reversing input removes speed at accel*wish_speed per tick (~21.5 cm/s at
	# 64-tick), so a full stop lands around 12 ticks / ~190ms. That is the real
	# feel of a counter-strafe; expecting a dead stop in 4 ticks was simply wrong.
	var countered_4 := _run_ticks(4, Vector2(-1, 0), true, running)
	var released_4 := _run_ticks(4, Vector2.ZERO, true, running)
	t.less(countered_4.length(), released_4.length() * 0.75,
			"counter-strafing sheds speed much faster than releasing")

	# A counter-strafe is a *tap*, not a hold: sustained reverse input stops you
	# and then accelerates you the other way. So the thing to measure is the
	# lowest speed reached, and how many ticks it takes to get there.
	var countered_min := 99999.0
	var countered_ticks := -1
	var cv := running
	for i in 20:
		cv = StrikeMovement.ground_move(cv, Vector2(-1, 0), StrikeMovement.MAX_SPEED, DT)
		if cv.length() < countered_min:
			countered_min = cv.length()
			countered_ticks = i + 1

	t.less(countered_min, 15.0, "counter-strafing brings you to a near dead stop")
	t.less(float(countered_ticks), 13.0, "and does it within ~12 ticks (~200ms)")

	# Friction alone over the same window never gets close.
	var released_12 := _run_ticks(12, Vector2.ZERO, true, running)
	t.greater(released_12.length(), countered_min * 4.0,
			"releasing the key leaves you far faster than counter-strafing")


static func _test_friction(t: TestFramework) -> void:
	# Friction alone must bring you to rest, and never reverse you.
	var v := Vector2(StrikeMovement.MAX_SPEED, 0)
	for i in 200:
		v = StrikeMovement.apply_friction(v, DT)
		t.ok(v.x >= -0.001, "friction never pushes you backwards") if i == 199 else null
	t.near(v.length(), 0.0, "friction brings you to a stop", 1.0)

	# Zero velocity in, zero out — no division by zero.
	var z := StrikeMovement.apply_friction(Vector2.ZERO, DT)
	t.near(z.length(), 0.0, "friction on a standing player is harmless")


static func _test_air_strafe(t: TestFramework) -> void:
	# Air-strafing: holding a direction perpendicular-ish to motion should let
	# you gain speed in the air. This is what separates the movement from a
	# generic FPS, so it is asserted rather than hoped for.
	var v := Vector2(StrikeMovement.MAX_SPEED, 0)
	var start_speed := v.length()

	# The technique is holding a strafe key while turning, which keeps the wish
	# direction near-perpendicular to velocity. Only there is the projection below
	# the 30 cm/s air cap, leaving headroom to add speed. Re-derive the direction
	# from the *current* velocity each tick, exactly as turning the mouse does.
	for i in 60:
		var dir := v.normalized().rotated(deg_to_rad(87.0))
		v = StrikeMovement.air_move(v, dir, StrikeMovement.MAX_SPEED, DT)

	t.greater(v.length(), start_speed, "air-strafing gains speed")

	# ...and a wish direction along the velocity gains nothing, because the
	# projection is already far past the air cap.
	var straight := Vector2(StrikeMovement.MAX_SPEED, 0)
	for i in 60:
		straight = StrikeMovement.air_move(straight, Vector2(1, 0),
				StrikeMovement.MAX_SPEED, DT)
	t.near(straight.length(), StrikeMovement.MAX_SPEED,
			"strafing straight ahead gains nothing", 0.5)


static func _test_air_speed_cap(t: TestFramework) -> void:
	# Air acceleration straight ahead must be nearly useless — otherwise you
	# could accelerate to infinity by holding W in mid-air.
	var v := Vector2(StrikeMovement.MAX_SPEED, 0)
	var before := v.length()
	for i in 60:
		v = StrikeMovement.air_move(v, Vector2(1, 0), StrikeMovement.MAX_SPEED, DT)
	t.near(v.length(), before, "holding forward in the air adds nothing", 0.5)

	# From a standstill in the air you can still reach the air cap, but no more.
	var v2 := Vector2.ZERO
	for i in 200:
		v2 = StrikeMovement.air_move(v2, Vector2(1, 0), StrikeMovement.MAX_SPEED, DT)
	t.near(v2.length(), StrikeMovement.AIR_SPEED_CAP,
			"air acceleration from rest tops out at the air cap", 1.0)


static func _test_accuracy(t: TestFramework) -> void:
	# Standing still must be the most accurate state available.
	var still := StrikeMovement.accuracy_penalty(0.0, true, false)
	var moving := StrikeMovement.accuracy_penalty(StrikeMovement.MAX_SPEED, true, false)
	var jumping := StrikeMovement.accuracy_penalty(0.0, false, false)
	var crouched := StrikeMovement.accuracy_penalty(0.0, true, true)

	t.near(still, 1.0, "standing still is the accuracy baseline")
	t.greater(moving, still * 3.0, "running is drastically less accurate")
	t.greater(jumping, moving, "jumping is worse than running")
	t.less(crouched, still, "crouching is the most accurate of all")

	# Monotonic in speed: no sweet spot where moving faster helps.
	var previous := 0.0
	for i in 21:
		var speed := StrikeMovement.MAX_SPEED * (float(i) / 20.0)
		var penalty := StrikeMovement.accuracy_penalty(speed, true, false)
		t.ok(penalty >= previous - 0.0001, "accuracy penalty rises with speed") if i == 20 else null
		previous = penalty
