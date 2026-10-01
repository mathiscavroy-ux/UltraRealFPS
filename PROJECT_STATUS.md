# UltraRealFPS — état technique du projet

## Base actuelle
- Unreal Engine 5.8
- C++
- Windows / DX12 / Shader Model 6
- Lumen GI + reflections
- Virtual Shadow Maps
- solo tactique par vagues

## Systèmes déjà présents
- locomotion FPS, sprint/endurance, accroupi, marche lente, lean, low ready ;
- ADS, hold breath, recoil, bloom, sway, headbob, obstruction de canon ;
- SEMI / BURST / AUTO, zéro 50-300 m, rechargement tactique/urgence ;
- projectile balistique, gravité/drag, pénétration bois, ricochet métal/béton ;
- surfaces physiques Concrete/Metal/Wood/Flesh ;
- dégâts localisés, armure, blessures, saignement/bandage ;
- grenades physiques ;
- IA par rôles, suppression, perception, fire slots, cover bias, vagues ;
- navigation IA sur grille (A*), patrouille, fouille, traque, ouverture des portes, rythme des vagues ;
- supplies et contrôle de vague ;
- audio spatial procédural, occlusion, pas par surface, near-miss, banque de variations ;
- portes interactives et zones CQB ;
- HUD tactique minimal ;
- viewmodel procédural avec inertie, sway, obstruction et reload visuel.

## Architecture
- `URFPSCharacter` — joueur, arme, santé, interaction.
- `URFPSProjectile` — projectile/ballistique.
- `URFPSEnemy` — IA tactique et état de combat.
- `URFPSGameMode` — monde procédural, vagues, matériaux, éclairage, rythme des vagues.
- `URFPSNavGrid` — grille de navigation des ennemis, construite au lancement depuis les obstacles.
- `URFPSAudio` — audio procédural/spatial.
- `URFPSDoor` — porte interactive.
- `URFPSGrenade` — grenade et explosion.
- `URFPSImpactEffect` — impacts.
- `URFPSShellCasing` — douilles physiques.
- `URFPSHUD` — affichage.

## Risques techniques connus
1. Les primitives remplacent encore les vrais meshes/squelettes : limite majeure pour l'animation et le rendu.
2. IA custom sans NavMesh/Behavior Tree : elle navigue sur une grille 2D construite au lancement. Ça suffit pour le compound actuel au sol, mais pas pour plusieurs étages ou de grandes maps.
3. Audio procédural : utile comme architecture/feedback, pas qualité finale d'un jeu commercial.
4. Beaucoup d'acteurs procéduraux séparés : acceptable sur la map actuelle, à remplacer plus tard par instancing / niveaux construits.
5. Le multijoueur n'est pas prévu dans les systèmes actuels ; ne pas l'ajouter avant stabilisation du solo.

## Développement actuel
- branche : `claude/upbeat-knuth-kuzsu8` (Industrial Visual Rebuild, Builds 1 et 2) ;
- base : `dev/industrial-visual-rebuild`, elle-même issue de `dev/art-foundation-industrial-kit` ;
- étape roadmap : corrections de gameplay et de géométrie, puis **étape 3 de `VISUAL_REBUILD_PLAN.md` — master material + instances** ;
- le GameMode utilise `M_Master_IndustrialSurface` quand il existe et garde le matériau de base en fallback ;
- les 11 modèles GLB du kit art restent importés automatiquement et utilisés quand ils existent ;
- Build 2 : les ennemis naviguent sur une grille, patrouillent, fouillent, traquent le joueur et ouvrent les portes ; un joueur caché ne bloque plus la vague ;
- validation requise : compilation UE 5.8, génération du master material, sections A et B de `QA_CHECKLIST.md`.

## Points ouverts
- `ArtSource/Industrial` (GLB) n'est pas sur GitHub : publier avec Git LFS installé depuis un dossier à jour ;
- la boussole du HUD affiche N face à l'entrepôt Est. Les noms de zones (Nord = +Y, Est = +X) forment un repère en miroir : aucune boussole ne peut les suivre sans échanger Nord et Sud dans les noms ;
- l'entrepôt et le bloc CQB restent ouverts. L'IA sait maintenant ouvrir les portes, donc on peut les fermer dans une prochaine passe ;
- le placement des meshes art n'a pas encore été vérifié depuis la correction de la géométrie.

## Documentation build
- `PATCH_NOTES.md` contient les patch notes de la build de test courante.

## Priorités recommandées après cette update
1. Valider la branche `claude/upbeat-knuth-kuzsu8` sous UE 5.8 puis merger si stable.
2. Construire le pipeline de vrais assets audio/armes (Git LFS + imports reproductibles).
3. Remplacer progressivement l'audio procédural par des enregistrements/MetaSounds sans changer les appels gameplay.
4. Remplacer l'arme placeholder par un vrai viewmodel squelettique + animations.
5. Remplacer les ennemis primitives par personnages squelettiques.
6. Passer l'IA vers NavMesh + perception/BT lorsque la géométrie finale commence à exister (la grille actuelle est le relais d'ici là).
7. Créer une vraie première mission au lieu d'empiler indéfiniment des vagues de test.
