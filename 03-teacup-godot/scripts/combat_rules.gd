class_name TeacupRules
extends RefCounted

## Core rules for TEACUP, as pure functions.
##
## The design constraint that drives all of this: every death must be the
## player's fault. A boss rush is only fair if attacks telegraph, i-frames are
## honest, and the retry is instant. Those are testable properties, so they are
## tested rather than hoped for.

# ── Player ──────────────────────────────────────────────────────────────────
const MAX_HP := 3
const DASH_DURATION := 0.28
const DASH_IFRAMES := 0.20        ## Invulnerable window inside the dash.
const DASH_COOLDOWN := 0.55
const DASH_SPEED := 1400.0

const MOVE_SPEED := 620.0
const JUMP_VELOCITY := 1150.0
const GRAVITY := 2400.0

const PARRY_WINDOW := 0.18        ## How long a parry attempt stays active.
const SUPER_PER_PARRY := 25.0     ## Meter gained per successful parry.
const SUPER_MAX := 100.0
const EX_COST := 25.0             ## An EX move spends a quarter of the meter.

const SHOT_INTERVAL := 0.11
const SHOT_DAMAGE := 3.2
const EX_DAMAGE := 22.0
const SUPER_DAMAGE := 65.0

## Retry must be under a second, or a game this hard becomes miserable.
const RETRY_SECONDS := 0.6

# ── Bosses ──────────────────────────────────────────────────────────────────
enum Boss { BOTTLECAP, GRAMOPHONE, TEAPOT }

## Every phase telegraphs before it commits. Below this the attack is unreadable.
const MIN_TELEGRAPH := 0.35


static func boss_name(boss: int) -> String:
	match boss:
		Boss.BOTTLECAP: return "Fizzwick the Bottlecap"
		Boss.GRAMOPHONE: return "Maestro Grind"
		Boss.TEAPOT: return "Earl Grey the Terrible"
	return "Unknown"


## Total HP for a boss phase. Phases get shorter as they get harder, so the
## fight accelerates instead of dragging.
static func phase_health(boss: int, phase: int) -> float:
	var base := 300.0
	match boss:
		Boss.BOTTLECAP: base = 260.0
		Boss.GRAMOPHONE: base = 320.0
		Boss.TEAPOT: base = 380.0
	# Phase 0 is the longest; later phases are shorter but deadlier.
	var scale: float = [1.0, 0.85, 0.7][clampi(phase, 0, 2)]
	return base * scale


## How long the boss telegraphs before committing to an attack, in this phase.
## Later phases telegraph faster — that is the difficulty curve — but never
## below MIN_TELEGRAPH, because an unreadable attack is an unfair one.
static func telegraph_time(boss: int, phase: int) -> float:
	var base := 0.85
	match boss:
		Boss.BOTTLECAP: base = 0.90
		Boss.GRAMOPHONE: base = 0.80
		Boss.TEAPOT: base = 0.75
	var t := base - 0.18 * float(clampi(phase, 0, 2))
	return maxf(t, MIN_TELEGRAPH)


## Seconds between attacks in a phase.
static func attack_interval(boss: int, phase: int) -> float:
	var base := 2.4
	match boss:
		Boss.BOTTLECAP: base = 2.5
		Boss.GRAMOPHONE: base = 2.2
		Boss.TEAPOT: base = 2.0
	return maxf(base - 0.35 * float(clampi(phase, 0, 2)), 1.0)


## Number of distinct attacks a phase can use. Every phase must have its own
## set, or the three phases are one phase with a different colour.
static func attack_count(boss: int, phase: int) -> int:
	return 2 + clampi(phase, 0, 2)


## Which phase a boss is in, given remaining health fraction of the whole fight.
static func phase_for_progress(fraction_remaining: float) -> int:
	if fraction_remaining > 0.66:
		return 0
	if fraction_remaining > 0.33:
		return 1
	return 2


## Damage a player shot deals, including EX and super.
static func shot_damage(is_ex: bool, is_super: bool) -> float:
	if is_super:
		return SUPER_DAMAGE
	if is_ex:
		return EX_DAMAGE
	return SHOT_DAMAGE


## Can an EX move be spent right now?
static func can_ex(meter: float) -> bool:
	return meter >= EX_COST


static func can_super(meter: float) -> bool:
	return meter >= SUPER_MAX


## Meter after gaining from a parry, clamped.
static func meter_after_parry(meter: float) -> float:
	return minf(meter + SUPER_PER_PARRY, SUPER_MAX)


static func meter_after_spend(meter: float, is_super: bool) -> float:
	var cost := SUPER_MAX if is_super else EX_COST
	return maxf(meter - cost, 0.0)


## Is the player invulnerable this far into a dash?
static func dash_invulnerable(time_since_dash: float) -> bool:
	return time_since_dash >= 0.0 and time_since_dash < DASH_IFRAMES


## Does a parry attempt started at `parry_time` connect with a pink object that
## becomes parryable at `object_time`?
static func parry_connects(parry_time: float, object_time: float) -> bool:
	var delta := object_time - parry_time
	return delta >= 0.0 and delta <= PARRY_WINDOW


## Grade for a completed boss, from time taken and hits received.
## A-F, where an A demands a clean fight rather than merely a fast one.
static func grade(seconds: float, hits_taken: int, parries: int) -> String:
	var score := 100.0
	score -= float(hits_taken) * 22.0        ## Getting hit dominates.
	score -= maxf(seconds - 60.0, 0.0) * 0.4  ## Slow is penalised gently.
	score += float(parries) * 4.0             ## Parrying is rewarded.

	if score >= 95.0:
		return "A"
	if score >= 80.0:
		return "B"
	if score >= 62.0:
		return "C"
	if score >= 45.0:
		return "D"
	if score >= 25.0:
		return "E"
	return "F"
