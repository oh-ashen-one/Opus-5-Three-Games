extends Control

## Main menu. Keyboard-driven, built in code so no UI assets are needed.

const ITEMS := ["PLAY", "SETTINGS", "QUIT"]

var _index := 0
var _in_settings := false
var _label: RichTextLabel

# Settings applied to the match when it starts.
static var mouse_sensitivity := 1.0
static var master_volume := 0.8
static var invert_y := false


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var bg := ColorRect.new()
	bg.color = Color(0.05, 0.06, 0.08)
	bg.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(bg)

	_label = RichTextLabel.new()
	_label.bbcode_enabled = true
	_label.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(_label)
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	_refresh()


func _refresh() -> void:
	if _in_settings:
		var rows := [
			"MOUSE SENSITIVITY   %.2f" % mouse_sensitivity,
			"MASTER VOLUME       %d%%" % int(master_volume * 100.0),
			"INVERT Y            %s" % ("ON" if invert_y else "OFF"),
			"BACK",
		]
		var text := "[center][font_size=40]SETTINGS[/font_size]\n\n"
		for i in rows.size():
			text += ("[color=#ffd24a]> %s <[/color]\n" if i == _index else "%s\n") % rows[i]
		text += "\nleft/right adjust   enter confirm[/center]"
		_label.text = text
		return

	var text := "[center]\n\n\n[font_size=64]STRIKE PROTOCOL[/font_size]\n"
	text += "[font_size=16]Opus 5 Three Games  -  project 2 of 3[/font_size]\n\n\n"
	for i in ITEMS.size():
		text += ("[color=#ffd24a]> %s <[/color]\n" if i == _index else "%s\n") % ITEMS[i]
	text += "\n\n[font_size=16]arrows to move   enter to select[/font_size][/center]"
	_label.text = text


func _unhandled_input(event: InputEvent) -> void:
	if not (event is InputEventKey and event.pressed and not event.echo):
		return

	var count := 4 if _in_settings else ITEMS.size()
	match event.keycode:
		KEY_UP:
			_index = maxi(_index - 1, 0)
		KEY_DOWN:
			_index = mini(_index + 1, count - 1)
		KEY_LEFT:
			_adjust(-1)
		KEY_RIGHT:
			_adjust(1)
		KEY_ENTER, KEY_KP_ENTER:
			_confirm()
	_refresh()


func _adjust(delta: int) -> void:
	if not _in_settings:
		return
	match _index:
		0: mouse_sensitivity = clampf(mouse_sensitivity + delta * 0.05, 0.1, 3.0)
		1: master_volume = clampf(master_volume + delta * 0.05, 0.0, 1.0)
		2: invert_y = not invert_y


func _confirm() -> void:
	if _in_settings:
		if _index == 3:
			_in_settings = false
			_index = 0
		return
	match _index:
		0: get_tree().change_scene_to_file("res://scenes/match.tscn")
		1:
			_in_settings = true
			_index = 0
		2: get_tree().quit()
