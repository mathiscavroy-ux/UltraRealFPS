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
