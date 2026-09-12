---
name: code-object-hierarchy
description: CODE/ base class hierarchy and concrete unit/building/object type classes
metadata:
  type: project_state
---

# CODE/ — Object Class Hierarchy

Parent index: [summary.md](summary.md).

## Core base classes (read these first)
Single-inheritance chain, each adding one layer of responsibility:
- `ABSTRACT.CPP`/`.H` — `AbstractClass`: root base for anything with a map coordinate/height/RTTI id; `Direction()`/`Distance()` helpers.
- `OBJECT.CPP`/`.H` — `ObjectClass : AbstractClass`: placement/limbo/damage-list/display-flag state common to all map objects.
- `MISSION.CPP`/`.H` — `MissionClass : ObjectClass`: the `MissionType` order/AI-strategy state machine (guard, attack, hunt, etc).
- `RADIO.CPP`/`.H` — `RadioClass : MissionClass`: inter-object "radio" messaging (command/escort/loading coordination).
- `TECHNO.CPP`/`.H` — `TechnoClass : RadioClass, FlasherClass, StageClass, CargoClass, DoorClass`: shared data for anything "technological" (units + buildings) — health, cloak, ownership, veterancy.
- `FOOT.CPP`/`.H` — `FootClass : TechnoClass`: shared behavior for anything that moves — path following, formation/team membership, scatter logic.
- `TARGET.CPP`/`.H` — opaque `TARGET` handle type + conversion helpers for referring to any targetable object/cell without a raw pointer (used for save-game safety and inter-object references).
- `TARCOM.CPP`/`.H` — `TarComClass`: target-tracking mixin layered under `FootClass`/turret logic.
- `DOOR.CPP`/`.H`, `CARGO.CPP`/`.H`, `CREW.CPP`/`.H`, `FLASHER.CPP`/`.H`, `STAGE.H` — small mixin classes composed into `TechnoClass` (bay doors, transported cargo, crew survivors, damage-flash effect, animation-stage timing).
- `DRIVE.CPP`/`.H`, `FLY.CPP`/`.H` — ground-vehicle and aircraft movement-physics mixins used by `UnitClass`/`AircraftClass`.
- `TURRET.CPP`/`.H` — turret rotation/facing logic shared by turreted units and buildings.

## Concrete object/unit type classes
Each has an instance class (`XxxClass`) and a static "type" class (`XxxTypeClass`) describing shared rules-data (cost, armor, art). `TYPE.H` declares the shared `ObjectTypeClass`/`TechnoTypeClass` bases.
- `UNIT.CPP`/`.H` — `UnitClass : FootClass`: ground vehicles (tanks, harvesters, MCV, etc).
- `INFANTRY.CPP`/`.H` — `InfantryClass : FootClass`: foot soldiers, includes squeeze/kill-under-tread logic.
- `AIRCRAFT.CPP`/`.H` — `AircraftClass : FootClass`: helicopters/planes.
- `VESSEL.CPP`/`.H` — `VesselClass : FootClass`: naval units.
- `BUILDING.CPP`/`.H` — `BuildingClass : TechnoClass`: structures, production, power/repair logic.
- `BULLET.CPP`/`.H` — `BulletClass : ObjectClass`: projectiles in flight.
- `ANIM.CPP`/`.H` (+ stale `.CPP.BAK`) — `AnimClass`: shape-animation overlay objects (explosions, fire).
- `TERRAIN.CPP`/`.H` — `TerrainClass`: static destructible terrain decoration (trees, rocks).
- `TEMPLATE.CPP`/`.H` — `TemplateClass`: ground tile "template" (large multi-cell terrain graphic) placement.
- `OVERLAY.CPP`/`.H` — `OverlayClass`: cell overlays (ore/gems, walls, fences, crates-on-ground).
- `SMUDGE.CPP`/`.H` — `SmudgeClass`: scorch marks/craters/bib graphics left on cells.
- `CRATE.CPP`/`.H` — `CrateClass`: pickup-crate spawn/effect logic.
- `WARHEAD.CPP`/`.H`, `WEAPON.CPP`/`.H` — data-driven damage-type (`WarheadTypeClass`) and weapon (`WeaponTypeClass`) definitions.
- `COMBAT.CPP` — damage-application/explosion routine (`Explosion_Damage`, `Modify_Damage`) tying warhead+armor+target together.
- `CREW.CPP`/`.H` — determines surviving crew ejected when a vehicle/building is destroyed.
- `FUSE.CPP`/`.H` — proximity/contact fuse logic for bullets/mines.

## The `*DATA.CPP` type-class family
Static type-class implementations (constructors, `As_Reference`, `.INI` parsing) for each RTTI kind — one file per object category, all following the same pattern:
`AADATA.CPP`→`AircraftTypeClass`, `ADATA.CPP`→`AnimTypeClass`, `BBDATA.CPP`→`BulletTypeClass`, `BDATA.CPP`→`BuildingTypeClass`, `CDATA.CPP`→`TemplateTypeClass`, `HDATA.CPP`→`HouseTypeClass`, `IDATA.CPP`→`InfantryTypeClass`, `ODATA.CPP`→`OverlayTypeClass`, `SDATA.CPP`→`SmudgeTypeClass`, `TDATA.CPP`→`TerrainTypeClass`, `UDATA.CPP`→`UnitTypeClass`, `VDATA.CPP`→`VesselTypeClass`.

## House / player logic
- `HOUSE.CPP`/`.H` — `HouseClass`/`HouseStaticClass`: represents one "country"/player-slot (human or AI) — resources, tech tree flags, diplomacy (ally/enemy), AI difficulty bias. `HouseStaticClass` holds scenario-configured country data (IQ level, starting credits, etc).
