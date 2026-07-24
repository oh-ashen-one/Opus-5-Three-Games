class_name StrikeMatch
extends RefCounted

## Match and round rules for MR6: first to 7 rounds wins, sides swap at 6.
##
## Pure so the whole match structure — including the half-time swap and the
## match-point condition — is testable without playing 13 rounds.

const ROUNDS_TO_WIN := 7
const ROUNDS_PER_HALF := 6
const MAX_ROUNDS := 12          ## 6 + 6; a 6-6 tie is a draw in this ruleset.

const ROUND_SECONDS := 115.0    ## 1:55
const FREEZE_SECONDS := 12.0
const BUY_SECONDS := 20.0
const BOMB_TIMER := 40.0
const PLANT_SECONDS := 3.0
const DEFUSE_SECONDS := 10.0
const DEFUSE_WITH_KIT := 5.0

enum Phase { FREEZE, LIVE, PLANTED, ENDED }
enum Team { T, CT }


## Is it half time after this many completed rounds?
static func is_halftime(rounds_played: int) -> bool:
	return rounds_played == ROUNDS_PER_HALF


## Which team the player is on for a given round, given their starting side.
## Sides swap once, at half time.
static func side_for_round(starting_side: int, round_number: int) -> int:
	if round_number <= ROUNDS_PER_HALF:
		return starting_side
	return Team.CT if starting_side == Team.T else Team.T


## Has someone won the match?
static func match_winner(t_score: int, ct_score: int) -> int:
	if t_score >= ROUNDS_TO_WIN:
		return Team.T
	if ct_score >= ROUNDS_TO_WIN:
		return Team.CT
	return -1


static func is_match_over(t_score: int, ct_score: int) -> bool:
	if match_winner(t_score, ct_score) != -1:
		return true
	# A 6-6 finish is a draw rather than overtime, so a session always ends.
	return t_score + ct_score >= MAX_ROUNDS


## True when either team can win with the next round.
static func is_match_point(t_score: int, ct_score: int) -> bool:
	if is_match_over(t_score, ct_score):
		return false
	return t_score == ROUNDS_TO_WIN - 1 or ct_score == ROUNDS_TO_WIN - 1


## Time the defuse takes.
static func defuse_time(has_kit: bool) -> float:
	return DEFUSE_WITH_KIT if has_kit else DEFUSE_SECONDS


## Can the bomb still be defused with this much fuse left?
static func can_defuse_in_time(fuse_remaining: float, has_kit: bool) -> bool:
	return fuse_remaining >= defuse_time(has_kit)


## Decide a round outcome.
##
## Returns { "winner": Team or -1, "reason": String }
## A round with the bomb planted is NOT won by the clock running out — that is
## the single rule that makes planting worth the risk.
static func resolve_round(ts_alive: int, cts_alive: int, bomb_planted: bool,
		bomb_defused: bool, bomb_exploded: bool, time_expired: bool) -> Dictionary:
	if bomb_defused:
		return {"winner": Team.CT, "reason": "defuse"}
	if bomb_exploded:
		return {"winner": Team.T, "reason": "bomb"}
	if cts_alive <= 0 and ts_alive > 0:
		return {"winner": Team.T, "reason": "elimination"}
	if ts_alive <= 0 and cts_alive > 0:
		# Ts dying with the bomb ticking does NOT save the CTs.
		if bomb_planted:
			return {"winner": -1, "reason": "bomb_still_live"}
		return {"winner": Team.CT, "reason": "elimination"}
	if ts_alive <= 0 and cts_alive <= 0:
		# Mutual destruction: bomb decides, otherwise CTs hold.
		if bomb_planted:
			return {"winner": -1, "reason": "bomb_still_live"}
		return {"winner": Team.CT, "reason": "elimination"}
	if time_expired:
		if bomb_planted:
			# Clock is irrelevant once it's planted.
			return {"winner": -1, "reason": "bomb_still_live"}
		return {"winner": Team.CT, "reason": "time"}
	return {"winner": -1, "reason": "ongoing"}
