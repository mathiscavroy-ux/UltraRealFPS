# UltraRealFPS — Acoustic & CQB Interaction Update (Unreal Engine 5.8)

Cette version continue directement la Ballistics & Armor Update corrigée. L'objectif principal de cette passe est de réduire l'impression de prototype sans empiler artificiellement des mécaniques : le son devient spatial et lié à la géométrie, les déplacements ont enfin un vrai feedback de surface, les tirs proches sont lisibles à l'oreille, l'IA peut entendre les actions du joueur et la map reçoit ses premières portes réellement interactives.

Le projet reste autonome : aucun pack Marketplace ou fichier audio externe n'est requis. Les sons de cette version sont synthétisés en C++ à l'exécution. C'est volontairement une étape technique intermédiaire avant l'arrivée de banques audio enregistrées et de vrais assets lourds.

## Nouveautés principales

### Couche audio 3D procédurale
Un nouveau module `URFPSAudio` centralise les sons du prototype :
- tirs joueur et ennemis ;
- explosions ;
- pas béton / métal / bois ;
- impacts béton / métal / bois / chair ;
- rechargement et dry fire ;
- portes ;
- douilles ;
- crack supersonique des balles qui passent près du joueur.

Les sons spatialisés utilisent une atténuation 3D, une absorption des hautes fréquences avec la distance et une occlusion basée sur le canal `Visibility`. Les données PCM sont synthétisées une fois par type d'événement puis mises en cache, afin d'éviter de recalculer des milliers d'échantillons à chaque tir automatique.

### Réflexions intérieur / extérieur des coups de feu
`URFPSAudio::PlayGunshot` sonde rapidement la géométrie autour du canon. Selon le nombre de surfaces proches, le tir reçoit :
- une queue courte et dense en espace fermé ;
- une queue plus tardive et plus légère à l'extérieur.

Ce n'est pas encore une simulation acoustique par convolution, mais un même tir ne sonne plus exactement comme s'il était joué dans le vide quel que soit l'endroit.

### Pas liés aux surfaces
Le joueur et les ennemis déterminent la surface sous leurs pieds à partir des `Physical Materials` déjà introduits :
- Concrete ;
- Metal ;
- Wood.

Le rythme dépend de la distance réellement parcourue, pas seulement d'un timer fixe. Sprint, marche normale, marche lente et accroupissement ont des cadences et volumes différents. Un atterrissage suffisamment fort produit également un impact de pas.

### Le bruit devient une mécanique de gameplay
Les pas du joueur ne sont plus seulement cosmétiques. L'IA peut entendre :
- un sprint à une distance nettement supérieure à un déplacement accroupi ;
- une marche normale à courte portée ;
- une porte manipulée à proximité.

Un mur entre le bruit et l'ennemi réduit fortement la portée d'alerte. Ces bruits donnent à l'IA une position sonore récente, sans lui donner artificiellement une ligne de vue.

La portée d'alerte d'un vrai coup de feu a également été augmentée : dans un compound de cette taille, un fusil est maintenant audible sur une part importante de la map, tandis que les murs réduisent fortement cette portée. Les limites de tireurs simultanés restent actives afin de ne pas revenir au problème des morts instantanées.

### Near-miss / crack supersonique
Quand une balle ennemie passe suffisamment près du joueur sans rencontrer un mur :
- la suppression existante est appliquée ;
- un crack très court est maintenant joué au point de passage le plus proche.

Cela rend les tirs ratés dangereux et lisibles sans ajouter de dégâts invisibles.

### Impacts et douilles sonores
Les impacts reprennent directement la surface physique touchée. Le métal, le bois, le béton et la chair ne partagent plus le même feedback audio.

Les douilles physiques peuvent maintenant produire un petit cliquetis métallique lors de leur premier impact suffisamment énergique. Un seul son est autorisé par douille pour éviter une avalanche de petits sons lors des rebonds.

### Portes interactives — `E`
Un nouvel acteur `AURFPSDoor` fournit :
- porte métallique avec collision ;
- pivot physique sur un côté ;
- ouverture dans le sens opposé au joueur ;
- fermeture ;
- interpolation de rotation ;
- son d'ouverture/fermeture ;
- surface balistique métallique ;
- prompt HUD `E OUVRIR / FERMER LA PORTE`.

Deux zones CQB fermées ont été ajoutées au compound. Elles sont volontairement secondaires : l'IA de vague n'est pas obligée de franchir une porte pour rejoindre le joueur, ce qui évite de créer un blocage tant qu'elle ne sait pas encore manipuler les portes elle-même.

### Éclairage intérieur
Les deux nouvelles zones intérieures disposent de lumières dynamiques locales différentes :
- bureau de sécurité : teinte chaude ;
- maintenance nord-est : teinte plus froide.

Cela donne immédiatement des espaces visuellement distincts et permet de tester le comportement de la lampe tactique et des ombres dans des volumes fermés.

### Portée audio adaptée à la map
Le son direct d'un fusil porte maintenant au-delà des dimensions principales du compound. Les sons faibles — pas, douilles, portes — restent au contraire volontairement locaux. Le but est d'obtenir une hiérarchie crédible des événements sonores sans saturer le mix.

## Organisation technique

Nouveaux fichiers :
- `Source/UltraRealFPS/URFPSAudio.h/.cpp`
- `Source/UltraRealFPS/URFPSDoor.h/.cpp`
- `COLLECT_BUILD_ERRORS.ps1`
- `.gitignore`
- `.gitattributes`
- `PUBLISH_TO_GITHUB.bat`
- `PROJECT_STATUS.md`

Les systèmes existants restent séparés :
- `URFPSCharacter` : joueur / arme / médical / interactions ;
- `URFPSProjectile` : trajectoire et interactions balistiques ;
- `URFPSEnemy` : combat, rôles, blessures, perception ;
- `URFPSGameMode` : map, vagues, éclairage, matériaux ;
- `URFPSAudio` : génération et lecture du son ;
- `URFPSDoor` : comportement d'une porte ;
- `URFPSImpactEffect` : feedback d'impact ;
- `URFPSShellCasing` : douilles physiques.

## Contrôles principaux

- `Z/W` : avancer
- `S` : reculer
- `Q/A` : gauche
- `D` : droite
- `Souris` : regarder
- `Clic gauche` : tirer
- `Clic droit` : viser
- `Shift gauche` : sprint / retenir la respiration en ADS
- `Alt gauche` : marche tactique lente
- `Ctrl gauche` : accroupissement
- `Espace` : saut
- `X` / `C` : lean gauche / droite
- `R` : recharger / reprendre après la mort
- `E` : interaction, portes, ravitaillement, radio
- `F` : lampe tactique
- `B` : SEMI → BURST → AUTO
- `V` : zéro 50 / 100 / 200 / 300 m
- `T` : Low Ready
- `G` : grenade
- `H` : bandage
- `Échap` : libérer / recapturer la souris

## Compilation

1. Fermer toute instance d'UltraRealFPS dans Unreal.
2. Extraire le ZIP dans un nouveau dossier.
3. Lancer `BUILD_AND_RUN.bat`.
4. `VERIFY_PROJECT.ps1` contrôle d'abord les erreurs récurrentes et la présence des nouveaux systèmes.
5. UnrealBuildTool compile `UltraRealFPSEditor` en `Development / Win64`.
6. Si la compilation réussit, Unreal Engine 5.8 se lance automatiquement.

En cas d'échec, `COLLECT_BUILD_ERRORS.ps1` extrait automatiquement les lignes utiles du log UnrealBuildTool dans :

`BUILD_ERRORS.txt`

Il suffit ensuite d'envoyer ce fichier dans le chat au lieu de copier l'intégralité du terminal.

## GitHub

Le dépôt privé du projet est : `mathiscavroy-ux/UltraRealFPS`.

Le ZIP contient `.gitignore` et `.gitattributes` adaptés à Unreal. Les futurs `.uasset`, `.umap`, textures, sons et meshes sont configurés pour Git LFS.

Pour publier cette version comme première vraie baseline Git, `PUBLISH_TO_GITHUB.bat` fait maintenant une opération plus sûre : il clone le dépôt dans un dossier temporaire, y copie uniquement les fichiers utiles du projet, commit puis push. Le dossier Unreal original n'est donc pas transformé en dépôt Git et le script n'effectue pas de merge d'histoires locales potentiellement conflictuel. Après cette première publication, on pourra cloner proprement le dépôt comme workspace principal.

## Limites connues

- Les sons sont synthétiques : ils améliorent beaucoup le feedback mais ne remplacent pas encore des enregistrements multi-couches de vraies armes.
- Les personnages et l'arme utilisent toujours des primitives Unreal, pas des meshes/animations de production.
- L'IA n'utilise pas encore NavMesh + Behavior Tree : elle navigue sur une grille construite au lancement (Build 2). Elle ouvre les portes mais ne les referme pas.
- Les portes tournent par interpolation et collision simple ; il n'y a pas encore de poignée animée, verrou, destruction ou ouverture progressive au maintien de touche.
- Aucun test de compilation UE 5.8 ne peut être exécuté dans l'environnement de génération. Les contrôles effectués ici sont statiques ; la compilation sur le PC du joueur reste la validation réelle.

Voir `QA_CHECKLIST.md` pour une session de test groupée.
