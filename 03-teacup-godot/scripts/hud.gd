extends CanvasLayer

## HUD: hit points, super meter, boss health, death counter, grades.

var stage: Node = null
var intro: Node = null

var _label: RichTextLabel
var _boss_bar: ColorRect
var _boss_bg: ColorRect


func _ready() -> void:
	layer = 10
	var root := Control.new()
	root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	_boss_bg = ColorRect.new()
	_boss_bg.color = Color(0, 0, 0, 0.55)
	_boss_bg.position = Vector2(360, 40)
	_boss_bg.size = Vector2(1200, 26)
	root.add_child(_boss_bg)

	_boss_bar = ColorRect.new()
	_boss_bar.color = Color(0.85, 0.32, 0.30)
	_boss_bar.position = Vector2(362, 42)
	_boss_bar.size = Vector2(1196, 22)
	root.add_child(_boss_bar)

	_label = RichTextLabel.new()
	_label.bbcode_enabled = true
	_label.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(_label)


func _hearts(p) -> String:
	var out := ""
	for i in TeacupRules.MAX_HP:
		out += "[color=#ff5a6e]@[/color] " if i < p.hp else "[color=#553]. [/color]"
	return out


func _meter(p) -> String:
	var out := ""
	var filled := int(p.meter / 25.0)
	for i in 4:
		out += "[color=#ffd24a]#[/color]" if i < filled else "[color=#553]-[/color]"
	return out


func _process(_delta: float) -> void:
	# Intro stage: same hearts and meter, no boss bar.
	if intro != null and is_instance_valid(intro):
		_boss_bg.visible = false
		_boss_bar.visible = false
		var ip = intro.player
		if ip != null and is_instance_valid(ip):
			_label.text = ("[font_size=30]%s   super %s[/font_size]\n"
					+ "[center][font_size=22]reach the golden marker[/font_size][/center]") % [
					_hearts(ip), _meter(ip)]
		return

	if stage == null or not is_instance_valid(stage):
		return

	var p = stage.player
	var b = stage.boss

	if b != null and is_instance_valid(b) and b.max_health > 0.0:
		_boss_bar.size.x = 1196.0 * clampf(b.health / b.max_health, 0.0, 1.0)
		_boss_bg.visible = true
		_boss_bar.visible = true
	else:
		_boss_bg.visible = false
		_boss_bar.visible = false

	if p == null or not is_instance_valid(p):
		return

	var hearts := _hearts(p)
	var meter_blocks := _meter(p)

	var boss_label := ""
	if b != null and is_instance_valid(b):
		boss_label = "%s   phase %d/3" % [TeacupRules.boss_name(b.boss_id), b.phase + 1]

	var grade_text := ""
	if not stage.grades.is_empty():
		grade_text = "   grades: " + " ".join(stage.grades)

	_label.text = ("[font_size=30]%s   super %s[/font_size]\n"
			+ "[font_size=18]deaths %d%s[/font_size]\n\n"
			+ "[center][font_size=20]%s[/font_size][/center]") % [
			hearts, meter_blocks, stage.deaths, grade_text, boss_label]

	if stage.run_complete:
		_label.text += "\n\n[center][font_size=54]ALL BOSSES DOWN[/font_size]\n" \
				+ "[font_size=22]deaths: %d   grades: %s[/font_size][/center]" % [
					stage.deaths, " ".join(stage.grades)]
