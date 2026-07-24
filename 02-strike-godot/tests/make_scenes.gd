extends SceneTree

## Generates the .tscn files with Godot itself, so the resource format is
## guaranteed correct rather than hand-written and silently wrong.

func _init() -> void:
	_save("res://scripts/match_scene.gd", "Match", Node3D, "res://scenes/match.tscn")
	_save("res://scripts/main_menu.gd", "MainMenu", Control, "res://scenes/main_menu.tscn")
	_save("res://tests/sim_match.gd", "Sim", Node, "res://scenes/sim.tscn")
	quit(0)


func _save(script_path: String, node_name: String, base, out_path: String) -> void:
	var node = base.new()
	node.name = node_name
	node.set_script(load(script_path))
	var packed := PackedScene.new()
	var err := packed.pack(node)
	if err != OK:
		print("PACK FAILED %s: %d" % [out_path, err])
		return
	err = ResourceSaver.save(packed, out_path)
	print("%s -> %s (%d)" % [script_path, out_path, err])
