/**
 * Chill Place – WebGPU/WebGL (Three.js)
 * Assets 100% locaux – aucune dépendance Google runtime
 * // REMPLACEZ CETTE LIGNE PAR VOTRE PROPRE CLE API
 * const API_KEY = "REMPLACER_PAR_VOTRE_CLE_API";
 */
import * as THREE from 'three';
import { GLTFLoader } from './vendor/GLTFLoader.js';

const API_KEY = "REMPLACER_PAR_VOTRE_CLE_API"; // REMPLACEZ CETTE LIGNE PAR VOTRE PROPRE CLE API

// ── Room dimensions (human scale, meters) ──────────────────────────────────
const ROOM_W = 10;
const ROOM_H = 3.0;
const ROOM_D = 12;

// ── DOM ────────────────────────────────────────────────────────────────────
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

// ── Renderer / Scene ───────────────────────────────────────────────────────
const renderer = new THREE.WebGLRenderer({
  canvas,
  antialias: true,
  powerPreference: 'high-performance',
  alpha: false,
});
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.15;

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x1a1520);
scene.fog = new THREE.Fog(0x1a1520, 14, 28);

const camera = new THREE.PerspectiveCamera(70, window.innerWidth / window.innerHeight, 0.05, 50);
camera.position.set(0, 1.65, 1.8);

// ── Lights ─────────────────────────────────────────────────────────────────
const amb = new THREE.AmbientLight(0xfff0e8, 0.45);
scene.add(amb);
const hemi = new THREE.HemisphereLight(0xc8d8ff, 0x3a2a18, 0.55);
scene.add(hemi);
const sun = new THREE.DirectionalLight(0xffe8d0, 1.1);
sun.position.set(2.5, 5, 3);
sun.castShadow = true;
sun.shadow.mapSize.set(1024, 1024);
sun.shadow.camera.near = 0.5;
sun.shadow.camera.far = 25;
sun.shadow.camera.left = -8;
sun.shadow.camera.right = 8;
sun.shadow.camera.top = 8;
sun.shadow.camera.bottom = -8;
scene.add(sun);
const lamp = new THREE.PointLight(0xffcc88, 0.8, 12, 2);
lamp.position.set(0, 2.6, 6);
scene.add(lamp);

// ── State ──────────────────────────────────────────────────────────────────
const state = {
  phase: 'title', // title | fade | scan | play
  timer: 0,
  yaw: 0,
  pitch: 0,
  move: new THREE.Vector2(),
  look: new THREE.Vector2(),
  pads: [],
  tvOn: false,
  tvChoice: 0, // 0 tiktok 1 youtube
  holding: null, // 'mug' | 'yoyo' | null
  lookAtMug: 0,
  mochoTalk: 0,
};

const keys = {};
const clock = new THREE.Clock();
let frames = 0, fpsT = 0;

// ── World objects refs ─────────────────────────────────────────────────────
const world = {
  sofa: null,
  yoyo: null,
  mug: null,
  mugBody: { pos: new THREE.Vector3(-1.4, 0.72, 5.0), vel: new THREE.Vector3(), held: false },
  yoyoBody: { pos: new THREE.Vector3(1.8, 0.15, 5.5), vel: new THREE.Vector3(), held: false },
  tv: null,
  tvScreen: null,
  mocho: null,
  mochoHead: null,
  table: null,
};

const loader = new GLTFLoader();
const texLoader = new THREE.TextureLoader();

function loadTex(url, repeatX = 1, repeatY = 1) {
  return new Promise((resolve) => {
    texLoader.load(url, (t) => {
      t.colorSpace = THREE.SRGBColorSpace;
      t.wrapS = t.wrapT = THREE.RepeatWrapping;
      t.repeat.set(repeatX, repeatY);
      t.anisotropy = Math.min(8, renderer.capabilities.getMaxAnisotropy());
      resolve(t);
    }, undefined, () => resolve(null));
  });
}

function loadGLB(url) {
  return new Promise((resolve) => {
    loader.load(url, (g) => resolve(g.scene), undefined, () => resolve(null));
  });
}

function fitModel(root, targetMaxSize) {
  const box = new THREE.Box3().setFromObject(root);
  const size = new THREE.Vector3();
  box.getSize(size);
  const maxDim = Math.max(size.x, size.y, size.z) || 1;
  const s = targetMaxSize / maxDim;
  root.scale.setScalar(s);
  // recompute after scale
  box.setFromObject(root);
  const center = new THREE.Vector3();
  box.getCenter(center);
  const min = box.min.clone();
  return { scale: s, center, min, size: size.multiplyScalar(s) };
}

function makeRoom(floorTex, wallTex, ceilTex) {
  const group = new THREE.Group();

  const floorMat = new THREE.MeshStandardMaterial({
    map: floorTex || null,
    color: floorTex ? 0xffffff : 0x5a5048,
    roughness: 0.85,
    metalness: 0.05,
  });
  const wallMat = new THREE.MeshStandardMaterial({
    map: wallTex || null,
    color: wallTex ? 0xffffff : 0x7a7068,
    roughness: 0.9,
    metalness: 0.0,
  });
  const ceilMat = new THREE.MeshStandardMaterial({
    map: ceilTex || null,
    color: ceilTex ? 0xffffff : 0x888080,
    roughness: 1.0,
    metalness: 0.0,
  });

  // Floor
  const floor = new THREE.Mesh(new THREE.PlaneGeometry(ROOM_W, ROOM_D), floorMat);
  floor.rotation.x = -Math.PI / 2;
  floor.position.set(0, 0, ROOM_D / 2);
  floor.receiveShadow = true;
  group.add(floor);

  // Ceiling
  const ceil = new THREE.Mesh(new THREE.PlaneGeometry(ROOM_W, ROOM_D), ceilMat);
  ceil.rotation.x = Math.PI / 2;
  ceil.position.set(0, ROOM_H, ROOM_D / 2);
  group.add(ceil);

  // Walls (inward-facing)
  const mkWall = (w, h, x, y, z, ry) => {
    const m = new THREE.Mesh(new THREE.PlaneGeometry(w, h), wallMat);
    m.position.set(x, y, z);
    m.rotation.y = ry;
    m.receiveShadow = true;
    group.add(m);
  };
  mkWall(ROOM_W, ROOM_H, 0, ROOM_H / 2, ROOM_D, Math.PI);     // back
  mkWall(ROOM_W, ROOM_H, 0, ROOM_H / 2, 0, 0);                   // front
  mkWall(ROOM_D, ROOM_H, -ROOM_W / 2, ROOM_H / 2, ROOM_D / 2, Math.PI / 2);
  mkWall(ROOM_D, ROOM_H, ROOM_W / 2, ROOM_H / 2, ROOM_D / 2, -Math.PI / 2);

  scene.add(group);
}

function makeTable() {
  const g = new THREE.Group();
  const top = new THREE.Mesh(
    new THREE.BoxGeometry(1.4, 0.06, 0.7),
    new THREE.MeshStandardMaterial({ color: 0x4a3828, roughness: 0.55 })
  );
  top.position.y = 0.42;
  top.castShadow = true;
  top.receiveShadow = true;
  g.add(top);
  const legMat = new THREE.MeshStandardMaterial({ color: 0x2e2218 });
  [[-0.6, -0.28], [0.6, -0.28], [-0.6, 0.28], [0.6, 0.28]].forEach(([x, z]) => {
    const leg = new THREE.Mesh(new THREE.BoxGeometry(0.06, 0.42, 0.06), legMat);
    leg.position.set(x, 0.21, z);
    leg.castShadow = true;
    g.add(leg);
  });
  g.position.set(0, 0, 7.2);
  scene.add(g);
  world.table = g;
}

function makeCRT() {
  const g = new THREE.Group();
  const bodyMat = new THREE.MeshStandardMaterial({ color: 0x2a2430, roughness: 0.6 });
  const body = new THREE.Mesh(new THREE.BoxGeometry(0.95, 0.72, 0.55), bodyMat);
  body.position.y = 0.9;
  body.castShadow = true;
  g.add(body);

  const bezel = new THREE.Mesh(
    new THREE.BoxGeometry(0.82, 0.58, 0.04),
    new THREE.MeshStandardMaterial({ color: 0x151018 })
  );
  bezel.position.set(0, 0.92, -0.28);
  g.add(bezel);

  const screenMat = new THREE.MeshStandardMaterial({
    color: 0x050510,
    emissive: 0x000000,
    roughness: 0.3,
  });
  const screen = new THREE.Mesh(new THREE.PlaneGeometry(0.72, 0.48), screenMat);
  screen.position.set(0, 0.92, -0.305);
  g.add(screen);
  world.tvScreen = screen;

  // Cabinet
  const cab = new THREE.Mesh(
    new THREE.BoxGeometry(1.05, 0.5, 0.5),
    new THREE.MeshStandardMaterial({ color: 0x3a2a1c, roughness: 0.7 })
  );
  cab.position.y = 0.25;
  cab.castShadow = true;
  g.add(cab);

  // Buttons
  for (let i = 0; i < 3; i++) {
    const b = new THREE.Mesh(
      new THREE.CylinderGeometry(0.015, 0.015, 0.02, 8),
      new THREE.MeshStandardMaterial({ color: 0x888 })
    );
    b.rotation.x = Math.PI / 2;
    b.position.set(0.28 + i * 0.05, 0.62, -0.28);
    g.add(b);
  }

  g.position.set(0, 0, 8.6);
  scene.add(g);
  world.tv = g;
}

function makeMug() {
  const g = new THREE.Group();
  const ceramic = new THREE.MeshStandardMaterial({
    color: 0xf2e8f8,
    roughness: 0.35,
    metalness: 0.05,
  });
  const body = new THREE.Mesh(new THREE.CylinderGeometry(0.055, 0.06, 0.11, 24), ceramic);
  body.castShadow = true;
  g.add(body);
  const handle = new THREE.Mesh(new THREE.TorusGeometry(0.04, 0.012, 8, 16, Math.PI), ceramic);
  handle.position.set(0.07, 0, 0);
  handle.rotation.y = Math.PI / 2;
  g.add(handle);
  // cartoon outline feel via slightly larger dark shell (optional rim)
  const rim = new THREE.Mesh(
    new THREE.TorusGeometry(0.057, 0.006, 8, 24),
    new THREE.MeshStandardMaterial({ color: 0x1a1020 })
  );
  rim.position.y = 0.052;
  rim.rotation.x = Math.PI / 2;
  g.add(rim);

  g.position.copy(world.mugBody.pos);
  scene.add(g);
  world.mug = g;
}

function makeMocho() {
  const g = new THREE.Group();
  const fur = new THREE.MeshStandardMaterial({ color: 0xffa03c, roughness: 0.8 });
  const body = new THREE.Mesh(new THREE.SphereGeometry(0.22, 20, 16), fur);
  body.position.y = 0.22;
  body.castShadow = true;
  g.add(body);

  const head = new THREE.Group();
  const headMesh = new THREE.Mesh(new THREE.SphereGeometry(0.14, 16, 12), fur);
  head.add(headMesh);
  // ears
  const earMat = new THREE.MeshStandardMaterial({ color: 0xff9030 });
  const earL = new THREE.Mesh(new THREE.ConeGeometry(0.05, 0.1, 6), earMat);
  earL.position.set(-0.08, 0.14, 0);
  const earR = earL.clone();
  earR.position.x = 0.08;
  head.add(earL, earR);
  // eyes
  const eyeMat = new THREE.MeshStandardMaterial({ color: 0x111 });
  const eyeL = new THREE.Mesh(new THREE.SphereGeometry(0.025, 8, 8), eyeMat);
  eyeL.position.set(-0.05, 0.02, 0.12);
  const eyeR = eyeL.clone();
  eyeR.position.x = 0.05;
  head.add(eyeL, eyeR);
  head.position.set(0, 0.48, 0.12);
  g.add(head);

  // tail
  const tail = new THREE.Mesh(new THREE.SphereGeometry(0.06, 10, 8), fur);
  tail.position.set(-0.2, 0.2, -0.18);
  g.add(tail);

  g.position.set(0, 0, 6.0);
  scene.add(g);
  world.mocho = g;
  world.mochoHead = head;
  world.mochoEyes = [eyeL, eyeR];
}

async function buildWorld() {
  const [floorTex, wallTex, ceilTex, sofaGLB, yoyoGLB] = await Promise.all([
    loadTex('assets/textures/texture_de_sol_.jpg', 4, 4),
    loadTex('assets/textures/texture_de_mur.jpg', 3, 1.2),
    loadTex('assets/textures/texture_de_plafond.jpg', 3, 3),
    loadGLB('assets/models/canape.glb'),
    loadGLB('assets/models/yoyo.glb'),
  ]);

  makeRoom(floorTex, wallTex, ceilTex);
  makeTable();
  makeCRT();
  makeMug();
  makeMocho();

  if (sofaGLB) {
    const fit = fitModel(sofaGLB, 2.5); // ~2.5m wide
    // place on floor at back of room
    sofaGLB.position.set(0, -fit.min.y, ROOM_D - 1.4);
    sofaGLB.traverse((o) => {
      if (o.isMesh) {
        o.castShadow = true;
        o.receiveShadow = true;
      }
    });
    scene.add(sofaGLB);
    world.sofa = sofaGLB;
    // TV in front of sofa
    if (world.tv) world.tv.position.z = sofaGLB.position.z - 2.6;
    if (world.table) world.table.position.z = sofaGLB.position.z - 1.5;
  } else {
    // placeholder sofa
    const s = new THREE.Mesh(
      new THREE.BoxGeometry(2.4, 0.8, 1.0),
      new THREE.MeshStandardMaterial({ color: 0x5a4080 })
    );
    s.position.set(0, 0.4, ROOM_D - 1.5);
    s.castShadow = true;
    scene.add(s);
    world.sofa = s;
  }

  if (yoyoGLB) {
    const fit = fitModel(yoyoGLB, 0.16);
    yoyoGLB.position.copy(world.yoyoBody.pos);
    yoyoGLB.position.y = -fit.min.y + 0.02;
    world.yoyoBody.pos.y = yoyoGLB.position.y;
    yoyoGLB.traverse((o) => { if (o.isMesh) o.castShadow = true; });
    scene.add(yoyoGLB);
    world.yoyo = yoyoGLB;
  }
}

// ── Input: keyboard + touch + gamepad ──────────────────────────────────────
window.addEventListener('keydown', (e) => { keys[e.code] = true; });
window.addEventListener('keyup', (e) => { keys[e.code] = false; });

let lookDragging = false;
let lastLook = { x: 0, y: 0 };
canvas.addEventListener('pointerdown', (e) => {
  if (state.phase !== 'play') return;
  if (e.clientX > window.innerWidth * 0.35) {
    lookDragging = true;
    lastLook = { x: e.clientX, y: e.clientY };
    canvas.setPointerCapture(e.pointerId);
  }
});
canvas.addEventListener('pointermove', (e) => {
  if (!lookDragging) return;
  state.look.x += (e.clientX - lastLook.x) * 0.0022;
  state.look.y += (e.clientY - lastLook.y) * 0.0022;
  lastLook = { x: e.clientX, y: e.clientY };
});
canvas.addEventListener('pointerup', () => { lookDragging = false; });

// Virtual stick (left half)
const stick = { active: false, id: null, origin: { x: 0, y: 0 }, value: new THREE.Vector2() };
canvas.addEventListener('pointerdown', (e) => {
  if (state.phase !== 'play') return;
  if (e.clientX < window.innerWidth * 0.35) {
    stick.active = true;
    stick.id = e.pointerId;
    stick.origin = { x: e.clientX, y: e.clientY };
    stick.value.set(0, 0);
  }
});
canvas.addEventListener('pointermove', (e) => {
  if (!stick.active || e.pointerId !== stick.id) return;
  const dx = (e.clientX - stick.origin.x) / 60;
  const dy = (e.clientY - stick.origin.y) / 60;
  stick.value.set(THREE.MathUtils.clamp(dx, -1, 1), THREE.MathUtils.clamp(-dy, -1, 1));
});
canvas.addEventListener('pointerup', (e) => {
  if (e.pointerId === stick.id) {
    stick.active = false;
    stick.value.set(0, 0);
  }
});

function pollGamepads() {
  const pads = navigator.getGamepads ? navigator.getGamepads() : [];
  state.pads = [];
  for (const p of pads) {
    if (p && p.connected) state.pads.push(p);
  }
  if (state.pads.length) {
    const p = state.pads[0];
    const lx = Math.abs(p.axes[0]) > 0.15 ? p.axes[0] : 0;
    const ly = Math.abs(p.axes[1]) > 0.15 ? p.axes[1] : 0;
    // up on stick = negative Y on most pads → invert for forward
    state.move.set(lx, -ly);
    const rx = Math.abs(p.axes[2]) > 0.12 ? p.axes[2] : 0;
    const ry = Math.abs(p.axes[3]) > 0.12 ? p.axes[3] : 0;
    state.look.x += rx * 0.04;
    state.look.y += ry * 0.04;

    // A button = interact / grab
    if (p.buttons[0]?.pressed) tryInteract();
    // B = close TV
    if (p.buttons[1]?.pressed && state.tvOn) closeTV();
    // Shoulder = throw
    if ((p.buttons[6]?.pressed || p.buttons[7]?.pressed) && state.holding) throwHeld();
  } else {
    // keyboard WASD
    let mx = 0, my = 0;
    if (keys['KeyW'] || keys['ArrowUp']) my += 1;
    if (keys['KeyS'] || keys['ArrowDown']) my -= 1;
    if (keys['KeyA'] || keys['ArrowLeft']) mx -= 1;
    if (keys['KeyD'] || keys['ArrowRight']) mx += 1;
    if (stick.active) {
      mx = stick.value.x;
      my = stick.value.y;
    }
    state.move.set(mx, my);
  }
}

// Device orientation (gyro) for VR-ish look
let gyroEnabled = false;
let baseOrient = null;
function enableGyro() {
  const handler = (e) => {
    if (state.phase !== 'play') return;
    // alpha z, beta x, gamma y
    if (e.beta == null || e.gamma == null) return;
    if (!baseOrient) {
      baseOrient = { beta: e.beta, gamma: e.gamma, alpha: e.alpha || 0 };
    }
    const dBeta = (e.beta - baseOrient.beta) * (Math.PI / 180);
    const dAlpha = ((e.alpha || 0) - baseOrient.alpha) * (Math.PI / 180);
    // Map: turn phone → yaw/pitch (no artificial clamp beyond comfort)
    state.yaw = -dAlpha;
    state.pitch = THREE.MathUtils.clamp(-dBeta * 0.85, -1.4, 1.4);
  };
  if (window.DeviceOrientationEvent) {
    if (typeof DeviceOrientationEvent.requestPermission === 'function') {
      DeviceOrientationEvent.requestPermission().then((r) => {
        if (r === 'granted') {
          window.addEventListener('deviceorientation', handler);
          gyroEnabled = true;
        }
      }).catch(() => {});
    } else {
      window.addEventListener('deviceorientation', handler);
      gyroEnabled = true;
    }
  }
}

// ── Interactions ───────────────────────────────────────────────────────────
const raycaster = new THREE.Raycaster();
function forwardRay() {
  const dir = new THREE.Vector3(0, 0, -1).applyQuaternion(camera.quaternion);
  raycaster.set(camera.position, dir);
}

function tryInteract() {
  if (state.tvOn) {
    // confirm TV choice
    const label = state.tvChoice === 0 ? 'TikTok' : 'YouTube';
    showMocho(`Miaou ! Tu regardes ${label} ? Moi je préfère les vidéos de souris 🐭`);
    return;
  }
  forwardRay();
  // TV proximity
  if (world.tv && camera.position.distanceTo(world.tv.position) < 2.2) {
    openTV();
    return;
  }
  // Grab mug if looking
  if (state.lookAtMug >= 2.5 && !state.holding) {
    state.holding = 'mug';
    world.mugBody.held = true;
    world.mugBody.vel.set(0, 0, 0);
    showMocho('Attention à ma tasse préférée !');
  }
}

function openTV() {
  state.tvOn = true;
  tvUi.classList.remove('hidden');
  if (world.tvScreen) {
    world.tvScreen.material.emissive.setHex(0x1a4a9a);
    world.tvScreen.material.color.setHex(0x1a4a9a);
  }
}
function closeTV() {
  state.tvOn = false;
  tvUi.classList.add('hidden');
  if (world.tvScreen) {
    world.tvScreen.material.emissive.setHex(0x000000);
    world.tvScreen.material.color.setHex(0x050510);
  }
}

function throwHeld() {
  if (state.holding === 'mug') {
    const dir = new THREE.Vector3(0, 0.2, -1).applyQuaternion(camera.quaternion);
    world.mugBody.held = false;
    world.mugBody.vel.copy(dir.multiplyScalar(4));
    state.holding = null;
  }
}

function showMocho(text) {
  mochoBubble.textContent = text;
  mochoBubble.classList.remove('hidden');
  state.mochoTalk = 4.0;
}

// TV UI buttons
tvUi.querySelectorAll('.tv-btn').forEach((btn, i) => {
  btn.addEventListener('click', () => {
    tvUi.querySelectorAll('.tv-btn').forEach((b) => b.classList.remove('selected'));
    btn.classList.add('selected');
    state.tvChoice = i;
    tryInteract();
  });
});

window.addEventListener('keydown', (e) => {
  if (e.code === 'KeyE' || e.code === 'Space') tryInteract();
  if (e.code === 'KeyF' && state.holding) throwHeld();
  if (e.code === 'Escape' && state.tvOn) closeTV();
});

// ── Phases ─────────────────────────────────────────────────────────────────
function startTitle() {
  state.phase = 'title';
  state.timer = 0;
  titleScreen.classList.remove('hidden');
  fadeBlack.classList.add('hidden');
  controllerScan.classList.add('hidden');
  hud.classList.add('hidden');
}

function updatePhases(dt) {
  state.timer += dt;
  if (state.phase === 'title') {
    if (state.timer >= 5) {
      state.phase = 'fade';
      state.timer = 0;
      fadeBlack.classList.remove('hidden');
      requestAnimationFrame(() => fadeBlack.classList.add('show'));
    }
  } else if (state.phase === 'fade') {
    if (state.timer >= 1.3) {
      state.phase = 'scan';
      state.timer = 0;
      titleScreen.classList.add('hidden');
      controllerScan.classList.remove('hidden');
      enableGyro();
    }
  } else if (state.phase === 'scan') {
    const n = Math.floor(state.timer * 2) % 4;
    scanDots.textContent = '.'.repeat(n + 1);
    pollGamepads();
    padStatus.textContent = state.pads.length
      ? `${state.pads.length} manette(s) : ${state.pads.map((p) => p.id.slice(0, 32)).join(' · ')}`
      : 'Aucun détecté pour l’instant (clavier / tactile OK)';
    if (state.timer >= 3.5) {
      state.phase = 'play';
      controllerScan.classList.add('hidden');
      fadeBlack.classList.add('hidden');
      fadeBlack.classList.remove('show');
      hud.classList.remove('hidden');
      showMocho('Miaou ! Bienvenue dans Chill Place. Je suis Mocho 🐱');
    }
  }
}

// ── Play update ────────────────────────────────────────────────────────────
function updatePlay(dt) {
  pollGamepads();

  // Look from drag (accumulate then clear)
  state.yaw -= state.look.x;
  state.pitch -= state.look.y;
  state.pitch = THREE.MathUtils.clamp(state.pitch, -1.35, 1.35);
  state.look.set(0, 0);

  if (!gyroEnabled) {
    camera.rotation.order = 'YXZ';
    camera.rotation.y = state.yaw;
    camera.rotation.x = state.pitch;
  } else {
    camera.rotation.order = 'YXZ';
    camera.rotation.y = state.yaw;
    camera.rotation.x = state.pitch;
  }

  // Move
  const forward = new THREE.Vector3(0, 0, -1).applyAxisAngle(new THREE.Vector3(0, 1, 0), state.yaw);
  const right = new THREE.Vector3(1, 0, 0).applyAxisAngle(new THREE.Vector3(0, 1, 0), state.yaw);
  const speed = 2.8 * dt;
  camera.position.addScaledVector(forward, state.move.y * speed);
  camera.position.addScaledVector(right, state.move.x * speed);
  camera.position.y = 1.65;
  camera.position.x = THREE.MathUtils.clamp(camera.position.x, -ROOM_W / 2 + 0.4, ROOM_W / 2 - 0.4);
  camera.position.z = THREE.MathUtils.clamp(camera.position.z, 0.5, ROOM_D - 0.5);

  // Mug physics
  const mb = world.mugBody;
  if (mb.held && world.mug) {
    const holdPos = new THREE.Vector3(0.25, -0.15, -0.5).applyQuaternion(camera.quaternion).add(camera.position);
    mb.pos.copy(holdPos);
    world.mug.position.copy(mb.pos);
  } else if (world.mug) {
    mb.vel.y -= 9.8 * dt;
    mb.pos.addScaledVector(mb.vel, dt);
    if (mb.pos.y < 0.08) {
      mb.pos.y = 0.08;
      mb.vel.y *= -0.2;
      mb.vel.x *= 0.7;
      mb.vel.z *= 0.7;
    }
    mb.pos.x = THREE.MathUtils.clamp(mb.pos.x, -ROOM_W / 2 + 0.2, ROOM_W / 2 - 0.2);
    mb.pos.z = THREE.MathUtils.clamp(mb.pos.z, 0.3, ROOM_D - 0.3);
    world.mug.position.copy(mb.pos);
  }

  // Look-at mug for grab (3s)
  forwardRay();
  let lookingMug = false;
  if (world.mug && !mb.held) {
    const toMug = world.mug.position.clone().sub(camera.position).normalize();
    const dir = new THREE.Vector3(0, 0, -1).applyQuaternion(camera.quaternion);
    if (dir.dot(toMug) > 0.92 && camera.position.distanceTo(world.mug.position) < 3) {
      lookingMug = true;
      state.lookAtMug += dt;
    }
  }
  if (!lookingMug) state.lookAtMug = Math.max(0, state.lookAtMug - dt * 2);

  if (state.lookAtMug >= 2.5 && !state.holding) {
    interactHint.textContent = 'A / E — Attraper la tasse';
    interactHint.classList.remove('hidden');
  } else if (world.tv && camera.position.distanceTo(world.tv.position) < 2.2 && !state.tvOn) {
    interactHint.textContent = 'A / E — Allumer la TV';
    interactHint.classList.remove('hidden');
  } else {
    interactHint.classList.add('hidden');
  }

  // Mocho looks at player
  if (world.mochoHead) {
    const target = camera.position.clone();
    target.y = world.mocho.position.y + 0.48;
    world.mochoHead.lookAt(target);
  }
  if (world.mocho) {
    const breath = 1 + Math.sin(clock.elapsedTime * 2.2) * 0.03;
    world.mocho.scale.set(1, breath, 1);
  }
  // blink
  if (world.mochoEyes) {
    const blink = (Math.sin(clock.elapsedTime * 0.7) > 0.92);
    world.mochoEyes.forEach((e) => { e.scale.y = blink ? 0.15 : 1; });
  }

  if (state.mochoTalk > 0) {
    state.mochoTalk -= dt;
    if (state.mochoTalk <= 0) mochoBubble.classList.add('hidden');
  }

  // Yoyo idle spin
  if (world.yoyo) world.yoyo.rotation.y += dt * 1.5;
}

// ── Loop ───────────────────────────────────────────────────────────────────
function frame() {
  const dt = Math.min(clock.getDelta(), 0.05);
  frames++;
  fpsT += dt;
  if (fpsT >= 0.5) {
    fpsEl.textContent = `${Math.round(frames / fpsT)} fps`;
    frames = 0;
    fpsT = 0;
  }

  if (state.phase !== 'play') updatePhases(dt);
  else updatePlay(dt);

  renderer.render(scene, camera);
  requestAnimationFrame(frame);
}

window.addEventListener('resize', () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.setSize(window.innerWidth, window.innerHeight);
});

// Boot
(async () => {
  startTitle();
  await buildWorld();
  requestAnimationFrame(frame);
})();
