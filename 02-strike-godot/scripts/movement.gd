class_name StrikeMovement
extends RefCounted

## Source-style movement, as pure static functions.
##
## This is the whole skill ceiling of the game, so none of it lives in a
## _physics_process where it can only be checked by feel. Counter-strafing,
## air-strafing and the speed cap are all unit tested against these functions.
##
## Units are cm/s to match the source games these numbers come from.

# ── Tuning ──────────────────────────────────────────────────────────────────
const MAX_SPEED := 250.0          ## Ground speed cap while running.
const WALK_SPEED := 130.0         ## Holding walk.
const CROUCH_SPEED := 90.0
const ACCELERATE := 5.5           ## Ground acceleration coefficient.
const AIR_ACCELERATE := 12.0      ## Air acceleration coefficient.
const FRICTION := 5.2
const STOP_SPEED := 80.0          ## Below this, friction applies a floor amount.
const AIR_SPEED_CAP := 30.0       ## The magic number that makes air-strafing work.
const JUMP_VELOCITY := 300.0
const GRAVITY := 800.0

## Ground acceleration. Deliberately *not* a lerp toward a target: the
## projection onto wish_dir is what allows counter-strafing to stop you dead
## and what makes strafe-jumping possible at all.
##
## velocity, wish_dir: horizontal only (Vector2, x/z of the world).
static func accelerate(velocity: Vector2, wish_dir: Vector2, wish_speed: float,
		accel: float, delta: float) -> Vector2:
	if wish_dir.length_squared() < 0.0001:
		return velocity
	var dir := wish_dir.normalized()
	# Speed already going the way we want.
	var current_speed := velocity.dot(dir)
	var add_speed := wish_speed - current_speed
	if add_speed <= 0.0:
		# Already at or above the wish speed in that direction: add nothing.
		# Without this clause you could accelerate forever in a straight line.
		return velocity
	var accel_speed: float = min(accel * wish_speed * delta, add_speed)
	return velocity + dir * accel_speed


## Air acceleration is the same function with the wish speed clamped hard.
## That clamp is the entire reason air-strafing works: you can always add a
## little speed perpendicular to your motion, but never much along it.
static func air_accelerate(velocity: Vector2, wish_dir: Vector2, wish_speed: float,
		delta: float) -> Vector2:
	var capped: float = min(wish_speed, AIR_SPEED_CAP)
	return accelerate(velocity, wish_dir, capped, AIR_ACCELERATE, delta)


## Ground friction. Applied before acceleration each tick.
static func apply_friction(velocity: Vector2, delta: float) -> Vector2:
	var speed := velocity.length()
	if speed < 0.0001:
		return Vector2.ZERO
	# Below stop speed, friction is computed against STOP_SPEED instead, so slow
	# movement decays quickly rather than creeping forever.
	var control: float = maxf(speed, STOP_SPEED)
	var drop := control * FRICTION * delta
	var new_speed: float = maxf(speed - drop, 0.0)
	return velocity * (new_speed / speed)


## One full ground tick: friction, then acceleration.
static func ground_move(velocity: Vector2, wish_dir: Vector2, wish_speed: float,
		delta: float) -> Vector2:
	var v := apply_friction(velocity, delta)
	return accelerate(v, wish_dir, wish_speed, ACCELERATE, delta)


## One full air tick. No friction in the air — that's what preserves speed.
static func air_move(velocity: Vector2, wish_dir: Vector2, wish_speed: float,
		delta: float) -> Vector2:
	return air_accelerate(velocity, wish_dir, wish_speed, delta)


## Target speed for a movement state.
static func wish_speed_for(is_crouching: bool, is_walking: bool) -> float:
	if is_crouching:
		return CROUCH_SPEED
	if is_walking:
		return WALK_SPEED
	return MAX_SPEED


## Accuracy penalty from movement, as a spread multiplier.
##
## This is what stops the game being a run-and-gun shooter: firing while moving
## must be genuinely bad, and firing mid-air must be useless.
static func accuracy_penalty(speed: float, on_floor: bool, is_crouching: bool) -> float:
	if not on_floor:
		# Jumping shots are near-worthless by design.
		return 8.0
	var moving := speed / MAX_SPEED
	# Quadratic: a slight adjustment is nearly free, a full sprint is not.
	var penalty := 1.0 + 6.0 * moving * moving
	if is_crouching:
		penalty *= 0.6
	return penalty
