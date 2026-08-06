# Chill Place – Version Web (Three.js)

Jeu 3D **hors ligne**, assets **100 % locaux** (pas de dépendance Google au runtime).

## Lancer

```bash
# Depuis le dossier web/ — il faut un serveur local (modules ES)
npx serve .
# ou: python3 -m http.server 8080
```

Ouvre l’URL affichée sur téléphone ou PC.

## Contrôles

| Entrée | Action |
|--------|--------|
| WASD / flèches | Déplacement |
| Stick gauche manette | Déplacement |
| Drag côté droit / stick droit | Regard |
| E / Espace / bouton A | Interagir (TV, tasse) |
| F / gâchettes | Jeter l’objet tenu |
| Gyro téléphone | Orientation caméra (si autorisé) |

## Contenu

- Menu 5 s sans bouton Play → fondu → scan manettes
- Pièce texturée (sol / mur / plafond fournis)
- Canapé + yoyo (GLB locaux)
- TV CRT + interface TikTok / YouTube
- Tasse avec gravité + prise
- Mocho (chat) qui regarde le joueur
- Gamepad API (Joy-Con, Xbox, PS, etc.)

## IA Mocho

Dans `js/main.js` :

```js
const API_KEY = "REMPLACER_PAR_VOTRE_CLE_API";
```

## Packaging APK offline (plus tard)

Capacitor embarque tout le dossier `web/` dans un APK autonome, sans CDN.
