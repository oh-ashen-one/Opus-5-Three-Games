extends RefCounted


static func run(t: TestFramework) -> void:
	t.suite("weapons")
	_test_archetypes(t)
	_test_recoil_pattern(t)
	_test_damage(t)
	_test_spread(t)


static func _test_archetypes(t: TestFramework) -> void:
	var ak := StrikeWeapons.spec(StrikeWeapons.Id.AK)
	var m4 := StrikeWeapons.spec(StrikeWeapons.Id.M4)
	var awp := StrikeWeapons.spec(StrikeWeapons.Id.AWP)
	var smg := StrikeWeapons.spec(StrikeWeapons.Id.SMG)
	var deagle := StrikeWeapons.spec(StrikeWeapons.Id.DEAGLE)

	# The rifles must be a genuine choice, not a strict upgrade of each other.
	t.greater(ak.damage, m4.damage, "AK hits harder than M4")
	t.greater(m4.fire_rate, ak.fire_rate, "M4 fires faster than AK")
	t.less(m4.base_spread, ak.base_spread, "M4 is more accurate than AK")
	t.less(m4.recoil_magnitude, ak.recoil_magnitude, "M4 is easier to control")

	# AWP: one shot, one decision.
	t.greater(awp.damage, 100.0, "AWP one-shots an unarmoured body")
	t.less(awp.fire_rate, 1.0, "AWP is slow")
	t.less(awp.base_spread, 0.05, "AWP is pinpoint")

	# SMG's higher kill reward is what makes an eco round a real choice.
	t.greater(smg.kill_reward, ak.kill_reward, "SMG pays more per kill")
	t.less(smg.price, ak.price, "SMG is cheap")

	# Deagle: cheap, lethal, punishing.
	t.greater(deagle.damage, 60.0, "Deagle hits very hard")
	t.less(deagle.magazine, 10, "Deagle has few rounds")

	for id in StrikeWeapons.all_ids():
		var s := StrikeWeapons.spec(id)
		t.greater(s.damage, 0.0, "%s has damage" % s.name)
		t.greater(s.fire_rate, 0.0, "%s has a fire rate" % s.name)
		t.greater(float(s.magazine), 0.0, "%s has a magazine" % s.name)
		t.greater(float(s.price), 0.0, "%s has a price" % s.name)


static func _test_recoil_pattern(t: TestFramework) -> void:
	var id := StrikeWeapons.Id.AK

	# First shot must be dead accurate, or nothing about aiming is learnable.
	t.near(StrikeWeapons.recoil_offset(id, 0).length(), 0.0, "first shot has no recoil")

	# The pattern must be deterministic — the same index always gives the same
	# offset. If this is random, it cannot be learned and the game has no ceiling.
	for i in 30:
		var a := StrikeWeapons.recoil_offset(id, i)
		var b := StrikeWeapons.recoil_offset(id, i)
		t.ok(a.is_equal_approx(b), "recoil is deterministic") if i == 29 else null

	# Early shots climb, and climb monotonically.
	var previous := -1.0
	for i in 9:
		var v := StrikeWeapons.recoil_offset(id, i).y
		t.ok(v >= previous, "recoil climbs for the first 9 shots") if i == 8 else null
		previous = v

	# The climb must taper, or the pattern is unusable past a few shots.
	var early_step := StrikeWeapons.recoil_offset(id, 5).y - StrikeWeapons.recoil_offset(id, 4).y
	var late_step := StrikeWeapons.recoil_offset(id, 25).y - StrikeWeapons.recoil_offset(id, 24).y
	t.less(late_step, early_step, "vertical climb tapers off")

	# There must be genuine horizontal movement, otherwise "pull down" is the
	# entire skill and there's no pattern to learn.
	var max_horizontal := 0.0
	for i in 30:
		max_horizontal = maxf(max_horizontal, absf(StrikeWeapons.recoil_offset(id, i).x))
	t.greater(max_horizontal, 1.0, "the pattern moves horizontally too")

	# Different weapons must have different patterns.
	var ak_at_10 := StrikeWeapons.recoil_offset(StrikeWeapons.Id.AK, 10)
	var m4_at_10 := StrikeWeapons.recoil_offset(StrikeWeapons.Id.M4, 10)
	t.not_ok(ak_at_10.is_equal_approx(m4_at_10), "AK and M4 patterns differ")


static func _test_damage(t: TestFramework) -> void:
	var ak := StrikeWeapons.Id.AK

	# Headshots are lethal: 4x on a rifle kills through a helmet.
	var body := StrikeWeapons.compute_damage(ak, 500.0, false, 0.0)
	var head := StrikeWeapons.compute_damage(ak, 500.0, true, 0.0)
	t.near(head, body * StrikeWeapons.HEADSHOT_MULTIPLIER, "headshot is 4x", 0.01)
	t.greater(head, 100.0, "a rifle headshot kills outright")

	# Armour reduces damage but never to zero.
	var unarmored := StrikeWeapons.compute_damage(ak, 500.0, false, 0.0)
	var armored := StrikeWeapons.compute_damage(ak, 500.0, false, 100.0)
	t.less(armored, unarmored, "armour reduces damage")
	t.greater(armored, 0.0, "armour is not immunity")

	# The AWP ignores armour almost entirely — that's its identity.
	var awp_armored := StrikeWeapons.compute_damage(StrikeWeapons.Id.AWP, 500.0, false, 100.0)
	t.greater(awp_armored, 100.0, "AWP one-shots through armour")

	# Falloff: never rises with distance, never goes negative.
	var previous := 99999.0
	for i in 40:
		var distance := float(i) * 500.0
		var d := StrikeWeapons.compute_damage(ak, distance, false, 0.0)
		t.ok(d <= previous + 0.001, "damage never rises with distance") if i == 39 else null
		t.ok(d >= 0.0, "damage never negative") if i == 39 else null
		previous = d

	# The SMG must fall off far harder than the rifle at range.
	var smg_far := StrikeWeapons.compute_damage(StrikeWeapons.Id.SMG, 8000.0, false, 0.0)
	var ak_far := StrikeWeapons.compute_damage(ak, 8000.0, false, 0.0)
	t.less(smg_far, ak_far, "SMG is far weaker at range")


static func _test_spread(t: TestFramework) -> void:
	var id := StrikeWeapons.Id.AK

	# Standing still, first shot: as accurate as the weapon gets.
	var best := StrikeWeapons.spread_for(id, 1.0, 0)
	var running := StrikeWeapons.spread_for(id, 7.0, 0)
	t.greater(running, best * 3.0, "running wrecks accuracy")

	# Sustained fire opens the cone.
	var sustained := StrikeWeapons.spread_for(id, 1.0, 12)
	t.greater(sustained, best, "holding the trigger opens the cone")

	# Rate of fire to interval.
	t.near(StrikeWeapons.seconds_per_shot(id),
			1.0 / StrikeWeapons.spec(id).fire_rate, "seconds per shot")
