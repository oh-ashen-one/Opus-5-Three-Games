class_name TeacupRunConfig
extends RefCounted

## Which part of the run to start from. Set by the main menu's boss select.

static var start_boss := 0
static var skip_intro := false


static func reset() -> void:
	start_boss = 0
	skip_intro = false
