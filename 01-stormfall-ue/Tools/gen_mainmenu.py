"""Generate the STORMFALL main-menu level.

    UnrealEditor-Cmd Stormfall.uproject -run=pythonscript -script=Tools/gen_mainmenu.py

A quiet backdrop for the menu. The HUD switches to its MainMenu screen when the
level name contains "Menu", and the game mode skips spawning a match here.
"""

import unreal

MAP_PATH = "/Game/Maps/Lvl_MainMenu"
CUBE = "/Engine/BasicShapes/Cube.Cube"

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def box(label, location, scale, rotation=None):
    a = actors.spawn_actor_from_class(
        unreal.StaticMeshActor, location, rotation or unreal.Rotator(0, 0, 0))
    a.set_actor_label(label)
    a.set_actor_scale3d(scale)
    a.static_mesh_component.set_static_mesh(unreal.EditorAssetLibrary.load_asset(CUBE))
    a.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    return a


def main():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.new_level(MAP_PATH)

    box("Ground", unreal.Vector(0, 0, -50), unreal.Vector(120, 120, 1))
    # A few silhouettes so the backdrop reads as the island rather than a void.
    for i in range(9):
        box("Skyline_%d" % i,
            unreal.Vector(1800 + i * 340, -600 + (i % 3) * 500, 150 + (i % 4) * 120),
            unreal.Vector(3, 3, 3 + (i % 4) * 2))

    actors.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0, 0, 200), unreal.Rotator(0, 0, 0)
    ).set_actor_label("PlayerStart")

    sun = actors.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(-18, -60, 0))
    sun.set_actor_label("Sun")
    actors.spawn_actor_from_class(
        unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    ).set_actor_label("SkyAtmosphere")
    actors.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 800), unreal.Rotator(0, 0, 0)
    ).set_actor_label("SkyLight")
    actors.spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    ).set_actor_label("HeightFog")

    les.save_current_level()
    with open("/tmp/sf_menu_out.txt", "w") as fh:
        fh.write("menu map: %s\nactors: %d\n" % (MAP_PATH, len(actors.get_all_level_actors())))


main()
