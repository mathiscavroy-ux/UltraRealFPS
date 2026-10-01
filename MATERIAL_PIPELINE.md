# UltraRealFPS — Environment Material Pipeline

## Folder structure
`Content/UltraRealFPS/Art/Materials/Environment/Industrial/`

Planned assets:
- M_Master_IndustrialSurface
- MI_Concrete_Clean
- MI_Concrete_Dirty
- MI_Concrete_Stained
- MI_Asphalt_Dusty
- MI_PaintedMetal_Blue
- MI_PaintedMetal_Green
- MI_PaintedMetal_Rusty
- MI_BareSteel
- MI_WornSteel
- MI_Wood_Industrial
- MI_SafetyYellow

## Master material parameters
### Texture parameters
- BaseColorTexture
- NormalTexture
- ORMTexture
- DetailNormalTexture
- GrungeTexture

### Scalar parameters
- UVScale
- RoughnessMultiplier
- MetallicMultiplier
- NormalStrength
- AOIntensity
- DirtAmount
- DirtContrast
- EdgeWearAmount
- DetailNormalStrength

### Vector parameters
- BaseColorTint
- DirtTint
- PaintTint

## Rules
- Prefer one parent material with instance variation.
- Keep static switches limited.
- Use constant material instances for environment surfaces.
- Reserve dynamic instances for gameplay-driven changes only.
- Do not create a unique shader for every prop.
- Physical material assignment must remain consistent with ballistic surface type.

## First implementation target
The first pass only needs:
1. concrete;
2. asphalt;
3. painted steel;
4. bare steel;
5. wood;
6. safety paint.

These six families are enough to make the map stop reading as one grey/black blockout.

## Current implementation (Industrial Visual Rebuild — Build 1)
`M_Master_IndustrialSurface` exists as a generated asset:
- source: `Tools/build_environment_materials.py` (Unreal Python, MaterialEditingLibrary);
- run by `BUILD_MATERIALS.bat`, which `BUILD_AND_RUN.bat` calls after the build until `Saved/EnvironmentMaterials.ok` exists;
- a failed run deletes the half-built asset, so the game never loads a broken master.

This first pass uses no textures. The blockout is made of scaled engine shapes whose UVs stretch with every block, so all variation is computed in world space.

### Parameters
| Parameter | Type | Default | Role |
|---|---|---|---|
| `BaseColorTint` | vector | (0.150, 0.155, 0.160) | surface colour (linear) |
| `Roughness` | scalar | 0.85 | base roughness |
| `Metallic` | scalar | 0.0 | metallic response |
| `DirtTint` | vector | (0.55, 0.50, 0.44) | colour multiplier where dirt applies |
| `DirtAmount` | scalar | 0.35 | dirt strength at the foot of a surface |
| `DirtHeight` | scalar | 90 | height of the dirt band, cm |
| `GroundHeight` | scalar | -100 | Z of the ground slab |
| `MacroVariation` | scalar | 0.12 | tonal breakup, ± fraction of the tint |
| `MacroScale` | scalar | 400 | size of the breakup pattern, cm |

### Graph
- `Breakup = lerp(1 - MacroVariation, 1 + MacroVariation, Noise(WorldPosition / MacroScale))`
- `DirtMask = (1 - saturate((WorldZ - GroundHeight) / DirtHeight)) * DirtAmount`
- `BaseColor = lerp(BaseColorTint * Breakup, BaseColorTint * Breakup * DirtTint, DirtMask)`
- `Roughness = saturate(Roughness + DirtMask * 0.12)`
- `Metallic = Metallic`

### Style mapping
`AURFPSGameMode::CreateMaterials` creates one instance per blockout style and shares it between every block of that style. The ground styles get no dirt band because they are the base.

| Style | Family | BaseColorTint | Rough. | Metal. | Dirt | Macro | Scale |
|---|---|---|---|---|---|---|---|
| Floor | concrete slab | (0.075, 0.078, 0.082) | 0.92 | 0.00 | 0.00 | 0.16 | 900 |
| Wall | concrete | (0.150, 0.155, 0.160) | 0.88 | 0.00 | 0.55 | 0.12 | 350 |
| Cover | concrete | (0.190, 0.190, 0.185) | 0.85 | 0.00 | 0.45 | 0.10 | 250 |
| ConcreteLight | clean concrete | (0.235, 0.235, 0.220) | 0.82 | 0.00 | 0.25 | 0.08 | 500 |
| Asphalt | asphalt | (0.028, 0.030, 0.033) | 0.95 | 0.00 | 0.00 | 0.22 | 1200 |
| Metal | bare steel | (0.075, 0.080, 0.086) | 0.48 | 0.20 | 0.30 | 0.06 | 300 |
| Dark | dark graphite: perimeter, rooms, platforms | (0.060, 0.066, 0.074) | 0.70 | 0.10 | 0.40 | 0.08 | 500 |
| PaintBlue | painted steel, navy | (0.035, 0.070, 0.115) | 0.55 | 0.00 | 0.30 | 0.06 | 300 |
| Wood | wood | (0.170, 0.105, 0.055) | 0.80 | 0.00 | 0.30 | 0.14 | 120 |
| Accent | utility orange | (0.360, 0.130, 0.045) | 0.60 | 0.00 | 0.35 | 0.08 | 250 |
| Hazard | safety amber | (0.520, 0.360, 0.050) | 0.62 | 0.00 | 0.25 | 0.10 | 250 |
| Ammo / Medical / Grenade | gameplay colours | unchanged | 0.60 | 0.00 | 0.00 | 0.00 | 300 |

Without the master asset the same colours go to the `Color` parameter of `BasicShapeMaterial`, so both paths keep the same palette.

Deviation from the rules above: the instances are still dynamic, created at runtime. The blockout styles live in C++, so there is nothing to assign constant `MI_` assets to yet. There are 14 shared instances in total, not one per block. They become `MI_` assets when imported meshes take over the surfaces.

### Regenerate / disable
- regenerate after editing the script: run `BUILD_MATERIALS.bat`. Instances pick up the new graph at the next launch;
- disable: delete `Content/UltraRealFPS/Art/Materials` and keep `Saved/EnvironmentMaterials.ok`. The game falls back to `BasicShapeMaterial`;
- re-enable: run `BUILD_MATERIALS.bat`, or delete `Saved/EnvironmentMaterials.ok` so that `BUILD_AND_RUN.bat` regenerates it.

### Next
- texture parameters (`BaseColorTexture`, `NormalTexture`, `ORMTexture`, `GrungeTexture`) once real meshes with usable UVs replace the blockout;
- constant `MI_*` assets from the planned list, one per family;
- edge wear needs mesh data (vertex colour or curvature) that the engine cubes do not have.
