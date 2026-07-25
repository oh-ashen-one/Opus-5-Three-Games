class_name StrikeWeapons
extends RefCounted

## Weapon definitions and shooting math.
##
## Recoil is a *fixed pattern* per weapon, not random spray. That is the whole
## point: a pattern can be learned and pulled down against, which is what makes
## aim a skill rather than a dice roll.

enum Id { AK, M4, AWP, DEAGLE, PISTOL, USP, SMG }

const HEADSHOT_MULTIPLIER := 4.0
const ARMOR_ABSORB := 0.5   ## Fraction of damage armour soaks.


class Spec extends RefCounted:
	var id: int
	var name: String
	var damage: float
	var fire_rate: float          ## rounds per second
	var magazine: int
	var reload_time: float
	var price: int
	var kill_reward: int
	var armor_penetration: float  ## 0..1, how much armour it ignores
	var falloff_start: float      ## cm
	var falloff_end: float
	var min_damage_fraction: float
	var base_spread: float        ## degrees, standing still
	var recoil_magnitude: float   ## degrees per shot
	var is_sniper: bool

	func _init(p := {}) -> void:
		id = p.get("id", Id.AK)
		name = p.get("name", "")
		damage = p.get("damage", 30.0)
		fire_rate = p.get("fire_rate", 10.0)
		magazine = p.get("magazine", 30)
		reload_time = p.get("reload_time", 2.5)
		price = p.get("price", 2700)
		kill_reward = p.get("kill_reward", 300)
		armor_penetration = p.get("armor_penetration", 0.7)
		falloff_start = p.get("falloff_start", 2000.0)
		falloff_end = p.get("falloff_end", 8000.0)
		min_damage_fraction = p.get("min_damage_fraction", 0.5)
		base_spread = p.get("base_spread", 0.4)
		recoil_magnitude = p.get("recoil_magnitude", 1.8)
		is_sniper = p.get("is_sniper", false)


static func spec(id: int) -> Spec:
	match id:
		Id.AK:
			return Spec.new({"id": id, "name": "Vektor", "damage": 36.0, "fire_rate": 10.0,
				"magazine": 30, "reload_time": 2.4, "price": 2700, "kill_reward": 300,
				"armor_penetration": 0.775, "falloff_start": 3000.0, "falloff_end": 12000.0,
				"min_damage_fraction": 0.6, "base_spread": 0.35, "recoil_magnitude": 2.1})
		Id.M4:
			return Spec.new({"id": id, "name": "Lancer", "damage": 33.0, "fire_rate": 11.0,
				"magazine": 30, "reload_time": 3.1, "price": 3100, "kill_reward": 300,
				"armor_penetration": 0.70, "falloff_start": 3200.0, "falloff_end": 12000.0,
				"min_damage_fraction": 0.6, "base_spread": 0.30, "recoil_magnitude": 1.7})
		Id.AWP:
			return Spec.new({"id": id, "name": "Longbow", "damage": 115.0, "fire_rate": 0.68,
				"magazine": 10, "reload_time": 3.7, "price": 4750, "kill_reward": 100,
				"armor_penetration": 0.975, "falloff_start": 20000.0, "falloff_end": 40000.0,
				"min_damage_fraction": 0.9, "base_spread": 0.02, "recoil_magnitude": 5.0,
				"is_sniper": true})
		Id.DEAGLE:
			return Spec.new({"id": id, "name": "Talon", "damage": 63.0, "fire_rate": 4.0,
				"magazine": 7, "reload_time": 2.2, "price": 700, "kill_reward": 300,
				"armor_penetration": 0.93, "falloff_start": 2500.0, "falloff_end": 9000.0,
				"min_damage_fraction": 0.65, "base_spread": 0.5, "recoil_magnitude": 3.6})
		Id.PISTOL:
			return Spec.new({"id": id, "name": "Sidearm", "damage": 26.0, "fire_rate": 6.0,
				"magazine": 20, "reload_time": 2.0, "price": 200, "kill_reward": 300,
				"armor_penetration": 0.5, "falloff_start": 1500.0, "falloff_end": 6000.0,
				"min_damage_fraction": 0.5, "base_spread": 0.6, "recoil_magnitude": 1.4})
		Id.USP:
			return Spec.new({"id": id, "name": "Marshal", "damage": 35.0, "fire_rate": 5.0,
				"magazine": 12, "reload_time": 2.2, "price": 200, "kill_reward": 300,
				"armor_penetration": 0.505, "falloff_start": 1800.0, "falloff_end": 7000.0,
				"min_damage_fraction": 0.5, "base_spread": 0.45, "recoil_magnitude": 1.5})
		_:
			return Spec.new({"id": Id.SMG, "name": "Ripple", "damage": 27.0, "fire_rate": 13.3,
				"magazine": 30, "reload_time": 2.5, "price": 1250, "kill_reward": 600,
				"armor_penetration": 0.577, "falloff_start": 1200.0, "falloff_end": 5000.0,
				"min_damage_fraction": 0.4, "base_spread": 0.8, "recoil_magnitude": 1.5})


static func all_ids() -> Array:
	return [Id.AK, Id.M4, Id.AWP, Id.DEAGLE, Id.PISTOL, Id.USP, Id.SMG]


## The fixed recoil pattern. Shot index 0 is the first bullet.
##
## Shape: climbs almost straight up for the first ~8 shots, then pulls left,
## then sweeps right. Learnable by design — the same magazine always walks the
## same path, so pulling down and countersweeping is a trainable skill.
static func recoil_offset(id: int, shot_index: int) -> Vector2:
	var s := spec(id)
	if shot_index <= 0:
		# First shot is always perfectly accurate from a standstill.
		return Vector2.ZERO

	var i := float(shot_index)
	var vertical: float
	if shot_index < 9:
		vertical = i * s.recoil_magnitude
	else:
		# Climb tapers off rather than continuing forever.
		vertical = 9.0 * s.recoil_magnitude + (i - 9.0) * s.recoil_magnitude * 0.18

	var horizontal := 0.0
	if shot_index >= 6:
		# Left sweep, then right — the classic "J" tail.
		var phase := i - 6.0
		horizontal = -sin(phase * 0.45) * s.recoil_magnitude * 1.35
		if shot_index >= 16:
			horizontal += sin((i - 16.0) * 0.30) * s.recoil_magnitude * 1.8

	return Vector2(horizontal, vertical)


## Damage for one bullet, after distance falloff, armour and headshots.
static func compute_damage(id: int, distance: float, hit_head: bool, target_armor: float) -> float:
	var s := spec(id)

	var fraction := 1.0
	if distance > s.falloff_start:
		var span: float = maxf(s.falloff_end - s.falloff_start, 0.001)
		var alpha: float = clampf((distance - s.falloff_start) / span, 0.0, 1.0)
		fraction = lerpf(1.0, s.min_damage_fraction, alpha)

	var damage := s.damage * fraction
	if hit_head:
		damage *= HEADSHOT_MULTIPLIER

	if target_armor > 0.0:
		# Armour absorbs a share of what the weapon cannot penetrate.
		var blocked := (1.0 - s.armor_penetration) * ARMOR_ABSORB
		damage *= (1.0 - blocked)

	return maxf(damage, 0.0)


## Total spread in degrees for a shot: base, times the movement penalty.
static func spread_for(id: int, movement_penalty: float, shot_index: int) -> float:
	var s := spec(id)
	# Sustained fire opens the cone as well as walking the pattern.
	var sustained := 1.0 + minf(float(shot_index), 12.0) * 0.06
	return s.base_spread * movement_penalty * sustained


static func seconds_per_shot(id: int) -> float:
	var s := spec(id)
	return 1.0 / maxf(s.fire_rate, 0.001)
