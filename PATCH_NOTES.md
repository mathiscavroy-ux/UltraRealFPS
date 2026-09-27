# UltraRealFPS — Patch Notes

## Industrial Architecture Expansion — Build de test

**Branche GitHub :** `dev/visible-environment-overhaul`  
**Étape roadmap :** ÉTAPE 2 — Graphismes / environnement  
**Statut :** à compiler et tester sous Unreal Engine 5.8 avant merge.

### Changements majeurs
- nouvelle façade de chargement pour l'entrepôt Est avec quai, auvent, quatre grandes baies, nervures de portes, poteaux et bollards ;
- ajout de cinq unités de ventilation/service sur le toit de l'entrepôt ;
- refonte du bloc CQB Ouest avec ligne de toit en plusieurs hauteurs, auvent d'entrée et modules de façade ;
- portique central enrichi avec structure de type truss et petite cabine OPS suspendue ;
- nouvelle zone technique au Sud avec pipe-rack, conduites suspendues et cluster électrique ;
- nouvelle tour de surveillance au Nord pour modifier fortement la skyline ;
- murs périphériques découpés visuellement par des travées/poteaux clairs ;
- signalétique 3D dans le monde : WAREHOUSE 01, CQB WEST, OPS, HIGH VOLTAGE, TOWER 02.

### Éclairage / lisibilité
- soleil légèrement renforcé ;
- SkyLight légèrement remonté ;
- exposition globale moins sombre que la build précédente ;
- contraste local conservé pour éviter de revenir au rendu plat/surexposé.

### Performance / technique
- les petits détails répétitifs restent en Instanced Static Meshes ;
- les éléments décoratifs fins restent sans collision ;
- les structures réellement jouables utilisent les collisions habituelles ;
- aucun changement volontaire de la balistique, des dégâts, de l'IA ou des contrôles.

### À tester en priorité
1. Depuis le spawn, vérifier que la skyline est immédiatement plus riche.
2. Approcher l'entrepôt Est et vérifier les quatre baies de chargement.
3. Aller au bloc CQB Ouest et vérifier que les nouvelles formes ne bouchent aucun passage.
4. Passer sous/près du portique OPS et vérifier les collisions.
5. Traverser la cour Sud et vérifier le pipe-rack.
6. Regarder vers le Nord et confirmer que la tour est clairement visible.
7. Vérifier que les labels 3D sont lisibles et correctement orientés.
8. Vérifier qu'il n'y a pas de chute de FPS importante par rapport à la build précédente.

### Limites encore présentes
- les vrais assets PBR ne sont pas encore intégrés ;
- les bâtiments utilisent toujours majoritairement les primitives moteur ;
- l'arme et les personnages restent des placeholders ;
- la prochaine sous-étape doit préparer le pipeline de vrais meshes/textures et remplacer progressivement les plus gros placeholders.
