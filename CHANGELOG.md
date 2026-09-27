# UltraRealFPS — Changelog

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

