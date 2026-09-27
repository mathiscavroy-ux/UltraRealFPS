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
- supplies et contrôle de vague ;
- audio spatial procédural, occlusion, pas par surface, near-miss, banque de variations ;
- portes interactives et zones CQB ;
- HUD tactique minimal ;
- viewmodel procédural avec inertie, sway, obstruction et reload visuel.

## Architecture
- `URFPSCharacter` — joueur, arme, santé, interaction.
- `URFPSProjectile` — projectile/ballistique.
- `URFPSEnemy` — IA tactique et état de combat.
- `URFPSGameMode` — monde procédural, vagues, matériaux, éclairage.
- `URFPSAudio` — audio procédural/spatial.
- `URFPSDoor` — porte interactive.
- `URFPSGrenade` — grenade et explosion.
- `URFPSImpactEffect` — impacts.
- `URFPSShellCasing` — douilles physiques.
- `URFPSHUD` — affichage.

## Risques techniques connus
1. Les primitives remplacent encore les vrais meshes/squelettes : limite majeure pour l'animation et le rendu.
2. IA custom sans NavMesh/Behavior Tree : suffisante pour l'arène actuelle, pas pour de grandes maps complexes.
3. Audio procédural : utile comme architecture/feedback, pas qualité finale d'un jeu commercial.
4. Beaucoup d'acteurs procéduraux séparés : acceptable sur la map actuelle, à remplacer plus tard par instancing / niveaux construits.
5. Le multijoueur n'est pas prévu dans les systèmes actuels ; ne pas l'ajouter avant stabilisation du solo.

## Développement actuel
- branche : `dev/visual-impact-environment-pass` ;
- priorité : impacts, muzzle flash, explosion, micro-détails environnementaux et éclairage ;
- pas encore mergé dans `main` : validation UE 5.8 requise.

## Priorités recommandées après cette update
1. Valider la branche `dev/visual-impact-environment-pass` sous UE 5.8 puis merger si stable.
2. Construire le pipeline de vrais assets audio/armes (Git LFS + imports reproductibles).
3. Remplacer progressivement l'audio procédural par des enregistrements/MetaSounds sans changer les appels gameplay.
4. Remplacer l'arme placeholder par un vrai viewmodel squelettique + animations.
5. Remplacer les ennemis primitives par personnages squelettiques.
6. Passer l'IA vers NavMesh + perception/BT lorsque la géométrie finale commence à exister.
7. Créer une vraie première mission au lieu d'empiler indéfiniment des vagues de test.
