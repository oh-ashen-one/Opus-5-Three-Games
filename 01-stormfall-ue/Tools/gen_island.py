"""Generate the STORMFALL island.

    UnrealEditor-Cmd Stormfall.uproject -run=pythonscript -script=Tools/gen_island.py

Deterministic: a fixed seed means the same island every run, so a level change is
a diff to this file rather than an unrepeatable accident. ~1km across with named
POIs, each with real interiors and verticality, plus scattered resource nodes and
ground loot.
"""

import math
import random

import unreal

MAP_PATH = "/Game/Maps/Lvl_Island"
CUBE = "/Engine/BasicShapes/Cube.Cube"
SEED = 20260724

MAP_RADIUS = 50000.0  # 500m -> ~1km across

# name, (x, y) in fractions of MAP_RADIUS, footprint, storeys
POIS = [
    ("Anchor Point",   (-0.45, -0.40), 3, 2),
    ("Kiln Row",       ( 0.40, -0.50), 4, 1),
    ("The Terraces",   ( 0.55,  0.25), 3, 3),
    ("Sunken Yard",    (-0.55,  0.35), 4, 2),
    ("Meridian Halt",  ( 0.05,  0.60), 3, 2),
    ("Old Quarry",     (-0.10, -0.65), 5, 1),
    ("Signal Hill",    ( 0.68, -0.10), 2, 4),
    ("Drift Market",   (-0.68,  0.00), 4, 2),
]

CELL = 400.0  # matches SFBuild::GridSize so structures line up with the world

rng = random.Random(SEED)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
spawned = {"buildings": 0, "nodes": 0, "loot": 0}


def box(label, location, scale, rotation=None):
    a = actors.spawn_actor_from_class(
        unreal.StaticMeshActor, location, rotation or unreal.Rotator(0, 0, 0))
    a.set_actor_label(label)
    a.set_actor_scale3d(scale)
    a.static_mesh_component.set_static_mesh(unreal.EditorAssetLibrary.load_asset(CUBE))
    a.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    return a


def building(name, cx, cy, width_cells, storeys):
    """A hollow box with a doorway gap and a roof — enough to fight inside."""
    w = width_cells * CELL
    h = 350.0
    for storey in range(storeys):
        z = storey * h
        # Floor
        box("%s_Floor%d" % (name, storey), unreal.Vector(cx, cy, z),
            unreal.Vector(w / 100.0, w / 100.0, 0.2))
        # Four walls, with a gap left in the south wall of the ground floor.
        half = w / 2.0
        for side, (dx, dy, sx, sy) in enumerate([
            ( 0,  half,  w / 100.0, 0.2),
            ( 0, -half,  w / 100.0, 0.2),
            ( half, 0,   0.2, w / 100.0),
            (-half, 0,   0.2, w / 100.0),
        ]):
            if storey == 0 and side == 1:
                # Doorway: two stubs instead of a solid wall.
                for sign in (-1, 1):
                    box("%s_Wall%d_%d_%s" % (name, storey, side, sign),
                        unreal.Vector(cx + dx + sign * w * 0.32, cy + dy, z + h / 2.0),
                        unreal.Vector(w * 0.36 / 100.0, 0.2, h / 100.0))
                continue
            box("%s_Wall%d_%d" % (name, storey, side),
                unreal.Vector(cx + dx, cy + dy, z + h / 2.0),
                unreal.Vector(sx if sx > 0.5 else 0.2, sy if sy > 0.5 else 0.2, h / 100.0))
        spawned["buildings"] += 1
    # Roof
    box("%s_Roof" % name, unreal.Vector(cx, cy, storeys * h),
        unreal.Vector(w / 100.0, w / 100.0, 0.2))
    # A ramp up the outside so the roof is reachable without building.
    box("%s_Ramp" % name,
        unreal.Vector(cx + w * 0.7, cy, storeys * h * 0.5),
        unreal.Vector(storeys * h * 1.5 / 100.0, w * 0.5 / 100.0, 0.2),
        unreal.Rotator(0, 40, 0))


def resource_node(material, location):
    node = actors.spawn_actor_from_class(
        unreal.load_class(None, "/Script/Stormfall.SFResourceNode"),
        location, unreal.Rotator(0, 0, 0))
    node.call_method("InitNode", (material,))
    spawned["nodes"] += 1


def loot(location):
    """Weighted rarity: Legendary must feel like a find, not a given."""
    roll = rng.random()
    pickup = actors.spawn_actor_from_class(
        unreal.load_class(None, "/Script/Stormfall.SFPickup"),
        location, unreal.Rotator(0, 0, 0))

    if roll < 0.18:
        pickup.call_method("InitConsumable", (unreal.SFPickupKind.SHIELD, 50.0))
    elif roll < 0.30:
        pickup.call_method("InitConsumable", (unreal.SFPickupKind.HEAL, 40.0))
    else:
        r = rng.random()
        rarity = (unreal.SFRarity.COMMON if r < 0.40 else
                  unreal.SFRarity.UNCOMMON if r < 0.68 else
                  unreal.SFRarity.RARE if r < 0.86 else
                  unreal.SFRarity.EPIC if r < 0.96 else
                  unreal.SFRarity.LEGENDARY)
        wclass = rng.choice([
            unreal.SFWeaponClass.ASSAULT_RIFLE, unreal.SFWeaponClass.ASSAULT_RIFLE,
            unreal.SFWeaponClass.SMG, unreal.SFWeaponClass.SHOTGUN,
            unreal.SFWeaponClass.PISTOL, unreal.SFWeaponClass.SNIPER,
        ])
        pickup.call_method("InitWeaponPickup", (wclass, rarity))
    spawned["loot"] += 1


def main():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.new_level(MAP_PATH)

    # Ground: a single large plate. Cheap, flat, and readable — terrain sculpting
    # would look better but costs far more than it adds to a 10-minute match.
    box("Ground", unreal.Vector(0, 0, -50), unreal.Vector(MAP_RADIUS * 2.2 / 100.0,
                                                          MAP_RADIUS * 2.2 / 100.0, 1))

    # Low hills for cover and sightline breaks.
    for i in range(26):
        ang = rng.uniform(0, math.tau)
        dist = rng.uniform(0.15, 0.92) * MAP_RADIUS
        box("Hill_%02d" % i,
            unreal.Vector(math.cos(ang) * dist, math.sin(ang) * dist, rng.uniform(100, 500)),
            unreal.Vector(rng.uniform(14, 34), rng.uniform(14, 34), rng.uniform(2, 8)))

    # POIs.
    for name, (fx, fy), width, storeys in POIS:
        cx, cy = fx * MAP_RADIUS, fy * MAP_RADIUS
        building(name.replace(" ", "_"), cx, cy, width, storeys)
        # Loot clusters at each POI — this is why you land here.
        for _ in range(rng.randint(5, 9)):
            loot(unreal.Vector(cx + rng.uniform(-1400, 1400),
                               cy + rng.uniform(-1400, 1400), 120))
        # Materials nearby so building is possible where fighting happens.
        for _ in range(rng.randint(6, 10)):
            mat = rng.choice([unreal.SFBuildMaterial.WOOD, unreal.SFBuildMaterial.WOOD,
                              unreal.SFBuildMaterial.STONE, unreal.SFBuildMaterial.METAL])
            resource_node(mat, unreal.Vector(cx + rng.uniform(-2600, 2600),
                                             cy + rng.uniform(-2600, 2600), 100))

    # Wilderness: scattered nodes and a thinner spread of loot between POIs.
    for i in range(220):
        ang = rng.uniform(0, math.tau)
        dist = rng.uniform(0.05, 0.95) * MAP_RADIUS
        pos = unreal.Vector(math.cos(ang) * dist, math.sin(ang) * dist, 100)
        mat = (unreal.SFBuildMaterial.WOOD if rng.random() < 0.6 else
               unreal.SFBuildMaterial.STONE if rng.random() < 0.75 else
               unreal.SFBuildMaterial.METAL)
        resource_node(mat, pos)

    for i in range(40):
        ang = rng.uniform(0, math.tau)
        dist = rng.uniform(0.05, 0.9) * MAP_RADIUS
        loot(unreal.Vector(math.cos(ang) * dist, math.sin(ang) * dist, 120))

    # Player start, storm, lighting.
    actors.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0, 0, 300), unreal.Rotator(0, 0, 0)
    ).set_actor_label("PlayerStart")

    actors.spawn_actor_from_class(
        unreal.load_class(None, "/Script/Stormfall.SFStormActor"),
        unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    ).set_actor_label("Storm")

    sun = actors.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 4000), unreal.Rotator(-38, -50, 0))
    sun.set_actor_label("Sun")
    actors.spawn_actor_from_class(
        unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    ).set_actor_label("SkyAtmosphere")
    actors.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 1500), unreal.Rotator(0, 0, 0)
    ).set_actor_label("SkyLight")
    actors.spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    ).set_actor_label("HeightFog")

    les.save_current_level()

    total = len(actors.get_all_level_actors())
    report = ("map: %s\nPOIs: %d\nbuilding storeys: %d\nresource nodes: %d\n"
              "loot pickups: %d\ntotal actors: %d\n"
              % (MAP_PATH, len(POIS), spawned["buildings"], spawned["nodes"],
                 spawned["loot"], total))
    with open("/tmp/sf_island_out.txt", "w") as fh:
        fh.write(report)
    unreal.log(report)


main()
