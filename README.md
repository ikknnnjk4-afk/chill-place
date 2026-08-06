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
capacitor.config.json → configuration Capacitor pour le build APK
package.json          → dépendances Capacitor
```

## Tester
Ouvrir simplement `index.html` dans un navigateur (ou WebView Android) — aucun serveur requis.
Pour le meilleur rendu mobile, tester dans une WebView Android (Chrome mobile fonctionne aussi).

## Contrôles
- **Joystick en bas à gauche** : déplacement (avancer/reculer + strafe gauche/droite).
- **Glisser sur la moitié droite de l'écran** : tourner la tête (regarder autour).
- Clavier (desktop, pour débug) : ZQSD/WASD + flèches.

## Nouveautés (v1.1)
- **Secoue d'écran au déplacement** : léger tremblement de caméra proportionnel à la vitesse, comme le poids du corps à chaque pas.
- **Illusion d'optique progressive** : quand on s'approche vraiment proche de la fille (pas au début — seulement dans le dernier tiers de tension), le monde commence à se déformer organiquement, comme une illusion d'optique qui « respire ». Pas numérique — une distorsion fluide et organique via un filtre SVG feTurbulence.

## Transformer en APK via GitHub Actions
Le workflow `.github/workflows/build-apk.yml` compile automatiquement un APK à chaque push sur `main`.

### Étapes :
1. **Pousser ce dossier** sur un dépôt GitHub public ou privé.
2. **Aller dans Actions** → le workflow `Build Android APK` se déclenche automatiquement.
3. Une fois terminé, télécharger `le-couloir.apk` depuis les **Artifacts** de la run.
4. Pour une **Release publique** avec lien stable :
   - Créer un tag Git : `git tag v1.0.0 && git push origin v1.0.0`
   - Le workflow crée automatiquement une Release GitHub avec l'APK.
   - Le lien permanent sera : `https://github.com/USER/REPO/releases/latest/download/le-couloir.apk`

### Permissions requises dans GitHub
Dans **Settings → Actions → General → Workflow permissions** :
- Activer **"Read and write permissions"** (pour la création de Release).

## Build manuel (si vous préférez)
```bash
cd game/
npm install
npx cap add android
npx cap sync android
cd android/
./gradlew assembleDebug
# APK dans : android/app/build/outputs/apk/debug/app-debug.apk
```

## Réglages rapides (dans js/game.js)
- `MAX_SPEED` / `STRAFE_SPEED` : vitesse de déplacement.
- `CORRIDOR_LENGTH` : longueur du couloir (actuellement 6000 unités).
- `TENSION_START_DIST` : distance à partir de laquelle le coeur commence à battre.
- `JUMPSCARE_TRIGGER_DIST` : distance de déclenchement du jumpscare (~1 mètre stylisé).

## Notes
- Les textures sol/mur/plafond ont été légèrement compressées (JPEG) et redimensionnées en 512×512 pour rester légères en APK tout en restant répétables sans coupure visible.
- Le sprite de la fille a été recadré à sa zone utile (bounding box) en conservant la transparence PNG.
- Penser à activer dans le manifest Android : `android:hardwareAccelerated="true"` (déjà fait automatiquement par le workflow CI).
