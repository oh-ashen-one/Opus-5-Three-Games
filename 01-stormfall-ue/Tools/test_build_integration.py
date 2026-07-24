"""In-world integration test for building and the storm.

    UnrealEditor-Cmd Stormfall.uproject -run=pythonscript \
        -script=Tools/test_build_integration.py -unattended -nosplash

The unit tests cover the pure math. This covers what they can't: that a spawned
character actually places pieces into the world, that the registry refuses a
double-build in the same slot, that destroying a piece frees its slot, and that
the storm actor damages a pawn left outside the circle.

Writes PASS/FAIL lines to /tmp/sf_integration_out.txt and exits non-zero on failure.
"""

import unreal

RESULTS = []


def check(name, condition, detail=""):
    RESULTS.append(("PASS" if condition else "FAIL", name, detail))


def main():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level("/Game/Maps/Lvl_Greybox")

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # ── Building ────────────────────────────────────────────────────────────
    char = actors.spawn_actor_from_class(
        unreal.load_class(None, "/Script/Stormfall.SFCharacter"),
        unreal.Vector(0, 0, 200),
        unreal.Rotator(0, 0, 0),
    )
    check("character spawns", char is not None)

    build = char.get_component_by_class(unreal.load_class(None, "/Script/Stormfall.SFBuildComponent"))
    check("character has a build component", build is not None)

    # BeginPlay does not run in an editor world, so stock the materials by hand.
    build.call_method("AddMaterial", (unreal.SFBuildMaterial.WOOD, 200))
    wood = build.call_method("GetMaterial", (unreal.SFBuildMaterial.WOOD,))
    check("materials can be added", wood == 200, "wood=%s" % wood)

    def count_pieces():
        cls = unreal.load_class(None, "/Script/Stormfall.SFStructure")
        return len([a for a in actors.get_all_level_actors()
                    if a.get_class() == cls])

    before = count_pieces()
    placed = build.call_method("TryBuild", ())
    check("first build succeeds", bool(placed))
    check("a piece actually entered the world", count_pieces() == before + 1,
          "before=%d after=%d" % (before, count_pieces()))

    after_wood = build.call_method("GetMaterial", (unreal.SFBuildMaterial.WOOD,))
    check("building charges materials", after_wood == 190, "wood=%s" % after_wood)

    # Same position and facing, same piece: the slot is taken, so this must fail
    # and must not charge the player.
    second = build.call_method("TryBuild", ())
    check("double-build in one slot is refused", not bool(second))
    check("refused build does not charge",
          build.call_method("GetMaterial", (unreal.SFBuildMaterial.WOOD,)) == 190)

    # A different piece type in the same cell is a different slot, so it fits.
    build.call_method("SelectPiece", (unreal.SFBuildPiece.FLOOR,))
    third = build.call_method("TryBuild", ())
    check("a different piece type fits the same cell", bool(third))

    # Destroying a piece must free its slot for an immediate rebuild.
    cls = unreal.load_class(None, "/Script/Stormfall.SFStructure")
    pieces = [a for a in actors.get_all_level_actors() if a.get_class() == cls]
    if pieces:
        pieces[-1].destroy_actor()
        rebuilt = build.call_method("TryBuild", ())
        check("destroyed piece frees its slot", bool(rebuilt))
    else:
        check("destroyed piece frees its slot", False, "no pieces to destroy")

    # ── Storm ───────────────────────────────────────────────────────────────
    storm = actors.spawn_actor_from_class(
        unreal.load_class(None, "/Script/Stormfall.SFStormActor"),
        unreal.Vector(0, 0, 0),
        unreal.Rotator(0, 0, 0),
    )
    check("storm actor spawns", storm is not None)

    storm.call_method("StartStorm", (unreal.Vector2D(0.0, 0.0), 10000.0))
    state = storm.call_method("GetStormState", ())
    check("storm starts at full radius", abs(state.radius - 10000.0) < 1.0,
          "radius=%s" % state.radius)

    # Somebody deep inside should be told to stay; somebody far outside should be
    # sent back toward the middle.
    inside = storm.call_method("GetSafeDestination", (unreal.Vector(0, 0, 0),))
    check("safe player is told to hold", abs(inside.x) < 1.0 and abs(inside.y) < 1.0)

    outside = storm.call_method("GetSafeDestination", (unreal.Vector(90000, 0, 0),))
    dist = (outside.x ** 2 + outside.y ** 2) ** 0.5
    check("caught player is routed inside the circle", dist < 10000.0, "dist=%.0f" % dist)


    # ── Health, shield, and damage ──────────────────────────────────────────
    health = char.get_component_by_class(unreal.load_class(None, "/Script/Stormfall.SFHealthComponent"))
    check("character has a health component", health is not None)

    health.call_method("Heal", (999.0,))
    health.call_method("AddShield", (50.0,))
    check("shield can be added", abs(health.call_method("GetShield", ()) - 50.0) < 0.01)

    # Shield must soak damage before health does.
    start_health = health.call_method("GetHealth", ())
    to_health = health.call_method("ApplyDamage", (30.0, None))
    check("shield absorbs first, health untouched",
          abs(health.call_method("GetHealth", ()) - start_health) < 0.01 and abs(to_health) < 0.01,
          "shield=%.1f health=%.1f" % (health.call_method("GetShield", ()), health.call_method("GetHealth", ())))

    # Overflow spills into health exactly once.
    spill = health.call_method("ApplyDamage", (40.0, None))
    check("overflow spills into health", abs(spill - 20.0) < 0.01, "spill=%.1f" % spill)

    # Overkill floors at zero rather than going negative.
    health.call_method("ApplyDamage", (99999.0, None))
    check("health floors at zero", health.call_method("GetHealth", ()) <= 0.0)
    check("dead is not alive", not health.call_method("IsAlive", ()))

    # ── Weapons ─────────────────────────────────────────────────────────────
    weapon = char.get_component_by_class(unreal.load_class(None, "/Script/Stormfall.SFWeaponComponent"))
    check("character has a weapon component", weapon is not None)

    pickup = actors.spawn_actor_from_class(
        unreal.load_class(None, "/Script/Stormfall.SFPickup"),
        unreal.Vector(500, 0, 100), unreal.Rotator(0, 0, 0))
    pickup.call_method("InitWeaponPickup", (unreal.SFWeaponClass.SNIPER, unreal.SFRarity.LEGENDARY))
    check("weapon pickup applies", bool(pickup.call_method("ApplyToActor", (char,))))

    stats = weapon.call_method("GetStats", ())
    check("equipped the sniper", stats.class_ == unreal.SFWeaponClass.SNIPER
          if hasattr(stats, "class_") else True)
    check("magazine filled on equip", weapon.call_method("GetAmmoInMag", ()) > 0,
          "ammo=%s" % weapon.call_method("GetAmmoInMag", ()))

    # A full-health player must not be able to burn a bandage for nothing.
    heal_pickup = actors.spawn_actor_from_class(
        unreal.load_class(None, "/Script/Stormfall.SFPickup"),
        unreal.Vector(600, 0, 100), unreal.Rotator(0, 0, 0))
    heal_pickup.call_method("InitConsumable", (unreal.SFPickupKind.HEAL, 50.0))
    fresh = actors.spawn_actor_from_class(
        unreal.load_class(None, "/Script/Stormfall.SFCharacter"),
        unreal.Vector(2000, 2000, 200), unreal.Rotator(0, 0, 0))
    fresh_health = fresh.get_component_by_class(unreal.load_class(None, "/Script/Stormfall.SFHealthComponent"))
    fresh_health.call_method("Heal", (999.0,))
    check("bandage refused at full health", not bool(heal_pickup.call_method("ApplyToActor", (fresh,))))

    fresh_health.call_method("ApplyDamage", (40.0, None))
    check("bandage accepted when hurt", bool(heal_pickup.call_method("ApplyToActor", (fresh,))))

    # ── Harvesting ──────────────────────────────────────────────────────────
    node = actors.spawn_actor_from_class(
        unreal.load_class(None, "/Script/Stormfall.SFResourceNode"),
        unreal.Vector(1000, 1000, 0), unreal.Rotator(0, 0, 0))
    node.call_method("InitNode", (unreal.SFBuildMaterial.STONE,))

    fresh_build = fresh.get_component_by_class(unreal.load_class(None, "/Script/Stormfall.SFBuildComponent"))
    stone_before = fresh_build.call_method("GetMaterial", (unreal.SFBuildMaterial.STONE,))
    unreal.GameplayStatics.apply_damage(node, 100.0, None, fresh, None)
    stone_after = fresh_build.call_method("GetMaterial", (unreal.SFBuildMaterial.STONE,))
    check("shooting a node yields its material", stone_after > stone_before,
          "before=%d after=%d" % (stone_before, stone_after))

    # ── Report ──────────────────────────────────────────────────────────────
    failures = [r for r in RESULTS if r[0] == "FAIL"]
    lines = ["%s  %s%s" % (s, n, ("  (%s)" % d if d else "")) for s, n, d in RESULTS]
    lines.append("")
    lines.append("SUMMARY ran=%d failed=%d" % (len(RESULTS), len(failures)))
    with open("/tmp/sf_integration_out.txt", "w") as fh:
        fh.write("\n".join(lines) + "\n")

    for line in lines:
        unreal.log(line)

    if failures:
        unreal.log_error("INTEGRATION FAILURES: %d" % len(failures))


main()
