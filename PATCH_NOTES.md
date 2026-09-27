# UltraRealFPS — Patch Notes

## Hotfix import UE 5.8 — 2026-09-27
- compilation C++ validee sur la machine de test ;
- correction de la validation Interchange qui cherchait une sentinelle a un chemin trop strict ;
- normalisation automatique des Static Mesh vers `/Game/Environment/Industrial/<NomAsset>` ;
- validation basee sur `AssetImportTask.get_objects()` et `imported_object_paths` ;
- collisions migrees vers `StaticMeshEditorSubsystem` avec fallback de compatibilite ;
- diagnostics des chemins reellement importes en cas de nouvel echec.

## Art Foundation Industrial Kit — Build de test

**Branche GitHub :** `dev/art-foundation-industrial-kit`  
**Étape roadmap :** ÉTAPE 2 — sortie du blockout / vrais meshes  
**Statut :** à compiler et tester sous Unreal Engine 5.8 avant merge.

### Vrai changement de pipeline
- ajout d'un dossier `ArtSource/Industrial` contenant 11 vrais modèles GLB dédiés au projet ;
- import automatique dans `/Game/Environment/Industrial` via Unreal Python + Interchange ;
- `BUILD_AND_RUN.bat` déclenche l'import au premier build réussi ;
- fallback blockout conservé si les assets ne sont pas encore importés.

### Nouveaux modèles
- conteneur 20 pieds détaillé ;
- barrière béton type Jersey ;
- caisse industrielle ;
- palette bois ;
- armoire électrique ;
- bloc HVAC de toiture ;
- module de pipe-rack ;
- baie de chargement industrielle ;
- lampadaire ;
- réservoir horizontal ;
- rack de stockage.

### Intégration dans le gameplay
- quatre baies de chargement utilisent les nouveaux meshes si disponibles ;
- conteneurs empilés dans la cour Sud ;
- nouvelles barrières béton autour des routes centrales ;
- pipe-racks réels dans la zone technique ;
- deux réservoirs détaillés côté Est ;
- HVAC détaillés sur toiture ;
- armoires électriques, racks, palettes et caisses ajoutés dans les zones de service ;
- lampadaires importés utilisés comme silhouettes visuelles.

### Technique
- les meshes GLB utilisent un workflow PBR simple (metallic/roughness) ;
- Unreal 5.8 importe GLB via Interchange ;
- les props simples reçoivent une collision boîte automatiquement ;
- les modèles décoratifs complexes restent sans collision quand une collision approximative gênerait le passage ;
- publication GitHub bloquée si Git LFS manque alors que les assets binaires sont présents.

### À tester en priorité
1. Premier lancement : vérifier que l'import Art Foundation s'exécute après la compilation.
2. Vérifier la présence de `Content/Environment/Industrial/SM_Container20_A.uasset`.
3. Dans le jeu : confirmer que les nouveaux conteneurs, racks, réservoirs, HVAC et props sont visibles.
4. Vérifier qu'aucune collision invisible ne bloque les passages principaux.
5. Vérifier les FPS dans l'entrepôt et la cour Sud.
6. Tirer sur conteneurs/barrières et vérifier que les surfaces physiques restent cohérentes.
7. Relancer le jeu : l'import ne doit pas se refaire tant que les assets existent.

### Limites
- ce kit est une première fondation art interne, pas encore un pack AAA photogrammétrique ;
- les textures haute résolution et decals PBR viendront dans la passe suivante ;
- les assets gratuits Fab/Quixel seront intégrés séparément après acquisition/licence par le compte utilisateur.
