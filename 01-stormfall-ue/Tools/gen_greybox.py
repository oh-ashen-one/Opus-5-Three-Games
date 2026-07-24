"""Generate the movement-tuning grey box map.

    UnrealEditor-Cmd Stormfall.uproject -run=pythonscript -script=Tools/gen_greybox.py

Deliberately ugly and deliberately parametric: this map exists to answer "does
sprinting, crouching, jumping and falling feel good" before a single POI is built.
It ships ledges at known heights so fall damage can be checked against real drops.
"""

import unreal

MAP_PATH = "/Game/Maps/Lvl_Greybox"
CUBE = "/Engine/BasicShapes/Cube.Cube"

# Height (cm) -> what it is for. A 90cm capsule falls ~1200cm/s after roughly 7.5m,
# which is where fall damage starts, so these bracket the safe/lethal window.
LEDGES = [
    (200.0, "step"),
    (600.0, "safe drop"),
    (900.0, "threshold"),
    (1400.0, "hurts"),
    (2200.0, "lethal"),
]


def spawn_box(label, location, scale):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, location, unreal.Rotator(0, 0, 0)
    )
    actor.set_actor_label(label)
    actor.set_actor_scale3d(scale)
    mesh = unreal.EditorAssetLibrary.load_asset(CUBE)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    return actor


def main():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.new_level(MAP_PATH)

    # Ground: 200m x 200m plate.
    spawn_box("Ground", unreal.Vector(0, 0, -50), unreal.Vector(200, 200, 1))

    # A sprint lane with gaps, to feel out jump distance at each speed.
    for i in range(6):
        spawn_box(
            "SprintLane_%d" % i,
            unreal.Vector(-4000 + i * 1400, -3000, 0),
            unreal.Vector(8, 4, 1),
        )

    # Drop-test ledges at known heights, spaced so you can walk off each one.
    for i, (height, name) in enumerate(LEDGES):
        x = -4000 + i * 2000
        spawn_box(
            "Ledge_%dcm_%s" % (int(height), name.replace(" ", "_")),
            unreal.Vector(x, 2500, height * 0.5 - 50),
            unreal.Vector(6, 6, height / 100.0),
        )

    # A crouch tunnel: two blocks with a 110cm gap. Crouched half-height is 52.
    spawn_box("CrouchTunnel_Roof", unreal.Vector(2500, 0, 160), unreal.Vector(12, 6, 1))
    spawn_box("CrouchTunnel_L", unreal.Vector(2500, -350, 55), unreal.Vector(12, 1, 2))
    spawn_box("CrouchTunnel_R", unreal.Vector(2500, 350, 55), unreal.Vector(12, 1, 2))

    # A ramp of steps, for mantle work later.
    for i in range(8):
        spawn_box(
            "Step_%d" % i,
            unreal.Vector(-2500, 0, 25 + i * 50),
            unreal.Vector(2, 6, 1),
        )

    unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0, 0, 150), unreal.Rotator(0, 0, 0)
    ).set_actor_label("PlayerStart")

    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(-42, -35, 0)
    )
    sun.set_actor_label("Sun")
    unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    ).set_actor_label("SkyAtmosphere")
    unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 800), unreal.Rotator(0, 0, 0)
    ).set_actor_label("SkyLight")

    les.save_current_level()

    count = len(unreal.EditorLevelLibrary.get_all_level_actors())
    with open("/tmp/sf_greybox_out.txt", "w") as fh:
        fh.write("map: %s\nactors: %d\n" % (MAP_PATH, count))


main()
