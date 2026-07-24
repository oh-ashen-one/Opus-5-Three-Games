extends CanvasLayer

## HUD, buy menu, pause and end screens, drawn with Control nodes built in code.
## No .tscn UI assets, so the whole interface stays reviewable in the diff.

var match_scene: Node = null

var _root: Control
var _score_label: Label
var _timer_label: Label
var _health_label: Label
var _ammo_label: Label
var _money_label: Label
var _status_label: Label
var _menu_panel: PanelContainer
var _menu_label: RichTextLabel
var _crosshair: Control

var _paused := false
var _menu_index := 0
var _buy_open := false


func _ready() -> void:
	layer = 10
	_build()


func _build() -> void:
	_root = Control.new()
	_root.set_anchors_preset(Control.PRESET_FULL_RECT)
	_root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_root)

	_crosshair = Control.new()
	_crosshair.set_anchors_preset(Control.PRESET_CENTER)
	_crosshair.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_root.add_child(_crosshair)
	_crosshair.draw.connect(_draw_crosshair)

	_score_label = _make_label(Vector2(0, 16), 28, HORIZONTAL_ALIGNMENT_CENTER)
	_score_label.set_anchors_preset(Control.PRESET_CENTER_TOP)
	_timer_label = _make_label(Vector2(0, 52), 22, HORIZONTAL_ALIGNMENT_CENTER)
	_timer_label.set_anchors_preset(Control.PRESET_CENTER_TOP)

	_health_label = _make_label(Vector2(40, -80), 26)
	_health_label.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	_money_label = _make_label(Vector2(40, -120), 20)
	_money_label.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	_ammo_label = _make_label(Vector2(-220, -80), 26)
	_ammo_label.set_anchors_preset(Control.PRESET_BOTTOM_RIGHT)
	_status_label = _make_label(Vector2(0, -160), 24, HORIZONTAL_ALIGNMENT_CENTER)
	_status_label.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)

	_menu_panel = PanelContainer.new()
	_menu_panel.set_anchors_preset(Control.PRESET_CENTER)
	_menu_panel.position = Vector2(-260, -180)
	_menu_panel.custom_minimum_size = Vector2(520, 360)
	_menu_panel.visible = false
	_root.add_child(_menu_panel)

	_menu_label = RichTextLabel.new()
	_menu_label.bbcode_enabled = true
	_menu_label.fit_content = true
	_menu_panel.add_child(_menu_label)


func _make_label(offset: Vector2, size: int,
		align := HORIZONTAL_ALIGNMENT_LEFT) -> Label:
	var l := Label.new()
	l.position = offset
	l.horizontal_alignment = align
	l.add_theme_font_size_override("font_size", size)
	l.add_theme_color_override("font_color", Color(0.95, 0.95, 0.92))
	l.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.8))
	l.add_theme_constant_override("outline_size", 4)
	_root.add_child(l)
	return l


func _draw_crosshair() -> void:
	var c := Color(0.4, 1.0, 0.45, 0.9)
	var gap := 6.0
	var len := 10.0
	var thick := 2.0
	_crosshair.draw_rect(Rect2(-gap - len, -thick * 0.5, len, thick), c)
	_crosshair.draw_rect(Rect2(gap, -thick * 0.5, len, thick), c)
	_crosshair.draw_rect(Rect2(-thick * 0.5, -gap - len, thick, len), c)
	_crosshair.draw_rect(Rect2(-thick * 0.5, gap, thick, len), c)


func _process(_delta: float) -> void:
	if match_scene == null or not is_instance_valid(match_scene):
		return

	var p = match_scene.player
	if p != null and is_instance_valid(p):
		_health_label.text = "HP %d    ARMOR %d" % [int(p.health), int(p.armor)]
		_money_label.text = "$%d" % p.money
		var spec = StrikeWeapons.spec(p.weapon_id)
		_ammo_label.text = "RELOADING" if p.is_reloading() else "%s  %d/%d" % [
				spec.name, p.ammo, spec.magazine]

	_score_label.text = "T  %d     %d  CT" % [match_scene.t_score, match_scene.ct_score]

	if match_scene.match_over:
		_show_end_screen()
		return

	if match_scene.bomb_planted:
		_timer_label.text = "BOMB  %0.1f" % maxf(match_scene.bomb_timer, 0.0)
		_timer_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.3))
	else:
		_timer_label.text = _format_clock(match_scene.phase_time)
		_timer_label.add_theme_color_override("font_color", Color(0.95, 0.95, 0.92))

	if _buy_open and match_scene.phase != StrikeMatch.Phase.FREEZE:
		_close_buy_menu()

	var status := ""
	if match_scene.phase == StrikeMatch.Phase.FREEZE:
		status = "FREEZE TIME  -  press B to buy"
	elif match_scene.plant_progress > 0.0:
		status = "PLANTING  %0.0f%%" % (match_scene.plant_progress / StrikeMatch.PLANT_SECONDS * 100.0)
	elif match_scene.defuse_progress > 0.0:
		status = "DEFUSING  %0.0f%%" % (match_scene.defuse_progress / StrikeMatch.DEFUSE_SECONDS * 100.0)
	elif StrikeMatch.is_match_point(match_scene.t_score, match_scene.ct_score):
		status = "MATCH POINT"
	_status_label.text = status


func _format_clock(seconds: float) -> String:
	var total := int(maxf(seconds, 0.0))
	return "%d:%02d" % [total / 60, total % 60]


func _show_end_screen() -> void:
	_menu_panel.visible = true
	var won: bool = match_scene.match_winner == match_scene.player.team
	var title := "VICTORY" if won else "DEFEAT"
	_menu_label.text = "[center][font_size=44]%s[/font_size]\n\n%d - %d\n\n%s\n\n%s[/center]" % [
			title, match_scene.t_score, match_scene.ct_score,
			"[color=#7f7]> PLAY AGAIN <[/color]" if _menu_index == 0 else "PLAY AGAIN",
			"[color=#7f7]> QUIT <[/color]" if _menu_index == 1 else "QUIT"]
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE


func _unhandled_input(event: InputEvent) -> void:
	if match_scene == null:
		return

	if event is InputEventKey and event.pressed and not event.echo:
		match event.keycode:
			KEY_ESCAPE:
				if _buy_open:
					_close_buy_menu()
				elif not match_scene.match_over:
					_toggle_pause()
			KEY_B:
				if match_scene.phase == StrikeMatch.Phase.FREEZE:
					if _buy_open:
						_close_buy_menu()
					else:
						_open_buy_menu()
			KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8:
				if _buy_open:
					_purchase(event.keycode)
			KEY_DOWN:
				_menu_index = mini(_menu_index + 1, 1)
			KEY_UP:
				_menu_index = maxi(_menu_index - 1, 0)
			KEY_ENTER, KEY_KP_ENTER:
				_confirm()


func _toggle_pause() -> void:
	_paused = not _paused
	get_tree().paused = _paused
	_menu_panel.visible = _paused
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if _paused else Input.MOUSE_MODE_CAPTURED
	if _paused:
		_menu_label.text = "[center][font_size=40]PAUSED[/font_size]\n\n%s\n\n%s[/center]" % [
				"[color=#7f7]> RESUME <[/color]" if _menu_index == 0 else "RESUME",
				"[color=#7f7]> QUIT <[/color]" if _menu_index == 1 else "QUIT"]


func _close_buy_menu() -> void:
	_buy_open = false
	_menu_panel.visible = false
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


## Actually spend the money. Before this the buy menu listed prices and did
## nothing at all when you pressed a key -- the player could never buy anything,
## which quietly removed the entire economy from the player's side of the game.
func _purchase(keycode: int) -> void:
	var p = match_scene.player
	if p == null or not is_instance_valid(p):
		return

	var weapon_keys := {
		KEY_1: StrikeWeapons.Id.USP, KEY_2: StrikeWeapons.Id.DEAGLE,
		KEY_3: StrikeWeapons.Id.SMG, KEY_4: StrikeWeapons.Id.AK,
		KEY_5: StrikeWeapons.Id.M4, KEY_6: StrikeWeapons.Id.AWP,
	}

	if weapon_keys.has(keycode):
		var id: int = weapon_keys[keycode]
		var price: int = StrikeWeapons.spec(id).price
		if p.money >= price:
			p.money -= price
			p.equip(id)
	elif keycode == KEY_7:
		if p.armor < 100.0 and p.money >= StrikeEconomy.ARMOR_HELMET_PRICE:
			p.money -= StrikeEconomy.ARMOR_HELMET_PRICE
			p.armor = 100.0
			p.has_helmet = true
	elif keycode == KEY_8:
		if not p.has_kit and p.team == StrikeMatch.Team.CT \
				and p.money >= StrikeEconomy.KIT_PRICE:
			p.money -= StrikeEconomy.KIT_PRICE
			p.has_kit = true

	_open_buy_menu()  # refresh the panel with new prices/affordability


func _open_buy_menu() -> void:
	_buy_open = true
	# Buy menu: number keys purchase directly, which is faster on camera than
	# navigating a grid and matches how the real thing is actually played.
	var p = match_scene.player
	_menu_panel.visible = true
	var lines := "[center][font_size=32]BUY[/font_size]\n\n$%d\n\n" % p.money
	var options := [
		["1", StrikeWeapons.Id.USP], ["2", StrikeWeapons.Id.DEAGLE],
		["3", StrikeWeapons.Id.SMG], ["4", StrikeWeapons.Id.AK],
		["5", StrikeWeapons.Id.M4], ["6", StrikeWeapons.Id.AWP],
	]
	for o in options:
		var spec = StrikeWeapons.spec(o[1])
		var affordable: bool = p.money >= spec.price
		lines += "%s  %s  $%d%s\n" % [o[0], spec.name, spec.price,
				"" if affordable else "   (too expensive)"]
	lines += "\n7  Armour+Helmet $%d%s\n8  Defuse kit $%d%s\n\nB to close[/center]" % [
			StrikeEconomy.ARMOR_HELMET_PRICE,
			"   (owned)" if p.armor >= 100.0 else "",
			StrikeEconomy.KIT_PRICE,
			"   (owned)" if p.has_kit else ("" if p.team == StrikeMatch.Team.CT
					else "   (CT only)")]
	_menu_label.text = lines
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE


func _confirm() -> void:
	if match_scene.match_over:
		if _menu_index == 0:
			match_scene.restart()
		else:
			get_tree().quit()
	elif _paused:
		if _menu_index == 0:
			_toggle_pause()
		else:
			get_tree().quit()
