/**
 * Chill Place – Three.js offline
 * Contrôles inspirés du rendu "Labyrinthe" (caméra linéaire, zones tactiles, manettes).
 * // REMPLACEZ CETTE LIGNE PAR VOTRE PROPRE CLE API
 */
const API_KEY = "REMPLACER_PAR_VOTRE_CLE_API";

import * as THREE from 'three';
import { GLTFLoader } from './vendor/GLTFLoader.js';

const ROOM_W = 10, ROOM_H = 3.0, ROOM_D = 12;
const EYE = 1.65;
const MOVE_SPEED = 3.2;
const LOOK_SENS = 0.0034;
const PAD_LOOK = 2.6;
const DEAD = 0.15;

const canvas = document.getElementById('c');
const titleScreen = document.getElementById('title-screen');
const fadeBlack = document.getElementById('fade-black');
const controllerScan = document.getElementById('controller-scan');
const scanDots = document.getElementById('scan-dots');
const padStatus = document.getElementById('pad-status');
const hud = document.getElementById('hud');
const interactHint = document.getElementById('interact-hint');
const tvUi = document.getElementById('tv-ui');
const mochoBubble = document.getElementById('mocho-bubble');
const fpsEl = document.getElementById('fps');

const renderer = new THREE.WebGLRenderer({
  canvas, antialias: true, powerPreference: 'high-performance'
});
renderer.setPixelRatio(Math.min(devicePixelRatio, 1.8));
renderer.setSize(innerWidth, innerHeight);
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.1;

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x1c1824);
scene.fog = new THREE.Fog(0x1c1824, 12, 26);

const camera = new THREE.PerspectiveCamera(75, innerWidth / innerHeight, 0.05, 60);
camera.rotation.order = 'YXZ';
camera.position.set(0, EYE, 1.8);

// VR stereo (double écran)
const stereo = new THREE.StereoCamera();
stereo.eyeSep = 0.064;
let vrMode = false;

scene.add(new THREE.AmbientLight(0xfff4ea, 0.5));
scene.add(new THREE.HemisphereLight(0xc8d8ff, 0x3a2818, 0.55));
const sun = new THREE.DirectionalLight(0xffe8d0, 1.05);
sun.position.set(2.2, 5.5, 2.5);
sun.castShadow = true;
sun.shadow.mapSize.set(1024, 1024);
sun.shadow.camera.left = -7; sun.shadow.camera.right = 7;
sun.shadow.camera.top = 7; sun.shadow.camera.bottom = -7;
scene.add(sun);
const lamp = new THREE.PointLight(0xffcc88, 0.7, 14, 2);
lamp.position.set(0, 2.55, 6);
scene.add(lamp);

const state = {
  phase: 'title', timer: 0,
  yaw: 0, pitch: 0,
  move: { x: 0, y: 0 },
  tvOn: false, tvChoice: 0,
  holding: null,
  lookMug: 0,
  mochoTalk: 0,
  pads: [],
};

const world = {
  sofa: null, yoyo: null, mug: null, mugMesh: null,
  tv: null, tvScreen: null, tvGroup: null,
  mocho: null, mochoHead: null,
  colliders: [], // solid AABBs {minX,maxX,minY,maxY,minZ,maxZ}
};
const mugBody = { pos: new THREE.Vector3(-1.35, 0.55, 5.2), vel: new THREE.Vector3(), held: false };

const loader = new GLTFLoader();
const texLoader = new THREE.TextureLoader();
const clock = new THREE.Clock();
let frames = 0, fpsT = 0;

function loadTex(url, rx, ry) {
  return new Promise((res) => {
    texLoader.load(url, (t) => {
      t.colorSpace = THREE.SRGBColorSpace;
      t.wrapS = t.wrapT = THREE.MirroredRepeatWrapping;
      t.repeat.set(rx, ry);
      t.anisotropy = Math.min(8, renderer.capabilities.getMaxAnisotropy());
      res(t);
    }, undefined, () => res(null));
  });
}
function loadGLB(url) {
  return new Promise((res) => {
    loader.load(url, (g) => res(g.scene), undefined, () => res(null));
  });
}
function fitOnFloor(root, targetWidth) {
  const box = new THREE.Box3().setFromObject(root);
  const size = box.getSize(new THREE.Vector3());
  const s = targetWidth / Math.max(size.x, 0.001);
  root.scale.setScalar(s);
  box.setFromObject(root);
  root.position.y = -box.min.y;
  return box.getSize(new THREE.Vector3());
}
function addColliderFromObject(obj, pad = 0.05) {
  const box = new THREE.Box3().setFromObject(obj);
  world.colliders.push({
    minX: box.min.x - pad, maxX: box.max.x + pad,
    minY: box.min.y, maxY: box.max.y,
    minZ: box.min.z - pad, maxZ: box.max.z + pad,
  });
}
function resolvePlayer(pos, radius = 0.35) {
  for (let it = 0; it < 3; it++) {
    for (const b of world.colliders) {
      if (pos.y + 0.2 > b.maxY || pos.y + 1.5 < b.minY) continue;
      const cx = Math.max(b.minX, Math.min(pos.x, b.maxX));
      const cz = Math.max(b.minZ, Math.min(pos.z, b.maxZ));
      let dx = pos.x - cx, dz = pos.z - cz;
      const d2 = dx * dx + dz * dz;
      if (d2 < radius * radius) {
        const d = Math.sqrt(d2) || 1e-4;
        const o = radius - d;
        pos.x += (dx / d) * o;
        pos.z += (dz / d) * o;
      }
    }
  }
  pos.x = Math.max(-ROOM_W / 2 + 0.4, Math.min(ROOM_W / 2 - 0.4, pos.x));
  pos.z = Math.max(0.5, Math.min(ROOM_D - 0.5, pos.z));
}

function makeRoom(floorT, wallT, ceilT) {
  const floorMat = new THREE.MeshStandardMaterial({ map: floorT || null, color: floorT ? 0xffffff : 0x555, roughness: 0.9 });
  const wallMat = new THREE.MeshStandardMaterial({ map: wallT || null, color: wallT ? 0xffffff : 0x777, roughness: 0.92 });
  const ceilMat = new THREE.MeshStandardMaterial({ map: ceilT || null, color: ceilT ? 0xffffff : 0x888, roughness: 1 });

  const floor = new THREE.Mesh(new THREE.PlaneGeometry(ROOM_W, ROOM_D), floorMat);
  floor.rotation.x = -Math.PI / 2;
  floor.position.set(0, 0, ROOM_D / 2);
  floor.receiveShadow = true;
  scene.add(floor);

  const ceil = new THREE.Mesh(new THREE.PlaneGeometry(ROOM_W, ROOM_D), ceilMat);
  ceil.rotation.x = Math.PI / 2;
  ceil.position.set(0, ROOM_H, ROOM_D / 2);
  scene.add(ceil);

  const mk = (w, h, x, y, z, ry) => {
    const m = new THREE.Mesh(new THREE.PlaneGeometry(w, h), wallMat);
    m.position.set(x, y, z); m.rotation.y = ry; m.receiveShadow = true;
    scene.add(m);
  };
  mk(ROOM_W, ROOM_H, 0, ROOM_H / 2, ROOM_D, Math.PI);
  mk(ROOM_W, ROOM_H, 0, ROOM_H / 2, 0, 0);
  mk(ROOM_D, ROOM_H, -ROOM_W / 2, ROOM_H / 2, ROOM_D / 2, Math.PI / 2);
  mk(ROOM_D, ROOM_H, ROOM_W / 2, ROOM_H / 2, ROOM_D / 2, -Math.PI / 2);
}

function makeTable() {
  const g = new THREE.Group();
  const top = new THREE.Mesh(new THREE.BoxGeometry(1.35, 0.05, 0.65),
    new THREE.MeshStandardMaterial({ color: 0x4a3828, roughness: 0.55 }));
  top.position.y = 0.42; top.castShadow = true; g.add(top);
  [[-0.55, -0.25], [0.55, -0.25], [-0.55, 0.25], [0.55, 0.25]].forEach(([x, z]) => {
    const leg = new THREE.Mesh(new THREE.BoxGeometry(0.05, 0.42, 0.05),
      new THREE.MeshStandardMaterial({ color: 0x2e2218 }));
    leg.position.set(x, 0.21, z); g.add(leg);
  });
  g.position.set(0, 0, 7.4);
  scene.add(g);
  addColliderFromObject(g);
}

/** CRT solide – pas de gros rectangle parasite derrière */
function makeCRT() {
  const g = new THREE.Group();
  const bodyMat = new THREE.MeshStandardMaterial({ color: 0x2a2430, roughness: 0.55 });
  const body = new THREE.Mesh(new THREE.BoxGeometry(0.92, 0.7, 0.52), bodyMat);
  body.position.y = 0.88; body.castShadow = true; g.add(body);

  const cab = new THREE.Mesh(new THREE.BoxGeometry(1.0, 0.48, 0.48),
    new THREE.MeshStandardMaterial({ color: 0x3a2a1c, roughness: 0.7 }));
  cab.position.y = 0.24; cab.castShadow = true; g.add(cab);

  // Écran (plane sur face avant uniquement)
  const screenMat = new THREE.MeshStandardMaterial({
    color: 0x0a1a40, emissive: 0x0a1a40, emissiveIntensity: 0.35, roughness: 0.25
  });
  const screen = new THREE.Mesh(new THREE.PlaneGeometry(0.7, 0.46), screenMat);
  screen.position.set(0, 0.9, -0.265);
  g.add(screen);
  world.tvScreen = screen;

  g.position.set(0, 0, 8.5);
  scene.add(g);
  world.tvGroup = g;
  world.tv = g;
  addColliderFromObject(g, 0.08);
}

/** Tasse : charge mug.glb si présent (Tripo), sinon rien d'invasif */
async function loadMug() {
  const glb = await loadGLB('assets/models/mug.glb');
  if (glb) {
    fitOnFloor(glb, 0.12);
    glb.position.copy(mugBody.pos);
    glb.position.y = mugBody.pos.y;
    glb.traverse((o) => { if (o.isMesh) o.castShadow = true; });
    scene.add(glb);
    world.mugMesh = glb;
    return;
  }
  // Placeholder discret en attendant le GLB Tripo – pas de “reconstruction” fancy
  const g = new THREE.Group();
  const m = new THREE.MeshStandardMaterial({ color: 0xe8dff0, roughness: 0.35 });
  const body = new THREE.Mesh(new THREE.CylinderGeometry(0.05, 0.055, 0.1, 20), m);
  body.castShadow = true; g.add(body);
  g.position.copy(mugBody.pos);
  scene.add(g);
  world.mugMesh = g;
}

/** Mocho : vrai GLB si fourni (mocho.glb / chat.glb), sinon chat simple propre */
async function loadMocho() {
  let glb = await loadGLB('assets/models/mocho.glb');
  if (!glb) glb = await loadGLB('assets/models/chat.glb');
  if (glb) {
    fitOnFloor(glb, 0.55);
    glb.position.set(0, 0, 6.0);
    glb.traverse((o) => { if (o.isMesh) o.castShadow = true; });
    scene.add(glb);
    world.mocho = glb;
    // tête = enfant le plus haut si possible
    let best = null, bestY = -1e9;
    glb.traverse((o) => {
      if (o.isMesh) {
        const b = new THREE.Box3().setFromObject(o);
        if (b.max.y > bestY) { bestY = b.max.y; best = o; }
      }
    });
    world.mochoHead = best || glb;
    return;
  }
  // Fallback minimal (en attendant le vrai modèle)
  const g = new THREE.Group();
  const fur = new THREE.MeshStandardMaterial({ color: 0xffa03c, roughness: 0.75 });
  const body = new THREE.Mesh(new THREE.SphereGeometry(0.2, 18, 14), fur);
  body.position.y = 0.2; body.castShadow = true; g.add(body);
  const head = new THREE.Group();
  head.add(new THREE.Mesh(new THREE.SphereGeometry(0.13, 14, 12), fur));
  const earM = new THREE.MeshStandardMaterial({ color: 0xff9030 });
  const earL = new THREE.Mesh(new THREE.ConeGeometry(0.045, 0.09, 6), earM);
  earL.position.set(-0.07, 0.12, 0);
  const earR = earL.clone(); earR.position.x = 0.07;
  head.add(earL, earR);
  const eyeM = new THREE.MeshStandardMaterial({ color: 0x111 });
  const eL = new THREE.Mesh(new THREE.SphereGeometry(0.022, 8, 8), eyeM);
  eL.position.set(-0.045, 0.02, 0.11);
  const eR = eL.clone(); eR.position.x = 0.045;
  head.add(eL, eR);
  head.position.set(0, 0.42, 0.1);
  g.add(head);
  g.position.set(0, 0, 6.0);
  scene.add(g);
  world.mocho = g;
  world.mochoHead = head;
}

async function buildWorld() {
  const [floorT, wallT, ceilT, sofa, yoyo] = await Promise.all([
    loadTex('assets/textures/texture_de_sol_.jpg', 4, 4),
    loadTex('assets/textures/texture_de_mur.jpg', 2.5, 1.1),
    loadTex('assets/textures/texture_de_plafond.jpg', 3, 3),
    loadGLB('assets/models/canape.glb'),
    loadGLB('assets/models/yoyo.glb'),
  ]);
  makeRoom(floorT, wallT, ceilT);
  makeTable();
  makeCRT();
  await loadMug();
  await loadMocho();

  if (sofa) {
    fitOnFloor(sofa, 2.45);
    sofa.position.x = 0;
    sofa.position.z = ROOM_D - 1.35;
    sofa.traverse((o) => { if (o.isMesh) { o.castShadow = true; o.receiveShadow = true; } });
    scene.add(sofa);
    world.sofa = sofa;
    addColliderFromObject(sofa, 0.1);
    // TV devant le canapé
    if (world.tvGroup) {
      world.tvGroup.position.z = sofa.position.z - 2.55;
      // rebuild collider for new pos
      world.colliders = world.colliders.filter(() => true);
      // re-add solid objects
      world.colliders.length = 0;
      if (world.tvGroup) addColliderFromObject(world.tvGroup, 0.1);
      addColliderFromObject(sofa, 0.1);
    }
  }
  if (yoyo) {
    fitOnFloor(yoyo, 0.14);
    yoyo.position.set(1.7, 0, 5.4);
    const b = new THREE.Box3().setFromObject(yoyo);
    yoyo.position.y = -b.min.y + 0.02;
    yoyo.traverse((o) => { if (o.isMesh) o.castShadow = true; });
    scene.add(yoyo);
    world.yoyo = yoyo;
  }
}

/* ── Input : zones comme Labyrinthe + manettes ── */
const moveVec = { x: 0, y: 0 };
let joyId = null, joyOrigin = { x: 0, y: 0 };
let lookId = null, lastLook = { x: 0, y: 0 };
const keys = {};

canvas.addEventListener('touchstart', (e) => {
  if (state.phase !== 'play') return;
  e.preventDefault();
  for (const t of e.changedTouches) {
    if (t.clientX < innerWidth * 0.5 && joyId === null) {
      joyId = t.identifier; joyOrigin = { x: t.clientX, y: t.clientY };
    } else if (t.clientX >= innerWidth * 0.5 && lookId === null) {
      lookId = t.identifier; lastLook = { x: t.clientX, y: t.clientY };
    }
  }
}, { passive: false });

canvas.addEventListener('touchmove', (e) => {
  if (state.phase !== 'play') return;
  e.preventDefault();
  for (const t of e.changedTouches) {
    if (t.identifier === joyId) {
      let dx = t.clientX - joyOrigin.x, dy = t.clientY - joyOrigin.y;
      const d = Math.hypot(dx, dy) || 1;
      const R = 56;
      if (d > R) { dx = dx / d * R; dy = dy / d * R; }
      // y tactile vers le bas de l'écran → avancer = négatif comme Labyrinthe
      moveVec.x = dx / R;
      moveVec.y = dy / R;
    }
    if (t.identifier === lookId) {
      const dx = t.clientX - lastLook.x, dy = t.clientY - lastLook.y;
      state.yaw -= dx * LOOK_SENS;
      state.pitch -= dy * LOOK_SENS;
      state.pitch = Math.max(-1.25, Math.min(1.25, state.pitch));
      lastLook = { x: t.clientX, y: t.clientY };
    }
  }
}, { passive: false });

canvas.addEventListener('touchend', (e) => {
  for (const t of e.changedTouches) {
    if (t.identifier === joyId) { joyId = null; moveVec.x = 0; moveVec.y = 0; }
    if (t.identifier === lookId) lookId = null;
  }
});

let mouseDown = false, lastMouse = { x: 0, y: 0 };
canvas.addEventListener('mousedown', (e) => { mouseDown = true; lastMouse = { x: e.clientX, y: e.clientY }; });
window.addEventListener('mouseup', () => { mouseDown = false; });
window.addEventListener('mousemove', (e) => {
  if (!mouseDown || state.phase !== 'play') return;
  state.yaw -= (e.clientX - lastMouse.x) * LOOK_SENS * 1.3;
  state.pitch -= (e.clientY - lastMouse.y) * LOOK_SENS * 1.3;
  state.pitch = Math.max(-1.25, Math.min(1.25, state.pitch));
  lastMouse = { x: e.clientX, y: e.clientY };
});
window.addEventListener('keydown', (e) => {
  keys[e.key.toLowerCase()] = true;
  if (e.code === 'KeyE' || e.code === 'Space') tryInteract();
  if (e.code === 'KeyV') { vrMode = !vrMode; onResize(); }
});
window.addEventListener('keyup', (e) => { keys[e.key.toLowerCase()] = false; });

function keyboardMove() {
  let x = 0, y = 0;
  if (keys['w'] || keys['z'] || keys['arrowup']) y -= 1;
  if (keys['s'] || keys['arrowdown']) y += 1;
  if (keys['a'] || keys['q'] || keys['arrowleft']) x -= 1;
  if (keys['d'] || keys['arrowright']) x += 1;
  return { x, y };
}

function applyDZ(v) { return Math.abs(v) < DEAD ? 0 : v; }

let padBtnPrev = {};
function pollPads(dt) {
  const pads = navigator.getGamepads ? navigator.getGamepads() : [];
  state.pads = [];
  let gotMove = false, gotLook = false;
  for (const p of pads) {
    if (!p || !p.connected) continue;
    state.pads.push(p);
    const id = (p.id || '').toLowerCase();
    // sticks
    let mx = applyDZ(p.axes[0] || 0), my = applyDZ(p.axes[1] || 0);
    let lx = applyDZ(p.axes[2] || 0), ly = applyDZ(p.axes[3] || 0);
    if (mx === 0 && my === 0 && p.axes.length >= 4) {
      // joy-con sole stick sometimes on 2/3
    }
    if (!gotMove && (mx || my)) {
      moveVec.x = mx; moveVec.y = my; gotMove = true;
    }
    if (!gotLook && (lx || ly)) {
      state.yaw -= lx * PAD_LOOK * dt;
      state.pitch -= ly * PAD_LOOK * dt;
      state.pitch = Math.max(-1.25, Math.min(1.25, state.pitch));
      gotLook = true;
    }
    // Boutons manette : 0 = A/Cross, 1 = B/Circle
    const a = p.buttons[0]?.pressed;
    const b = p.buttons[1]?.pressed;
    const keyA = p.index + '_a', keyB = p.index + '_b';
    if (a && !padBtnPrev[keyA]) tryInteract();
    if (b && !padBtnPrev[keyB] && state.tvOn) closeTV();
    padBtnPrev[keyA] = a;
    padBtnPrev[keyB] = b;
  }
}

function tryInteract() {
  if (state.tvOn) {
    const label = state.tvChoice === 0 ? 'TikTok' : 'YouTube';
    showMocho(`Miaou ! ${label} ? Moi je préfère les vidéos de souris.`);
    return;
  }
  if (world.tvGroup && camera.position.distanceTo(world.tvGroup.position) < 2.4) {
    openTV();
    return;
  }
  if (state.lookMug >= 2.5 && !mugBody.held) {
    mugBody.held = true;
    state.holding = 'mug';
    showMocho('Fais gaffe à ma tasse !');
  }
}
function openTV() {
  state.tvOn = true;
  tvUi.classList.remove('hidden');
  if (world.tvScreen) {
    world.tvScreen.material.color.setHex(0x1565c0);
    world.tvScreen.material.emissive.setHex(0x1565c0);
    world.tvScreen.material.emissiveIntensity = 0.8;
  }
}
function closeTV() {
  state.tvOn = false;
  tvUi.classList.add('hidden');
  if (world.tvScreen) {
    world.tvScreen.material.color.setHex(0x0a1a40);
    world.tvScreen.material.emissive.setHex(0x0a1a40);
    world.tvScreen.material.emissiveIntensity = 0.35;
  }
}
function showMocho(t) {
  mochoBubble.textContent = t;
  mochoBubble.classList.remove('hidden');
  state.mochoTalk = 4.5;
}

// TV UI – boutons carrés TikTok / YouTube
tvUi.innerHTML = `
  <div class="tv-panel tv-blue">
    <p class="tv-label">SMART TV</p>
    <div class="tv-grid">
      <button type="button" class="tv-square selected" data-i="0">TikTok</button>
      <button type="button" class="tv-square" data-i="1">YouTube</button>
    </div>
    <p class="tv-help">Manette : stick naviguer · A valider · B retour</p>
  </div>`;
tvUi.querySelectorAll('.tv-square').forEach((btn) => {
  btn.addEventListener('click', () => {
    tvUi.querySelectorAll('.tv-square').forEach((b) => b.classList.remove('selected'));
    btn.classList.add('selected');
    state.tvChoice = +btn.dataset.i;
    tryInteract();
  });
});

function updatePhases(dt) {
  state.timer += dt;
  if (state.phase === 'title' && state.timer >= 5) {
    state.phase = 'fade'; state.timer = 0;
    fadeBlack.classList.remove('hidden');
    requestAnimationFrame(() => fadeBlack.classList.add('show'));
  } else if (state.phase === 'fade' && state.timer >= 1.2) {
    state.phase = 'scan'; state.timer = 0;
    titleScreen.classList.add('hidden');
    controllerScan.classList.remove('hidden');
  } else if (state.phase === 'scan') {
    scanDots.textContent = '.'.repeat((Math.floor(state.timer * 2) % 4) + 1);
    pollPads(dt);
    padStatus.textContent = state.pads.length
      ? state.pads.map((p) => p.id.slice(0, 40)).join(' · ')
      : 'Aucune manette – tactile / clavier OK';
    if (state.timer >= 3.5) {
      state.phase = 'play';
      controllerScan.classList.add('hidden');
      fadeBlack.classList.add('hidden');
      fadeBlack.classList.remove('show');
      hud.classList.remove('hidden');
      showMocho('Miaou ! Bienvenue. Je suis Mocho.');
    }
  }
}

function updatePlay(dt) {
  pollPads(dt);
  camera.rotation.set(state.pitch, state.yaw, 0, 'YXZ');

  let input = (moveVec.x || moveVec.y) ? moveVec : keyboardMove();
  const mag = Math.hypot(input.x, input.y);
  if (mag > 0.05) {
    const forward = new THREE.Vector3(0, 0, -1).applyEuler(new THREE.Euler(0, state.yaw, 0, 'YXZ'));
    const right = new THREE.Vector3(1, 0, 0).applyEuler(new THREE.Euler(0, state.yaw, 0, 'YXZ'));
    // Labyrinthe : move.y négatif = avancer
    const move = new THREE.Vector3();
    move.addScaledVector(forward, -input.y);
    move.addScaledVector(right, input.x);
    if (move.lengthSq() > 1) move.normalize();
    const pos = {
      x: camera.position.x + move.x * MOVE_SPEED * Math.min(1, mag) * dt,
      y: EYE,
      z: camera.position.z + move.z * MOVE_SPEED * Math.min(1, mag) * dt,
    };
    resolvePlayer(pos);
    camera.position.x = pos.x;
    camera.position.z = pos.z;
  }
  camera.position.y = EYE;

  // Mug
  if (mugBody.held && world.mugMesh) {
    const hp = new THREE.Vector3(0.22, -0.12, -0.45).applyQuaternion(camera.quaternion).add(camera.position);
    mugBody.pos.copy(hp);
    world.mugMesh.position.copy(hp);
  } else if (world.mugMesh) {
    mugBody.vel.y -= 9.8 * dt;
    mugBody.pos.addScaledVector(mugBody.vel, dt);
    if (mugBody.pos.y < 0.08) {
      mugBody.pos.y = 0.08;
      mugBody.vel.y *= -0.15;
      mugBody.vel.x *= 0.7;
      mugBody.vel.z *= 0.7;
    }
    world.mugMesh.position.copy(mugBody.pos);
  }

  // Regard tasse
  if (world.mugMesh && !mugBody.held) {
    const to = mugBody.pos.clone().sub(camera.position).normalize();
    const dir = new THREE.Vector3(0, 0, -1).applyQuaternion(camera.quaternion);
    if (dir.dot(to) > 0.9 && camera.position.distanceTo(mugBody.pos) < 3)
      state.lookMug += dt;
    else state.lookMug = Math.max(0, state.lookMug - dt * 2);
  }

  // Hints manette-aware
  const pad = state.pads.length > 0;
  if (state.lookMug >= 2.5 && !mugBody.held) {
    interactHint.textContent = pad ? 'A — Attraper la tasse' : 'E / Espace — Attraper la tasse';
    interactHint.classList.remove('hidden');
  } else if (world.tvGroup && camera.position.distanceTo(world.tvGroup.position) < 2.4 && !state.tvOn) {
    interactHint.textContent = pad ? 'A — Allumer la TV' : 'E / Espace — Allumer la TV';
    interactHint.classList.remove('hidden');
  } else interactHint.classList.add('hidden');

  // Mocho suit le regard
  if (world.mocho && world.mochoHead) {
    const t = camera.position.clone();
    t.y = world.mocho.position.y + 0.45;
    try { world.mochoHead.lookAt(t); } catch (_) {}
  }
  if (world.mocho) {
    const breath = 1 + Math.sin(clock.elapsedTime * 2.2) * 0.025;
    world.mocho.scale.y = breath;
  }
  if (state.mochoTalk > 0) {
    state.mochoTalk -= dt;
    if (state.mochoTalk <= 0) mochoBubble.classList.add('hidden');
  }
  if (world.yoyo) world.yoyo.rotation.y += dt * 1.4;
}

function render() {
  if (!vrMode) {
    renderer.setViewport(0, 0, innerWidth, innerHeight);
    renderer.setScissorTest(false);
    renderer.render(scene, camera);
    return;
  }
  // Double écran VR
  stereo.update(camera);
  const w = innerWidth, h = innerHeight, half = w / 2 | 0;
  renderer.setScissorTest(true);
  renderer.setViewport(0, 0, half, h);
  renderer.setScissor(0, 0, half, h);
  renderer.render(scene, stereo.cameraL);
  renderer.setViewport(half, 0, w - half, h);
  renderer.setScissor(half, 0, w - half, h);
  renderer.render(scene, stereo.cameraR);
  renderer.setScissorTest(false);
}

function frame() {
  const dt = Math.min(clock.getDelta(), 0.05);
  frames++; fpsT += dt;
  if (fpsT >= 0.5) {
    fpsEl.textContent = `${Math.round(frames / fpsT)} fps` + (vrMode ? ' · VR' : '');
    frames = 0; fpsT = 0;
  }
  if (state.phase !== 'play') updatePhases(dt);
  else updatePlay(dt);
  render();
  requestAnimationFrame(frame);
}

function onResize() {
  const w = innerWidth, h = innerHeight;
  if (vrMode) {
    camera.aspect = (w / 2) / h;
  } else {
    camera.aspect = w / h;
  }
  camera.updateProjectionMatrix();
  renderer.setSize(w, h);
}
window.addEventListener('resize', onResize);

(async () => {
  titleScreen.classList.remove('hidden');
  await buildWorld();
  requestAnimationFrame(frame);
})();
