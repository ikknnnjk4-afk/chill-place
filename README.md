# LE COULOIR — jeu d'horreur psychologique (HTML/CSS/JS pur)

100% hors-ligne. Aucune dépendance à un CDN, Google, ou service en ligne.
Tous les sons sont **synthétisés en direct** via Web Audio API (pas de fichiers .mp3/.wav requis).

## Structure
```
index.html
css/style.css      → moteur du couloir en CSS 3D (perspective/transform), éclairage, HUD
js/audio.js         → moteur audio (pas, coeur, drone, cri, statique TV, moniteur)
js/game.js          → logique de jeu (joystick, regard, tension, jumpscare)
assets/
  floor.jpg          → texture sol (répétable)
  wall.jpg           → texture murs (répétable)
  ceiling.jpg        → texture plafond (répétable)
  girl.png           → sprite de la fille (transparence conservée)
```

## Tester
Ouvrir simplement `index.html` dans un navigateur (ou WebView Android) — aucun serveur requis.
Pour le meilleur rendu mobile, tester dans une WebView Android (Chrome mobile fonctionne aussi).

## Contrôles
- **Joystick en bas à gauche** : déplacement (avancer/reculer + strafe gauche/droite).
- **Glisser sur la moitié droite de l'écran** : tourner la tête (regarder autour).
- Clavier (desktop, pour débug) : ZQSD/WASD + flèches.

## Mise à jour des effets

- Le déplacement produit un léger secouement organique de la caméra.
- À l'approche de la fille, une déformation optique naturelle apparaît progressivement et devient forte seulement dans les derniers instants avant le jumpscare.

## Transformer en APK
Le projet est une simple WebView statique, donc compatible avec :
- **Capacitor** (recommandé) : `npx cap init`, copier ce dossier dans `www/`, `npx cap add android`, build dans Android Studio.
- **Cordova** : copier dans `www/`, `cordova platform add android`, `cordova build android`.
- Aucune de ces étapes ne nécessite d'appel réseau au runtime : le jeu tourne 100% local une fois embarqué.

Pense à activer dans le manifest Android : `android:hardwareAccelerated="true"` pour de meilleures performances CSS 3D,
et à verrouiller l'orientation en `landscape` si tu veux un rendu plus cinématique (sinon le jeu s'adapte aussi au portrait).

## Réglages rapides (dans js/game.js)
- `MAX_SPEED` / `STRAFE_SPEED` : vitesse de déplacement.
- `CORRIDOR_LENGTH` : longueur du couloir (actuellement 6000 unités).
- `TENSION_START_DIST` : distance à partir de laquelle le coeur commence à battre.
- `JUMPSCARE_TRIGGER_DIST` : distance de déclenchement du jumpscare (~1 mètre stylisé).

## Notes
- Les textures sol/mur/plafond ont été légèrement compressées (JPEG) et redimensionnées en 512×512 pour rester
  légères en APK tout en restant répétables sans coupure visible.
- Le sprite de la fille a été recadré à sa zone utile (bounding box) en conservant la transparence PNG.
