# UltraRealFPS — Changelog

## Visible Environment Overhaul — en test

### Changements immédiatement visibles
- refonte du sol avec grandes zones asphaltées, dalles de service et marquages plus lisibles dès le spawn ;
- contraste global renforcé : matériaux plus sombres et moins gris uniforme ;
- angle solaire abaissé, skylight réduit et exposition mieux contenue ;
- ajout d'un portique d'entrée central très visible ;
- entrepôt Est avec toit/corniche plus lisible et bandes de façade bleues ;
- shoot-house Ouest avec bande haute contrastée ;
- deux grands réservoirs industriels ajoutés pour casser la skyline et donner une vraie identité au côté Est.

### Rendu / exposition
- ajout d'un PostProcessVolume global ;
- compensation d'exposition négative légère ;
- adaptation min/max resserrée ;
- local exposure réglé pour préserver les hautes lumières et les ombres.

### Validation
- branche GitHub : `dev/visible-environment-overhaul` ;
- objectif : corriger le retour "les changements sont trop subtils" ;
- compilation/runtime UE 5.8 à valider sur le PC de test.


## Environment Graphics Pass — en test

### Structure visuelle
- les détails répétitifs non-collisionnants passent sur `UInstancedStaticMeshComponent` au lieu d'un Actor par élément ;
- l'entrepôt Est reçoit davantage de poutres, colonnes et panneaux hauts ;
- le shoot-house Ouest reçoit des encadrements et trims pour mieux lire ses pièces ;
- la cour Sud reçoit deux volumes type conteneur avec nervures et joints ;
- ajout de lampadaires de service comme repères visuels autour du compound.

### Éclairage / performance
- les lampes de service utilisent un falloff inverse-square et restent sans ombres pour limiter le coût ;
- les détails décoratifs restent sans collision : ils n'altèrent ni le déplacement ni la balistique ;
- l'instancing prépare une montée en densité visuelle avec moins d'Actors et moins d'overhead CPU.

### Validation
- branche GitHub : `dev/environment-graphics-pass` ;
- étape roadmap : **ÉTAPE 2 — graphismes / environnement** ;
- compilation/runtime UE 5.8 à valider sur le PC de test avant publication définitive.


## Visual Impact & Environment Pass — en test

### Impacts / feedback balistique
- marque d'impact aplatie et orientée sur la normale de surface ;
- micro-débris procéduraux sans asset externe : fragments béton, éclats bois, étincelles métal ;
- ricochets métal plus lumineux et plus nerveux ;
- variantes légères de taille/couleur/intensité pour casser la répétition ;
- impacts secondaires de grenade sur les surfaces proches, sans spam audio.

### Muzzle flash / explosion
- flash joueur et IA composé d'une lumière locale + d'un cône directionnel ;
- durée raccourcie et variation par tir pour supprimer l'effet "ampoule orange" ;
- falloff inverse-square et source radius configurés sur les lumières transitoires ;
- explosion de grenade plus brève/énergétique visuellement, avec traces de fragmentation autour du point d'explosion.

### Environnement
- helpers de détails non-collisionnants pour ne pas créer de pièges de gameplay ;
- poutres/ribs dans l'entrepôt, renforts, encadrements de portes, plinthes, luminaire simple ;
- tuyaux/cable tray dans la maintenance ;
- marquages industriels supplémentaires dans la voie centrale ;
- éclairage intérieur sécurité/maintenance passé sur un falloff plus physique.

### Validation
- branche GitHub : `dev/visual-impact-environment-pass` ;
- contrôles statiques C++ effectués ;
- compilation/runtime UE 5.8 à valider sur le PC de test avant merge dans `main`.


## Presentation Realism Pass — en test

### Viewmodel / sensation d'arme
- inertie du viewmodel liée aux changements de vitesse et aux mouvements de souris ;
- sway de marche retravaillé et respiration idle plus discrète ;
- animation de rechargement enrichie sans modifier la logique de munitions ;
- silhouette du fusil placeholder détaillée (upper receiver, front sight, trigger guard, foregrip) ;
- séparation visuelle corps / pièces métalliques / lentille d'optique.

### Audio placeholder
- banque de 4 variantes PCM par événement au lieu d'une seule forme d'onde répétée ;
- tir, réverbération intérieure/extérieure, grenade, pas et portes moins tonals ;
- davantage de bruit filtré/pression transitoire pour réduire le rendu "jouet" ;
- occlusion légèrement rééquilibrée pour conserver plus d'information directionnelle derrière un obstacle.

### Correctif gameplay
- lorsqu'un casque ennemi est détruit, sa collision Visibility est maintenant désactivée avec le mesh : il ne peut plus devenir une géométrie balistique invisible.

### Validation
- branche GitHub : `dev/presentation-realism-pass` ;
- compilation UE 5.8 encore à valider sur le PC de test avant merge vers `main`.


## Acoustic & CQB Interaction Update

### Audio / immersion
- couche audio procédurale centralisée `URFPSAudio` ;
- tirs joueur/IA spatialisés avec atténuation, occlusion et absorption HF ;
- queue acoustique intérieur/extérieur selon la géométrie environnante ;
- cracks supersoniques lors des near-miss ;
- pas béton, métal et bois pour joueur et IA ;
- sons d'impacts par surface ;
- sons de rechargement, dry fire, grenade, portes et douilles ;
- cache PCM par événement pour éviter la resynthèse à chaque tir.

### IA / perception
- bruit des pas lié au déplacement réel et au mode de locomotion ;
- sprint beaucoup plus audible que marche lente/accroupie ;
- occlusion simple des bruits de gameplay par la géométrie ;
- portes manipulées capables d'alerter une IA proche ;
- tirs audibles sur une part cohérente du compound sans donner automatiquement la vision au bot.

### CQB / interaction
- nouvel acteur `AURFPSDoor` ;
- ouverture opposée au joueur, fermeture, collision et son ;
- surface physique métal pour interaction balistique ;
- deux nouvelles zones intérieures secondaires : bureau sécurité et maintenance ;
- éclairage local chaud/froid pour tester lampe, ombres et acoustique.

### Workflow / fiabilité
- `COLLECT_BUILD_ERRORS.ps1` extrait automatiquement les erreurs UBT utiles ;
- `VERIFY_PROJECT.ps1` contrôle les régressions déjà rencontrées ;
- `.gitignore` Unreal ;
- `.gitattributes` prêt pour Git LFS ;
- `PUBLISH_TO_GITHUB.bat` publie via un clone temporaire sans modifier le dossier de jeu original ;
- `PROJECT_STATUS.md` et `QA_CHECKLIST.md` documentent la baseline.

### Validation
Les sources ont fait l'objet de contrôles statiques (délimiteurs, correspondance déclarations/définitions, anciennes API problématiques, fichiers requis). Unreal Engine 5.8 n'est pas disponible dans l'environnement de génération : la compilation UBT sur le PC de test reste la validation réelle.

## Correctif compilation UE 5.8 — douilles
- correction de `URFPSShellCasing.cpp` : `FHitResult::ImpactPoint` est un `FVector_NetQuantize` et ne doit pas être mélangé directement avec `FVector` dans l’opérateur ternaire sous MSVC 14.51 ;
- conversion évitée en copiant explicitement les composantes X/Y/Z dans un `FVector` ;
- ajout d’un contrôle statique dans `VERIFY_PROJECT.ps1` pour détecter la régression.

