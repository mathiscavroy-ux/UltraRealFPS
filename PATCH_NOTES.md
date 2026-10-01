# UltraRealFPS — Patch Notes

## Industrial Visual Rebuild — Build 1 : corrections + matériaux

**Branche GitHub :** `claude/upbeat-knuth-kuzsu8` (part de `dev/industrial-visual-rebuild`)  
**Étape roadmap :** corrections d'abord, puis étape 3 de `VISUAL_REBUILD_PLAN.md` (master material + instances)  
**Statut :** vérifiée hors moteur, compilation UE 5.8 à valider sur le PC de test.

### Récupérer cette build
1. Sur GitHub, ouvrir la branche `claude/upbeat-knuth-kuzsu8`, puis **Code → Download ZIP**.
2. Copier le contenu du ZIP **par-dessus** le dossier local du projet en remplaçant les fichiers. Ne rien supprimer : `ArtSource/` et `Content/` (kit GLB et assets importés) ne sont pas sur GitHub et doivent rester.
3. Lancer `BUILD_AND_RUN.bat`.

### Corrections — joueur / arme
- la cadence de 720 coups/min est respectée dans tous les modes : cliquer vite en SEMI/BURST ou tapoter en AUTO ne tire plus plus vite que l'arme ;
- les zones de dégâts du joueur suivent la hauteur réelle de la capsule : accroupi, la tête pouvait ne jamais être touchée et la plupart des tirs au torse comptaient comme des tirs aux jambes (x0,72) ;
- le respawn relève le joueur accroupi, coupe l'élan et remet les déplacements à zéro ;
- le canon et la bouche étaient des cylindres debout (rotation sur le mauvais axe) : ils sont maintenant dans l'axe de tir ;
- le bloc de visée plein qui masquait la cible en ADS est remplacé par un viseur reflex ouvert avec point rouge centré sur la caméra.

### Corrections — IA
- les ennemis arrêtent de tirer sur le joueur mort et oublient le contact jusqu'au respawn ;
- les cadavres tombent au sol au lieu de rester suspendus à environ 65 cm ;
- casque, gilet et jambes des cadavres n'arrêtent plus les balles, la ligne de vue ni l'explosion des grenades ;
- les ennemis ont des jambes avec hitbox qui bougent en marchant : le torse ne flotte plus à 42 cm du sol et les tirs aux genoux touchent ;
- l'arme ennemie est dans l'axe de tir.

### Corrections — vagues
- le parcours des 20 points de spawn utilisait un pas de 5 : seuls 4 points étaient utilisés et chaque vague plafonnait à **4 ennemis** au lieu de 7 à 10. Le pas est maintenant premier avec le nombre de points ;
- 5 points de spawn déplacés : 2 étaient dans un mur, 1 dans la salle de maintenance, 2 sous les étagères flottantes de la cour Sud (ils sont maintenant sur les plateformes de tir).

### Corrections — map
- le mur d'enceinte ne couvrait que la moitié de chaque côté : les quatre coins étaient ouverts et on pouvait sortir de la map ;
- les portes s'ouvraient vers le joueur au lieu de s'ouvrir à l'opposé ;
- bureau de sécurité et maintenance : coins de murs ouverts, ouvertures de 2,75 m et 3,95 m pour une porte de 1,10 m, linteau et encadrement dans le vide. Les pièces sont maintenant fermées, avec un toit et une ouverture à la taille de la porte ;
- de nombreux blocs flottaient de quelques centimètres à 35 cm au-dessus du sol : ils sont prolongés jusqu'à la dalle ;
- passerelle Nord sans supports, et ses rampes étaient inclinées sur le côté à 9 m de distance : supports ajoutés, rampes à 17° qui rejoignent la passerelle ;
- étagères de la cour Sud flottantes : ce sont maintenant des plateformes de tir pleines de 1,20 m avec leurs rampes ;
- les tuyaux de 17 m de la cour, les tuyaux de maintenance et le raccord des réservoirs étaient debout comme des poteaux : ils sont horizontaux ;
- toits de l'entrepôt et du CQB décollés des murs, pannes, colonnes, auvents, portique, tour, pipe-rack, lampadaires et panneaux : remis au contact de leurs supports ;
- bandes de façade, portes de baies, plinthes et pilastres enfoncés dans les murs : replacés sur les faces visibles ;
- réservoirs et conteneurs en blocs ne sont plus générés quand les meshes du kit art existent (les deux occupaient la même place).

### Corrections — audio
- les sons d'une balle ou d'un impact étaient rattachés à un acteur détruit dans la même frame. Ils sont maintenant gardés dans le package transitoire pendant toute leur lecture.

### Nouveauté — matériaux environnement (étape 3)
- `Tools/build_environment_materials.py` crée `M_Master_IndustrialSurface` : teinte, rugosité, métal, variation de ton à grande échelle et salissure en pied de mur, sans dépendre des UV ;
- `BUILD_MATERIALS.bat` le génère via Unreal. `BUILD_AND_RUN.bat` le lance tout seul, sans bloquer, tant qu'il n'a pas réussi une fois ;
- le GameMode crée une instance par style de surface. Sans le master material, il garde le matériau de base d'avant ;
- palette alignée sur `ART_DIRECTION.md` : plus de structures quasi noires, béton gris froid, bleu marine désaturé, ambre de sécurité plus retenu.

### Outils
- `VERIFY_PROJECT.ps1` bloque la compilation si une de ces corrections est annulée par erreur ;
- `PUBLISH_TO_GITHUB.bat` publie sur cette branche et refuse de publier depuis un dossier plus ancien que la branche (il annulerait les corrections) ;
- les `.bat` sont extraits avec des fins de ligne Windows (CRLF).

### Vérifications faites
Aucune compilation Unreal n'est possible dans l'environnement de génération. Les contrôles ont été faits hors moteur :
- les 11 fichiers `.cpp` passent une vérification de syntaxe C++ contre des en-têtes UE simulés ;
- le compound généré par le GameMode a été exporté et contrôlé : 0 point de spawn dans un mur, 0 zone praticable hors de la map, plateformes accessibles par leurs rampes, 7 ennemis dès la vague 1 ;
- cadence, dégâts accroupi, chute des cadavres, arrêt du tir sur un joueur mort et mouvement des jambes testés en simulation ;
- le script de matériaux a été exécuté contre un faux module `unreal` (graphe complet, suppression de l'asset en cas d'échec) ;
- `VERIFY_PROJECT.ps1` et le garde-fou de publication ont été exécutés sous PowerShell.

### À tester en priorité
Voir la section 0 de `QA_CHECKLIST.md`.

### Limites
- en cas d'erreur de compilation, envoyer `BUILD_ERRORS.txt` ;
- les modèles GLB (`ArtSource/Industrial`) ne sont pas sur GitHub : installer Git LFS, mettre le dossier local à jour avec cette build, puis lancer `PUBLISH_TO_GITHUB.bat` pour les sauvegarder ;
- sans ces fichiers, le placement des meshes art n'a pas pu être vérifié visuellement ici.

## Industrial Visual Rebuild — démarrage

### Direction validée
- la map est reconstruite autour de 4 zones lisibles : Spawn/Staging, Central Yard, Logistics Lane, Maintenance/Utilities ;
- priorité au macro-layout et aux surfaces avant les micro-détails ;
- abandon du "gris/noir partout" au profit d'une palette béton / asphalte / acier / bleu industriel / jaune sécurité ;
- ajout des documents de production `VISUAL_REBUILD_PLAN.md`, `ART_DIRECTION.md` et `MATERIAL_PIPELINE.md`.

### Pipeline prévu
- master material environnement + instances ;
- assets modulaires réels pour façades/containers/barrières/pipe-racks ;
- ISM/HISM pour répétitions ;
- PCG uniquement pour dressing secondaire une fois la composition stabilisée.

### Statut
- branche : `dev/industrial-visual-rebuild` ;
- phase : préproduction terminée, implémentation visuelle en cours ;
- pas encore à merger dans `main`.

## Hotfix 2 — Import Interchange sans faux échec
- le log de test confirme que la compilation C++ réussit ;
- les 11 GLB sont bien traités par Interchange ;
- suppression des appels `load_asset()` sur des chemins absents qui écrivaient artificiellement 11 erreurs dans le log au premier import ;
- vérification préalable avec `does_asset_exist()` avant chargement ;
- validation finale par Asset Registry **ou** présence du fichier `.uasset` ;
- ajout d'un marqueur `Saved/ArtFoundationImport.ok` créé uniquement après un import réellement réussi ;
- `BUILD_AND_RUN.bat` relance donc l'import tant que ce marqueur n'existe pas, même si un ancien `.uasset` partiel est présent.

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
