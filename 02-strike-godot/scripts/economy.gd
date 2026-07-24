class_name StrikeEconomy
extends RefCounted

## Round economy. Pure functions so the whole money model can be tested without
## playing 13 rounds by hand.
##
## The economy is what makes the game a series of decisions rather than a series
## of gunfights: losing must fund a comeback, and winning must not run away with
## the match.

const START_MONEY := 800
const MAX_MONEY := 16000

const WIN_REWARD_ELIMINATION := 3250
const WIN_REWARD_BOMB := 3500
const WIN_REWARD_DEFUSE := 3500
const PLANT_BONUS := 300          ## Paid to Ts even in a lost round.

## Consecutive-loss bonus ladder. Losing repeatedly must fund a real buy again,
## or a team that drops two rounds is mathematically out of the match.
const LOSS_LADDER := [1400, 1900, 2400, 2900, 3400]

const KIT_PRICE := 400
const ARMOR_PRICE := 650
const ARMOR_HELMET_PRICE := 1000


static func clamp_money(amount: int) -> int:
	return clampi(amount, 0, MAX_MONEY)


## Money awarded for losing, given how many rounds in a row you've now lost.
static func loss_bonus(consecutive_losses: int) -> int:
	var index: int = clampi(consecutive_losses - 1, 0, LOSS_LADDER.size() - 1)
	return LOSS_LADDER[index]


## Award at the end of a round.
##
## won: did this team win.
## reason: "elimination" | "bomb" | "defuse" | "time"
## consecutive_losses: this team's loss streak *including* this round if lost.
## planted: did this team plant the bomb (Ts only).
static func round_reward(won: bool, reason: String, consecutive_losses: int,
		planted := false) -> int:
	var reward := 0
	if won:
		match reason:
			"bomb":
				reward = WIN_REWARD_BOMB
			"defuse":
				reward = WIN_REWARD_DEFUSE
			_:
				reward = WIN_REWARD_ELIMINATION
	else:
		reward = loss_bonus(consecutive_losses)
		if planted:
			# Planting is rewarded even in defeat, so executing is worth it.
			reward += PLANT_BONUS
	return reward


## Can this loadout be afforded?
static func can_afford(money: int, weapon_id: int, armor := false, helmet := false,
		kit := false) -> bool:
	return cost_of(weapon_id, armor, helmet, kit) <= money


static func cost_of(weapon_id: int, armor := false, helmet := false, kit := false) -> int:
	var total := 0
	if weapon_id >= 0:
		total += StrikeWeapons.spec(weapon_id).price
	if armor:
		total += ARMOR_HELMET_PRICE if helmet else ARMOR_PRICE
	if kit:
		total += KIT_PRICE
	return total


## What a bot should buy with the money it has, and how much it keeps.
##
## Returns { "weapon": id or -1, "armor": bool, "helmet": bool, "kit": bool,
##           "spend": int }
##
## The rule that matters: if a full buy isn't affordable, save rather than
## half-buying into a rifle round. Bots that force-buy every round are free
## money for the player and read as stupid.
static func plan_buy(money: int, is_ct: bool, round_number: int,
		teammates_saving := false) -> Dictionary:
	var plan := {"weapon": -1, "armor": false, "helmet": false, "kit": false, "spend": 0}

	var rifle: int = StrikeWeapons.Id.M4 if is_ct else StrikeWeapons.Id.AK
	var full_buy := cost_of(rifle, true, true, is_ct)

	if money >= full_buy:
		plan.weapon = rifle
		plan.armor = true
		plan.helmet = true
		plan.kit = is_ct
	elif money >= cost_of(StrikeWeapons.Id.SMG, true, false):
		# Mid buy: SMG and body armour. SMG kill reward makes this a real choice
		# rather than a consolation prize.
		plan.weapon = StrikeWeapons.Id.SMG
		plan.armor = true
	elif money >= cost_of(StrikeWeapons.Id.DEAGLE, true, false) and not teammates_saving:
		plan.weapon = StrikeWeapons.Id.DEAGLE
		plan.armor = true
	elif round_number == 1:
		# Pistol round: everyone can afford armour or a better pistol, not both.
		plan.weapon = StrikeWeapons.Id.USP
		plan.armor = money >= cost_of(StrikeWeapons.Id.USP, true, false)
	else:
		# Save. Explicitly buying nothing is a decision, not a failure.
		plan.weapon = -1

	plan.spend = cost_of(plan.weapon, plan.armor, plan.helmet, plan.kit)
	return plan
