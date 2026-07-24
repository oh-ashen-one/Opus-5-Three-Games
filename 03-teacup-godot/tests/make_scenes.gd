extends SceneTree

## Writes the .tscn files as plain text with a verified format.

func _init() -> void:
	_write("res://scripts/stage_root.gd", "StageRoot", "Node3D", "res://scenes/stage.tscn")
	_write("res://scripts/main_menu.gd", "MainMenu", "Control", "res://scenes/main_menu.tscn")
	_write("res://tests/sim_run.gd", "Sim", "Node", "res://scenes/sim.tscn")
	quit(0)


func _write(script_path: String, node_name: String, type: String, out_path: String) -> void:
	var text := "[gd_scene load_steps=2 format=3]\n\n"
	text += '[ext_resource type="Script" path="%s" id="1_s"]\n\n' % script_path
	text += '[node name="%s" type="%s"]\n' % [node_name, type]
	text += 'script = ExtResource("1_s")\n'
	var f := FileAccess.open(out_path, FileAccess.WRITE)
	f.store_string(text)
	f.close()
	print("wrote %s" % out_path)
