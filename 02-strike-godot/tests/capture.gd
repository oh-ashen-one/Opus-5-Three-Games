extends Node

## Renders real frames and measures real framerate.
##
## Everything until now was headless: no rendering, no FPS. This runs the game
## with an actual renderer, samples Engine.get_frames_per_second(), and saves
## PNGs of the viewport so the game can be *looked at* rather than only asserted.

const WARMUP := 3.0          ## Let shaders compile before measuring.
const SAMPLE_SECONDS := 12.0
const SHOT_TIMES := [4.0, 8.0, 13.0]

var scene_path := "res://scenes/match.tscn"
var out_prefix := "strike"

var _scene: Node
var _elapsed := 0.0
var _fps: Array[float] = []
var _shots_taken := 0
var _done := false


func _ready() -> void:
	var packed := load(scene_path)
	_scene = packed.instantiate()
	get_tree().root.add_child.call_deferred(_scene)


func _process(delta: float) -> void:
	if _done:
		return
	_elapsed += delta

	if _elapsed > WARMUP:
		_fps.append(Engine.get_frames_per_second())

	if _shots_taken < SHOT_TIMES.size() and _elapsed >= SHOT_TIMES[_shots_taken]:
		_capture(_shots_taken)
		_shots_taken += 1

	if _elapsed >= WARMUP + SAMPLE_SECONDS:
		_finish()


func _capture(index: int) -> void:
	var img := get_viewport().get_texture().get_image()
	var path := "user://%s_%d.png" % [out_prefix, index]
	img.save_png(path)
	print("saved %s (%dx%d)" % [ProjectSettings.globalize_path(path),
			img.get_width(), img.get_height()])


func _finish() -> void:
	_done = true
	var total := 0.0
	var lowest := 99999.0
	for f in _fps:
		total += f
		lowest = minf(lowest, f)
	var mean := total / maxf(float(_fps.size()), 1.0)

	print("")
	print("FPS samples=%d  mean=%.1f  min=%.1f" % [_fps.size(), mean, lowest])
	print("CAPTURE DONE")
	get_tree().quit(0)
