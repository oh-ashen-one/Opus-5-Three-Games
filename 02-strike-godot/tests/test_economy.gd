extends RefCounted


static func run(t: TestFramework) -> void:
	t.suite("economy")
	_test_loss_ladder(t)
	_test_rewards(t)
	_test_buy_planning(t)


static func _test_loss_ladder(t: TestFramework) -> void:
	# Losing repeatedly must fund a comeback, or a team that drops two rounds is
	# mathematically out of the match and the remaining rounds are a formality.
	var previous := 0
	for losses in range(1, 6):
		var bonus := StrikeEconomy.loss_bonus(losses)
		t.greater(float(bonus), float(previous), "loss bonus grows with the streak")
		previous = bonus

	# The ladder caps rather than growing forever.
	t.eq(StrikeEconomy.loss_bonus(5), StrikeEconomy.loss_bonus(9), "loss bonus caps out")

	# After enough losses a team must be able to afford a real rifle buy.
	var after_three := StrikeEconomy.START_MONEY + StrikeEconomy.loss_bonus(1) \
			+ StrikeEconomy.loss_bonus(2) + StrikeEconomy.loss_bonus(3)
	var full_buy := StrikeEconomy.cost_of(StrikeWeapons.Id.AK, true, true)
	t.greater(float(after_three), float(full_buy),
			"three straight losses funds a full buy")


static func _test_rewards(t: TestFramework) -> void:
	# Winning by any route pays more than losing.
	var win := StrikeEconomy.round_reward(true, "elimination", 0)
	var loss := StrikeEconomy.round_reward(false, "elimination", 1)
	t.greater(float(win), float(loss), "winning pays more than losing")

	# Bomb objectives pay more than a plain elimination.
	t.greater(float(StrikeEconomy.round_reward(true, "bomb", 0)), float(win),
			"detonating pays more than an ace")
	t.greater(float(StrikeEconomy.round_reward(true, "defuse", 0)), float(win),
			"defusing pays more than an ace")

	# Planting pays even in defeat, so executing a site is worth the risk.
	var lost_without_plant := StrikeEconomy.round_reward(false, "defuse", 1, false)
	var lost_with_plant := StrikeEconomy.round_reward(false, "defuse", 1, true)
	t.greater(float(lost_with_plant), float(lost_without_plant),
			"planting is rewarded even in a lost round")

	# Money is clamped both ends.
	t.eq(StrikeEconomy.clamp_money(-500), 0, "money never goes negative")
	t.eq(StrikeEconomy.clamp_money(99999), StrikeEconomy.MAX_MONEY, "money is capped")


static func _test_buy_planning(t: TestFramework) -> void:
	# Rich: full buy, armour, helmet, and a kit if CT.
	var rich := StrikeEconomy.plan_buy(16000, true, 5)
	t.eq(rich.weapon, StrikeWeapons.Id.M4, "rich CT buys the M4")
	t.ok(rich.armor and rich.helmet, "rich buy includes armour and helmet")
	t.ok(rich.kit, "rich CT buys a defuse kit")

	var rich_t := StrikeEconomy.plan_buy(16000, false, 5)
	t.eq(rich_t.weapon, StrikeWeapons.Id.AK, "rich T buys the AK")
	t.not_ok(rich_t.kit, "T never buys a defuse kit")

	# Broke: save rather than half-buying into a rifle round. Bots that
	# force-buy every round are free money and read as stupid.
	var broke := StrikeEconomy.plan_buy(900, true, 5)
	t.eq(broke.weapon, -1, "a broke team saves instead of force-buying")
	t.eq(broke.spend, 0, "saving spends nothing")

	# Mid money: SMG buy is a real choice, not a consolation prize.
	var mid := StrikeEconomy.plan_buy(2200, false, 5)
	t.eq(mid.weapon, StrikeWeapons.Id.SMG, "mid money buys the SMG")
	t.ok(mid.armor, "mid buy still takes armour")

	# Pistol round is handled specially.
	var pistol := StrikeEconomy.plan_buy(StrikeEconomy.START_MONEY, true, 1)
	t.eq(pistol.weapon, StrikeWeapons.Id.USP, "pistol round buys a pistol")

	# Every plan must be affordable — never plan a buy you cannot pay for.
	for money in [0, 500, 800, 1500, 2500, 3500, 5000, 8000, 16000]:
		for is_ct in [true, false]:
			var plan := StrikeEconomy.plan_buy(money, is_ct, 5)
			t.ok(plan.spend <= money,
					"plan at $%d is affordable (spends %d)" % [money, plan.spend])
