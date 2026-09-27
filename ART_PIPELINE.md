# UltraRealFPS — Art Foundation Pipeline

## Objectif
Cette branche quitte le blockout pur. Les fichiers dans `ArtSource/Industrial` sont de vrais meshes glTF/GLB generes pour le projet, importes automatiquement dans Unreal Engine 5.8.

## Installation automatique
1. Extraire le ZIP complet.
2. Lancer `BUILD_AND_RUN.bat`.
3. Apres la compilation C++, si le kit n'a jamais ete importe, `IMPORT_ART_ASSETS.bat` est appele automatiquement.
4. Unreal cree les `.uasset` dans `Content/Environment/Industrial`.
5. L'editeur se lance ensuite et `URFPSGameMode` charge les nouveaux meshes.

Pour forcer une reimportation apres modification des fichiers GLB, lancer directement :
```bat
IMPORT_ART_ASSETS.bat
```

## Kit inclus
- SM_Container20_A
- SM_ConcreteBarrier_A
- SM_IndustrialCrate_A
- SM_Pallet_A
- SM_ElectricalCabinet_A
- SM_HVAC_Rooftop_A
- SM_PipeRack_Module_A
- SM_LoadingBay_A
- SM_LampPost_A
- SM_FuelTank_A
- SM_StorageRack_A

## Git / LFS
Les `.glb` et `.uasset` sont des binaires. Le depot contient deja les regles Git LFS pour ces extensions. Avant `PUBLISH_TO_GITHUB.bat`, Git LFS doit etre installe.

## Extension Fab recommandee
Pour la prochaine passe, les deux sources gratuites prioritaires sont :
- Quixel Megascans — Warehouse (pack gratuit, 85 assets, FBX/textures) ;
- Factory Environment Collection (pack Unreal gratuit avec sections usine, stockage, bureaux, props, FX).

Ces packs ne sont pas redistribues dans ce depot : leur acquisition passe par Fab et l'EULA correspondante.
