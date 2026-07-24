extends SceneTree

func _init() -> void:
	for path in ["res://scenes/match.tscn", "res://scenes/main_menu.tscn", "res://scenes/sim.tscn"]:
		var packed = load(path)
		if packed == null:
			print("LOAD FAILED  %s" % path)
			continue
		var node = packed.instantiate()
		var s = node.get_script()
		print("%s  script=%s" % [path, "YES " + str(s.resource_path) if s else "MISSING"])
		node.free()
	quit(0)
