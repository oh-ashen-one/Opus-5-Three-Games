extends RefCounted


static func run(t: TestFramework) -> void:
	t.suite("match")
	_test_round_resolution(t)
	_test_bomb_rules(t)
	_test_match_structure(t)


static func _test_round_resolution(t: TestFramework) -> void:
	# Straight eliminations.
	var t_win := StrikeMatch.resolve_round(3, 0, false, false, false, false)
	t.eq(t_win.winner, StrikeMatch.Team.T, "Ts win by elimination")

	var ct_win := StrikeMatch.resolve_round(0, 2, false, false, false, false)
	t.eq(ct_win.winner, StrikeMatch.Team.CT, "CTs win by elimination")

	# Ongoing rounds resolve to nobody.
	var ongoing := StrikeMatch.resolve_round(3, 3, false, false, false, false)
	t.eq(ongoing.winner, -1, "a live round has no winner yet")

	# Time running out with no plant is a CT win.
	var timeout := StrikeMatch.resolve_round(2, 2, false, false, false, true)
	t.eq(timeout.winner, StrikeMatch.Team.CT, "CTs win on the clock")


static func _test_bomb_rules(t: TestFramework) -> void:
	# The rule that makes planting worth the risk: once the bomb is down, the
	# round clock is irrelevant.
	var planted_timeout := StrikeMatch.resolve_round(2, 2, true, false, false, true)
	t.eq(planted_timeout.winner, -1, "the clock cannot end a planted round")

	# Ts all dying with the bomb ticking does NOT save the CTs — they must defuse.
	var ts_dead_planted := StrikeMatch.resolve_round(0, 2, true, false, false, false)
	t.eq(ts_dead_planted.winner, -1, "killing every T does not defuse the bomb")

	# ...but with no plant, killing every T ends it.
	var ts_dead_no_plant := StrikeMatch.resolve_round(0, 2, false, false, false, false)
	t.eq(ts_dead_no_plant.winner, StrikeMatch.Team.CT, "no plant means Ts dying loses it")

	# Defuse and detonation.
	t.eq(StrikeMatch.resolve_round(0, 1, true, true, false, false).winner,
			StrikeMatch.Team.CT, "defusing wins it for CTs")
	t.eq(StrikeMatch.resolve_round(1, 0, true, false, true, false).winner,
			StrikeMatch.Team.T, "detonation wins it for Ts")

	# Mutual destruction with a live bomb is still undecided.
	t.eq(StrikeMatch.resolve_round(0, 0, true, false, false, false).winner, -1,
			"everyone dead with a live bomb is undecided")

	# Kits matter: the defuse must be meaningfully faster.
	t.less(StrikeMatch.defuse_time(true), StrikeMatch.defuse_time(false),
			"a kit defuses faster")

	# The classic tension: with 6 seconds left, a kit saves you and nothing else does.
	t.ok(StrikeMatch.can_defuse_in_time(6.0, true), "6s is enough with a kit")
	t.not_ok(StrikeMatch.can_defuse_in_time(6.0, false), "6s is not enough without one")
	t.ok(StrikeMatch.can_defuse_in_time(11.0, false), "11s is enough bare-handed")


static func _test_match_structure(t: TestFramework) -> void:
	# MR6: first to 7.
	t.eq(StrikeMatch.match_winner(7, 3), StrikeMatch.Team.T, "7 rounds wins it")
	t.eq(StrikeMatch.match_winner(3, 7), StrikeMatch.Team.CT, "7 rounds wins it for CTs")
	t.eq(StrikeMatch.match_winner(6, 6), -1, "6-6 is not a win")
	t.ok(StrikeMatch.is_match_over(6, 6), "6-6 ends the match as a draw")

	# Match point.
	t.ok(StrikeMatch.is_match_point(6, 2), "6 rounds is match point")
	t.ok(StrikeMatch.is_match_point(2, 6), "6 rounds is match point for either side")
	t.not_ok(StrikeMatch.is_match_point(5, 5), "5-5 is not match point")
	t.not_ok(StrikeMatch.is_match_point(7, 2), "a finished match is not match point")

	# Half time and the side swap — forgetting this is how a match silently
	# becomes 12 rounds on one side.
	t.ok(StrikeMatch.is_halftime(6), "half time after 6 rounds")
	t.not_ok(StrikeMatch.is_halftime(5), "not half time after 5")

	t.eq(StrikeMatch.side_for_round(StrikeMatch.Team.T, 1), StrikeMatch.Team.T,
			"first half keeps the starting side")
	t.eq(StrikeMatch.side_for_round(StrikeMatch.Team.T, 6), StrikeMatch.Team.T,
			"round 6 is still the first half")
	t.eq(StrikeMatch.side_for_round(StrikeMatch.Team.T, 7), StrikeMatch.Team.CT,
			"round 7 swaps sides")
	t.eq(StrikeMatch.side_for_round(StrikeMatch.Team.CT, 7), StrikeMatch.Team.T,
			"the swap works both ways")

	# The match must fit the ~10 minute budget the spec asks for.
	var shortest := StrikeMatch.ROUNDS_TO_WIN * (StrikeMatch.ROUND_SECONDS
			+ StrikeMatch.FREEZE_SECONDS)
	var longest := StrikeMatch.MAX_ROUNDS * (StrikeMatch.ROUND_SECONDS
			+ StrikeMatch.FREEZE_SECONDS)
	t.greater(shortest, 480.0, "even a 7-0 sweep is a real match")
	t.less(longest / 60.0, 30.0, "a full 12 rounds still fits one sitting")
