extends Node3D

## Boots the whole run: intro stage first, then the boss rush, with one HUD
## across both. Kept separate so each part has no UI dependency and can be
## driven headlessly.

var intro: TeacupIntroStage
var stage: Node
var hud: Node


func _ready() -> void:
	hud = preload("res://scripts/hud.gd").new()
	add_child(hud)

	if TeacupRunConfig.skip_intro:
		_start_boss_rush()
	else:
		intro = TeacupIntroStage.new()
		intro.name = "Intro"
		add_child(intro)
		intro.stage_cleared.connect(_on_intro_cleared)
		hud.intro = intro


func _on_intro_cleared() -> void:
	if intro != null and is_instance_valid(intro):
		intro.queue_free()
	hud.intro = null
	_start_boss_rush()


func _start_boss_rush() -> void:
	stage = preload("res://scripts/stage.gd").new()
	stage.name = "Stage"
	stage.start_index = TeacupRunConfig.start_boss
	add_child(stage)
	hud.stage = stage
