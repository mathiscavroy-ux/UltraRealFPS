# UltraRealFPS — QA Environment Graphics Pass

Objectif : valider toute la mise à jour en une seule compilation/session.

## 1. Build / démarrage
- `BUILD_AND_RUN.bat` affiche `Precheck projet: OK`.
- UnrealBuildTool termine sans `error`.
- En cas d'échec, `BUILD_ERRORS.txt` est créé automatiquement.
- Play démarre sans message d'éclairage statique à régénérer.

## 2. Environnement / instancing
- Démarrage : aucun crash lors de la construction du compound.
- Entrepôt Est : poutres/colonnes/panneaux supplémentaires visibles sans collisions invisibles.
- Shoot-house Ouest : encadrements et trims visibles sans bloquer les passages.
- Cour Sud : les deux volumes type conteneur sont bien présents et leurs nervures ne bloquent pas le joueur.
- Les détails décoratifs ne doivent pas intercepter les tirs.
- Vérifier l'absence de chute de FPS anormale par rapport à la build précédente.

## 3. Lampes de service
- Les cinq lampadaires sont visibles comme repères périphériques.
- Leur lumière reste localisée, sans transformer toute la map en zone surexposée.
- Aucun scintillement ou lumière restant dans le vide.
- Les lampadaires décoratifs n'empêchent pas le passage du joueur.

## 4. Impacts visuels
- Béton : marque sombre plate + quelques fragments gris, sans grosse boule visible.
- Métal : étincelles très brèves, plus marquées sur ricochet.
- Bois : éclats allongés brun/orange, plus lents que les étincelles métal.
- Chair : pas de fragments durs générés.
- Les débris ne doivent avoir aucune collision avec le joueur.
- Tirer en automatique sur un mur ne doit pas provoquer de freeze majeur.

## 5. Muzzle flash
- Le flash joueur dure seulement quelques dizaines de millisecondes.
- La lumière éclaire brièvement la surface devant le canon, pas uniquement autour du joueur.
- En AUTO, variation légère de rayon/intensité entre les tirs.
- Les ennemis utilisent le même principe sans halo permanent.
- Après mort/respawn, aucune lumière de muzzle flash ne reste allumée.

## 6. Grenade / explosion
- Explosion plus vive au départ puis extinction rapide.
- Des impacts secondaires peuvent apparaître sur murs/sols proches.
- Ces impacts secondaires ne doivent pas jouer neuf sons d'impact simultanés.
- La logique de dégâts/occlusion de grenade reste inchangée.

## 7. Détails environnementaux
- Entrepôt Est : poutres de plafond visibles, sans collision invisible.
- Bureau sécurité : encadrement/plinthes/luminaire visibles sans bloquer la porte.
- Maintenance : tuyaux et cable tray visibles sans gêner le déplacement.
- Marquages centraux : aucune collision et aucun effet sur la balistique.

## 8. Viewmodel / arme
- Déplacements rapides de souris : l'arme accuse légèrement le mouvement sans déplacer le point d'impact réel.
- Démarrage/arrêt de sprint : petite inertie perceptible, sans oscillation permanente.
- ADS : inertie fortement réduite et visée stable.
- Rechargement : le corps de l'arme accompagne le mouvement du chargeur sans clipping majeur.
- Vérifier la nouvelle silhouette : upper receiver, guidon avant, pontet et poignée avant visibles.
- Le viseur conserve un alignement exploitable en ADS.

## 9. Fusil / acoustique
- Tirer en extérieur : coup sec + queue extérieure perceptible, sans répétition infinie.
- Entrer dans le bureau de sécurité, fermer la porte et tirer : la queue doit paraître plus courte/dense que dehors.
- Tirer en AUTO : pas de freeze marqué dû à la génération audio.
- Un ennemi lointain qui tire doit rester localisable à gauche/droite par le son.

## 10. Occlusion
- Se placer derrière un mur pendant qu'un ennemi tire : son plus étouffé/faible qu'en ligne ouverte.
- Ouvrir/fermer une porte entre une source et le joueur doit modifier naturellement l'occlusion grâce à sa collision.

## 11. Pas / surfaces
- Béton : pas sourds/compacts.
- Marcher sur un objet métallique : timbre plus métallique.
- Marcher sur une caisse/plateforme bois : timbre différent.
- Sprint : pas plus rapides et plus audibles.
- Alt / accroupi : pas plus discrets.
- Atterrissage après un saut : impact supplémentaire si la chute est assez rapide.

## 12. IA et bruit joueur
- Approcher un ennemi accroupi derrière une séparation : il ne doit pas détecter le joueur à grande distance uniquement à cause des pas.
- Sprinter près d'un ennemi sans ligne de vue : il doit pouvoir enquêter sur la dernière position sonore.
- Manipuler une porte proche d'un ennemi : elle peut l'alerter.
- Un mur doit réduire fortement cette perception sonore.
- Les limites de tireurs simultanés doivent toujours empêcher une exécution instantanée par tout le groupe.

## 13. Near miss
- Se faire tirer dessus sans être touché : une balle passant près de la tête/du torse peut produire un crack bref et de la suppression.
- Une balle arrêtée par un mur ne doit pas produire de crack de near-miss de l'autre côté.

## 14. Impacts / douilles
- Métal, bois, béton et chair ont des impacts audio distincts.
- Une douille éjectée produit au maximum un cliquetis principal lorsqu'elle touche le sol/une surface.
- Le nombre de douilles ne doit pas provoquer de spam audio continu.

## 15. Portes
- Regarder une porte à courte distance : HUD `E OUVRIR LA PORTE`.
- `E` ouvre la porte dans le sens opposé au joueur.
- Nouveau prompt : `E FERMER LA PORTE`.
- La porte bloque le joueur et les tirs quand elle est fermée.
- La porte ouverte ne doit pas se téléporter ni tourner autour de son centre : le pivot reste côté charnière.
- Tirer dans la porte donne un feedback métal.

## 16. Nouveaux intérieurs
- Bureau sécurité sud-est : lumière chaude, collision correcte, pas de mur invisible dans l'entrée.
- Maintenance nord-est : lumière froide, porte fonctionnelle.
- Vérifier qu'un ennemi n'est pas obligé d'ouvrir une porte pour continuer le combat principal.

## 17. Armure ennemie
- Détruire un casque ennemi : il disparaît.
- Tir suivant dans la zone de tête : aucun impact métallique ne doit provenir d'un casque invisible.
- Le gilet continue à absorber selon sa durabilité.

## 18. Régressions importantes
- Balistique bois/métal/béton toujours fonctionnelle.
- Casque/gilet et blessures ennemies toujours fonctionnels.
- `V` zéro, `B` modes de tir, `T` Low Ready, `G` grenade, `H` bandage, `F` lampe.
- Rechargement tactique/urgence, munitions, vagues, supplies et radio toujours fonctionnels.
- Lean X/C reste limité près des murs.

Si un test échoue, noter le numéro de section et ce qui se passe réellement. Pour une erreur de compilation, envoyer directement `BUILD_ERRORS.txt`.
