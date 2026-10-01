# UltraRealFPS — Industrial Visual Rebuild Plan

## Goal
Replace the current blockout-first look with a coherent tactical industrial compound that remains readable for combat and can be upgraded progressively with production assets.

## Current diagnosis
- Gameplay foundation is already functional.
- `URFPSGameMode.cpp` still builds most of the world procedurally from Engine basic shapes.
- Runtime blockout materials are derived from `BasicShapeMaterial` and currently vary mainly by color.
- Imported industrial GLB assets exist, but they are still a small layer on top of the blockout instead of the visual foundation.
- The current map has too much empty horizontal space and too many large featureless dark surfaces.

## Production order
1. Layout cleanup and visual hierarchy.
2. Zone-based environment composition.
3. Master material + instances.
4. Imported modular architecture.
5. Prop clusters and medium-frequency detail.
6. Lighting and exposure rebalance.
7. Repetition optimization with ISM/HISM.
8. PCG only for secondary dressing after art direction is stable.

Progress (Build 1, branch `claude/upbeat-knuth-kuzsu8`):
- step 1: geometry errors of the current layout fixed (closed rooms, grounded and supported structures, perimeter corners);
- step 3: master material implemented without textures, one instance per blockout style (see `MATERIAL_PIPELINE.md`).

## Zone layout

### Zone A — Spawn / staging
Purpose: clean orientation, low visual noise, immediate landmark.
- Keep clear player spawn cone.
- One operations canopy / checkpoint.
- Concrete barriers and two prop clusters.
- Strong sightline toward central yard.
- No random props inside the first navigation corridor.

### Zone B — Central yard
Purpose: main combat arena.
- 3 major cover islands rather than many isolated blocks.
- 1 central landmark.
- Mixed low / medium / tall cover.
- Dark asphalt path crossing lighter concrete apron.
- Distinct utility lane on one side.

### Zone C — Logistics lane
Purpose: medium-range tactical lane.
- Shipping containers, loading bays, pallets, storage racks.
- Alternating cover offsets instead of straight symmetric rows.
- Visual rhythm every 6–10 meters.
- One elevated or framed end landmark.

### Zone D — Maintenance / utilities
Purpose: visually dense technical side.
- Pipe rack, HVAC, electrical cabinets, tanks, fencing.
- Narrower sightlines.
- Warm utility lights in shadowed areas.
- Strong steel / painted-metal identity.

## Composition rules
- Macro: one recognisable silhouette every 20–30 m.
- Medium: one coherent prop cluster every 8–12 m.
- Small: details only after macro/medium composition works.
- Never scatter single props randomly without a functional story.
- Avoid perfectly parallel cover rows.
- Keep at least one clean route in every combat zone.
- Use vertical hierarchy: ground props < cover < architecture < skyline landmark.

## Asset priority
### Tier 1 — Must replace blockout first
- Warehouse facade modules
- CQB facade / doorway modules
- Concrete barriers
- Containers
- Industrial fencing/gates
- Loading bay modules
- Pipe racks
- Large tanks

### Tier 2 — Medium visual density
- Pallets
- Crates
- Racks
- Electrical cabinets
- HVAC
- Lamps
- Cable trays
- Bollards

### Tier 3 — Polish
- Decals
- Dirt
- Drips
- Oil stains
- Ground markings
- Warning signage
- Small cables and fasteners

## Acceptance criteria
A build is not accepted as a visual upgrade unless:
- the difference is obvious from the spawn screenshot;
- no large wall reads as a plain black rectangle;
- at least three zones are visually distinct;
- no important prop floats or intersects visibly;
- the central yard remains readable for combat;
- FPS does not regress abnormally from the previous validated build.
