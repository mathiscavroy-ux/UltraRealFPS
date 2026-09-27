# UltraRealFPS — QA Acoustic & CQB Interaction Update

Objectif : valider toute la mise à jour en une seule compilation/session.

## 1. Build / démarrage
- `BUILD_AND_RUN.bat` affiche `Precheck projet: OK`.
- UnrealBuildTool termine sans `error`.
- En cas d'échec, `BUILD_ERRORS.txt` est créé automatiquement.
- Play démarre sans message d'éclairage statique à régénérer.

## 2. Fusil / acoustique
- Tirer en extérieur : coup sec + queue extérieure perceptible, sans répétition infinie.
- Entrer dans le bureau de sécurité, fermer la porte et tirer : la queue doit paraître plus courte/dense que dehors.
- Tirer en AUTO : pas de freeze marqué dû à la génération audio.
- Un ennemi lointain qui tire doit rester localisable à gauche/droite par le son.

## 3. Occlusion
- Se placer derrière un mur pendant qu'un ennemi tire : son plus étouffé/faible qu'en ligne ouverte.
- Ouvrir/fermer une porte entre une source et le joueur doit modifier naturellement l'occlusion grâce à sa collision.

## 4. Pas / surfaces
- Béton : pas sourds/compacts.
- Marcher sur un objet métallique : timbre plus métallique.
- Marcher sur une caisse/plateforme bois : timbre différent.
- Sprint : pas plus rapides et plus audibles.
- Alt / accroupi : pas plus discrets.
- Atterrissage après un saut : impact supplémentaire si la chute est assez rapide.

## 5. IA et bruit joueur
- Approcher un ennemi accroupi derrière une séparation : il ne doit pas détecter le joueur à grande distance uniquement à cause des pas.
- Sprinter près d'un ennemi sans ligne de vue : il doit pouvoir enquêter sur la dernière position sonore.
- Manipuler une porte proche d'un ennemi : elle peut l'alerter.
- Un mur doit réduire fortement cette perception sonore.
- Les limites de tireurs simultanés doivent toujours empêcher une exécution instantanée par tout le groupe.

## 6. Near miss
- Se faire tirer dessus sans être touché : une balle passant près de la tête/du torse peut produire un crack bref et de la suppression.
- Une balle arrêtée par un mur ne doit pas produire de crack de near-miss de l'autre côté.

## 7. Impacts / douilles
- Métal, bois, béton et chair ont des impacts audio distincts.
- Une douille éjectée produit au maximum un cliquetis principal lorsqu'elle touche le sol/une surface.
- Le nombre de douilles ne doit pas provoquer de spam audio continu.

## 8. Portes
- Regarder une porte à courte distance : HUD `E OUVRIR LA PORTE`.
- `E` ouvre la porte dans le sens opposé au joueur.
- Nouveau prompt : `E FERMER LA PORTE`.
- La porte bloque le joueur et les tirs quand elle est fermée.
- La porte ouverte ne doit pas se téléporter ni tourner autour de son centre : le pivot reste côté charnière.
- Tirer dans la porte donne un feedback métal.

## 9. Nouveaux intérieurs
- Bureau sécurité sud-est : lumière chaude, collision correcte, pas de mur invisible dans l'entrée.
- Maintenance nord-est : lumière froide, porte fonctionnelle.
- Vérifier qu'un ennemi n'est pas obligé d'ouvrir une porte pour continuer le combat principal.

## 10. Régressions importantes
- Balistique bois/métal/béton toujours fonctionnelle.
- Casque/gilet et blessures ennemies toujours fonctionnels.
- `V` zéro, `B` modes de tir, `T` Low Ready, `G` grenade, `H` bandage, `F` lampe.
- Rechargement tactique/urgence, munitions, vagues, supplies et radio toujours fonctionnels.
- Lean X/C reste limité près des murs.

Si un test échoue, noter le numéro de section et ce qui se passe réellement. Pour une erreur de compilation, envoyer directement `BUILD_ERRORS.txt`.
