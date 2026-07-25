extends Node3D

## Runs a full MR6 match: builds the map, spawns 5v5, and drives the round loop
## through freeze -> live -> planted -> ended, scoring with StrikeMatch.

const TEAM_SIZE := 5

var t_score := 0
var ct_score := 0
var round_number := 1
var phase: int = StrikeMatch.Phase.FREEZE
var phase_time := StrikeMatch.FREEZE_SECONDS

var bomb_planted := false
var bomb_defused := false
var bomb_exploded := false
var bomb_timer := 0.0
var bomb_position := Vector3.ZERO
var defuse_progress := 0.0
var plant_progress := 0.0

var player: StrikePlayer
var bots: Array[StrikeBot] = []
var player_starting_side: int = StrikeMatch.Team.T

var t_loss_streak := 0
var ct_loss_streak := 0
var match_over := false
var match_winner := -1

@onready var hud: Node = null


func _ready() -> void:
	StrikeMapBuilder.build(self)
	_spawn_actors()
	hud = preload("res://scripts/hud.gd").new()
	hud.match_scene = self
	add_child(hud)
	_start_round()


func _spawn_actors() -> void:
	player = StrikePlayer.new()
	player.is_bot = false
	player.display_name = "You"
	player.team = player_starting_side
	player.add_to_group("players")
	add_child(player)
	player.died.connect(_on_death)

	# 4 team-mates and 5 opponents.
	for i in TEAM_SIZE * 2 - 1:
		var bot := StrikeBot.new()
		bot.is_bot = true
		var on_player_team := i < TEAM_SIZE - 1
		bot.team = player_starting_side if on_player_team \
				else (StrikeMatch.Team.CT if player_starting_side == StrikeMatch.Team.T
						else StrikeMatch.Team.T)
		bot.display_name = "%s %d" % ["Ally" if on_player_team else "Enemy", i + 1]
		bot.difficulty = 0.65
		bot.add_to_group("players")
		add_child(bot)
		bot.died.connect(_on_death)
		bots.append(bot)


func _all_players() -> Array:
	var out: Array = [player]
	out.append_array(bots)
	return out


func _start_round() -> void:
	phase = StrikeMatch.Phase.FREEZE
	phase_time = StrikeMatch.FREEZE_SECONDS
	bomb_planted = false
	bomb_defused = false
	bomb_exploded = false
	bomb_timer = 0.0
	defuse_progress = 0.0
	plant_progress = 0.0

	var t_index := 0
	var ct_index := 0
	var t_spawns := StrikeMapBuilder.spawn_points(false, TEAM_SIZE)
	var ct_spawns := StrikeMapBuilder.spawn_points(true, TEAM_SIZE)

	for p in _all_players():
		# Team assignment is fixed at spawn; the swap flips everyone at once.
		var this_side: int = p.team
		if StrikeMatch.is_halftime(round_number - 1) and round_number == StrikeMatch.ROUNDS_PER_HALF + 1:
			this_side = StrikeMatch.Team.CT if p.team == StrikeMatch.Team.T else StrikeMatch.Team.T
			p.team = this_side

		var spawn: Vector3
		if this_side == StrikeMatch.Team.T:
			spawn = t_spawns[mini(t_index, t_spawns.size() - 1)]
			t_index += 1
		else:
			spawn = ct_spawns[mini(ct_index, ct_spawns.size() - 1)]
			ct_index += 1

		p.reset_for_round(spawn, this_side)
		if p is StrikeBot:
			p.difficulty = 0.68 if this_side == StrikeMatch.Team.T else 0.55
			p.do_buy(round_number)
		elif round_number == 1:
			p.equip(StrikeWeapons.Id.USP)

	# One T carries the bomb. Prefer a bot, so a passive human player cannot
	# stall the round simply by never walking to a site.
	var carrier = null
	for p in _all_players():
		if p.team == StrikeMatch.Team.T and p is StrikeBot:
			carrier = p
			break
	if carrier == null:
		for p in _all_players():
			if p.team == StrikeMatch.Team.T:
				carrier = p
				break
	if carrier != null:
		carrier.carrying_bomb = true
		if carrier is StrikeBot:
			var site_a: bool = round_number % 2 == 1
			# Carrier takes the mid route: shortest path to either site.
			carrier.set_route(StrikeMapBuilder.route_to(false, site_a, 2),
					StrikeBot.State.PLANT)

	_assign_bot_objectives()


func _assign_bot_objectives() -> void:
	# Ts execute a site; CTs split to hold both. Simple, but it produces the
	# right shapes: attackers commit, defenders spread and hold rather than
	# roaming into mid, which used to kill the T side before any plant.
	var target_site: Vector3 = StrikeMapBuilder.SITE_A if (round_number % 2 == 1) \
			else StrikeMapBuilder.SITE_B
	var site_is_a: bool = round_number % 2 == 1
	var t_index := 0
	var ct_index := 0
	for bot in bots:
		if not is_instance_valid(bot):
			continue
		if bot.team == StrikeMatch.Team.T:
			# Split the execute across the two approaches to the site.
			bot.set_route(StrikeMapBuilder.route_to(false, site_is_a, t_index),
					StrikeBot.State.PUSH)
			t_index += 1
		else:
			# CTs take up positions on both sites rather than stacking one.
			var defend_a: bool = ct_index % 2 == 0
			bot.set_route(StrikeMapBuilder.route_to(true, defend_a, ct_index),
					StrikeBot.State.PUSH)
			ct_index += 1


func _process(delta: float) -> void:
	if match_over:
		return

	phase_time -= delta

	# Freeze time is only meaningful if it actually freezes everyone.
	var live: bool = phase != StrikeMatch.Phase.FREEZE
	for p in _all_players():
		if is_instance_valid(p):
			p.can_act = live

	match phase:
		StrikeMatch.Phase.FREEZE:
			if phase_time <= 0.0:
				phase = StrikeMatch.Phase.LIVE
				phase_time = StrikeMatch.ROUND_SECONDS
		StrikeMatch.Phase.LIVE:
			_tick_live(delta)
		StrikeMatch.Phase.PLANTED:
			_tick_planted(delta)


func _tick_live(delta: float) -> void:
	_check_plant(delta)
	_evaluate_round(phase_time <= 0.0)


func _tick_planted(delta: float) -> void:
	bomb_timer -= delta
	_check_defuse(delta)
	if bomb_timer <= 0.0 and not bomb_defused:
		bomb_exploded = true
	_evaluate_round(false)


func _check_plant(delta: float) -> void:
	if bomb_planted:
		return
	for p in _all_players():
		if not p.is_alive or not p.carrying_bomb or p.team != StrikeMatch.Team.T:
			continue
		var on_a: bool = p.global_position.distance_to(StrikeMapBuilder.SITE_A) < 500.0
		var on_b: bool = p.global_position.distance_to(StrikeMapBuilder.SITE_B) < 500.0
		if on_a or on_b:
			plant_progress += delta
			if plant_progress >= StrikeMatch.PLANT_SECONDS:
				bomb_planted = true
				bomb_position = p.global_position
				bomb_timer = StrikeMatch.BOMB_TIMER
				phase = StrikeMatch.Phase.PLANTED
			return
	plant_progress = 0.0


func _check_defuse(delta: float) -> void:
	for p in _all_players():
		if not p.is_alive or p.team != StrikeMatch.Team.CT:
			continue
		if p.global_position.distance_to(bomb_position) > 250.0:
			continue
		defuse_progress += delta
		if defuse_progress >= StrikeMatch.defuse_time(p.has_kit):
			bomb_defused = true
		return
	defuse_progress = 0.0


func _count_alive(team: int) -> int:
	var n := 0
	for p in _all_players():
		if is_instance_valid(p) and p.is_alive and p.team == team:
			n += 1
	return n


func _evaluate_round(time_expired: bool) -> void:
	var result := StrikeMatch.resolve_round(
			_count_alive(StrikeMatch.Team.T),
			_count_alive(StrikeMatch.Team.CT),
			bomb_planted, bomb_defused, bomb_exploded, time_expired)

	if result.winner == -1:
		return

	_award_round(result.winner, result.reason)


func _award_round(winner: int, reason: String) -> void:
	if winner == StrikeMatch.Team.T:
		t_score += 1
		ct_loss_streak += 1
		t_loss_streak = 0
	else:
		ct_score += 1
		t_loss_streak += 1
		ct_loss_streak = 0

	for p in _all_players():
		if not is_instance_valid(p):
			continue
		var won: bool = p.team == winner
		var streak: int = t_loss_streak if p.team == StrikeMatch.Team.T else ct_loss_streak
		var reward: int = StrikeEconomy.round_reward(won, reason, streak, bomb_planted)
		p.money = StrikeEconomy.clamp_money(p.money + reward)

	round_number += 1

	if StrikeMatch.is_match_over(t_score, ct_score):
		match_over = true
		match_winner = StrikeMatch.match_winner(t_score, ct_score)
		phase = StrikeMatch.Phase.ENDED
		return

	_start_round()


func _on_death(victim: Node, killer: Node) -> void:
	if killer is StrikePlayer and killer != victim:
		var reward: int = StrikeWeapons.spec(killer.weapon_id).kill_reward
		killer.money = StrikeEconomy.clamp_money(killer.money + reward)

	# The bomb must survive its carrier. Without this, killing one specific bot
	# ends the T side's ability to win the round at all, and no round is ever
	# decided by the objective.
	if victim is StrikePlayer and victim.carrying_bomb:
		victim.carrying_bomb = false
		_hand_bomb_to_nearest_t(victim.global_position)


func _hand_bomb_to_nearest_t(from: Vector3) -> void:
	var best = null
	var best_distance := INF
	for p in _all_players():
		if not is_instance_valid(p) or not p.is_alive:
			continue
		if p.team != StrikeMatch.Team.T:
			continue
		var d: float = from.distance_to(p.global_position)
		if d < best_distance:
			best_distance = d
			best = p
	if best != null:
		best.carrying_bomb = true
		# Whoever picks it up goes to plant, whatever they were doing before.
		if best is StrikeBot:
			var site_is_a: bool = round_number % 2 == 1
			best.set_route(StrikeMapBuilder.route_to(false, site_is_a, 0),
					StrikeBot.State.PLANT)


func restart() -> void:
	get_tree().reload_current_scene()
