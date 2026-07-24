extends Node3D

## Boots the stage and attaches the HUD. Kept separate so the stage itself has
## no UI dependency and can be driven headlessly by the simulation.

func _ready() -> void:
	var stage := preload("res://scripts/stage.gd").new()
	stage.name = "Stage"
	add_child(stage)

	var hud := preload("res://scripts/hud.gd").new()
	hud.stage = stage
	add_child(hud)
