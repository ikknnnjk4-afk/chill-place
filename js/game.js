/* ============================================================
   LE COULOIR — moteur de jeu (couloir pseudo-3D en CSS 3D)
   100% local, aucune dépendance externe, prêt pour packaging APK.
   ============================================================ */

(() => {
  'use strict';

  // ---------- Constantes de géométrie (doivent matcher css/style.css) ----------
  const CORRIDOR_HALF_WIDTH  = 150;   // marge de sécurité vs murs (160 - rayon joueur)
  const CORRIDOR_LENGTH      = 6000;
  const GIRL_Z                = 5750;  // distance depuis le départ jusqu'à la fille
  const JUMPSCARE_TRIGGER_DIST = 95;   // ~1 mètre stylisé
  const TENSION_START_DIST   = 2600;   // distance à partir de laquelle le coeur commence

  const MAX_SPEED   = 78;     // unités / seconde à pleine poussée
  const STRAFE_SPEED = 60;
  const FOOTSTEP_DISTANCE = 46; // distance parcourue entre deux pas
  const OPTICAL_START_DIST = 1800;
  const OPTICAL_PEAK_DIST = 220;

  // ---------- État ----------
  const state = {
    z: 0,            // avancement dans le couloir (0 = départ)
    x: 0,             // latéral
    yaw: 0,           // regard horizontal (radians)
    pitch: 0,
    distSinceStep: 0,
    running: false,
    jumpscareStarted: false,
    ended: false,
    shake: 0,
    shakePhase: 0,
  };

  // ---------- DOM ----------
  const world      = document.getElementById('world');
  const girlWrap    = document.getElementById('girlWrap');
  const girl        = document.getElementById('girl');
  const girlLight   = document.getElementById('girlLight');
  const scene       = document.getElementById('scene');
  const opticalHalo = document.getElementById('opticalHalo');
  const opticalDisplacement = document.getElementById('opticalDisplacement');
  const titleScreen = document.getElementById('titleScreen');
  const flash       = document.getElementById('flash');
  const blackout    = document.getElementById('blackout');
  const fxCanvas     = document.getElementById('fxCanvas');
  const endScreen    = document.getElementById('endScreen');
  const ctx2d = fxCanvas.getContext('2d');

  function resizeCanvas() {
    fxCanvas.width = window.innerWidth;
    fxCanvas.height = window.innerHeight;
  }
  window.addEventListener('resize', resizeCanvas);
  resizeCanvas();

  // ============================================================
  // JOYSTICK VIRTUEL (mouvement)
  // ============================================================
  const joyZone  = document.getElementById('joystickZone');
  const joyBase  = document.getElementById('joystickBase');
  const joyThumb = document.getElementById('joystickThumb');
  let joyActive = false;
  let joyId = null;
  let joyCenter = { x:0, y:0 };
  let joyVec = { x:0, y:0 }; // -1..1

  function joyBaseRect() {
    const r = joyBase.getBoundingClientRect();
    return { x: r.left + r.width/2, y: r.top + r.height/2, radius: r.width/2 };
  }

  function joyStart(id, clientX, clientY) {
    joyActive = true;
    joyId = id;
    joyCenter = joyBaseRect();
    joyBase.classList.add('active');
    joyMove(clientX, clientY);
  }
  function joyMove(clientX, clientY) {
    if (!joyActive) return;
    let dx = clientX - joyCenter.x;
    let dy = clientY - joyCenter.y;
    const max = joyCenter.radius;
    const dist = Math.min(Math.hypot(dx,dy), max);
    const angle = Math.atan2(dy,dx);
    dx = Math.cos(angle)*dist;
    dy = Math.sin(angle)*dist;
    joyThumb.style.transform = `translate(${dx-26}px, ${dy-26}px)`;
    joyVec.x = dx/max;
    joyVec.y = dy/max;
  }
  function joyEnd() {
    joyActive = false;
    joyId = null;
    joyVec.x = 0; joyVec.y = 0;
    joyThumb.style.transform = 'translate(-50%,-50%)';
    joyBase.classList.remove('active');
  }

  joyZone.addEventListener('touchstart', (e) => {
    const t = e.changedTouches[0];
    joyStart(t.identifier, t.clientX, t.clientY);
    e.preventDefault();
  }, { passive:false });
  joyZone.addEventListener('touchmove', (e) => {
    for (const t of e.changedTouches) {
      if (t.identifier === joyId) joyMove(t.clientX, t.clientY);
    }
    e.preventDefault();
  }, { passive:false });
  joyZone.addEventListener('touchend', (e) => {
    for (const t of e.changedTouches) {
      if (t.identifier === joyId) joyEnd();
    }
  });
  joyZone.addEventListener('touchcancel', joyEnd);

  // support souris (test desktop)
  let mouseJoyDown = false;
  joyZone.addEventListener('mousedown', (e) => { mouseJoyDown = true; joyStart('mouse', e.clientX, e.clientY); });
  window.addEventListener('mousemove', (e) => { if (mouseJoyDown) joyMove(e.clientX, e.clientY); });
  window.addEventListener('mouseup', () => { if (mouseJoyDown) { mouseJoyDown = false; joyEnd(); } });

  // ============================================================
  // ZONE DE REGARD (drag pour tourner la tête, côté droit)
  // ============================================================
  const lookZone = document.getElementById('lookZone');
  let lookId = null;
  let lookLast = { x:0, y:0 };

  function lookStart(id, x, y) { lookId = id; lookLast = { x, y }; }
  function lookMove(x, y) {
    const dx = x - lookLast.x;
    const dy = y - lookLast.y;
    lookLast = { x, y };
    state.yaw   -= dx * 0.0026;
    state.pitch -= dy * 0.0022;
    state.pitch = Math.max(-0.35, Math.min(0.35, state.pitch));
    state.yaw   = Math.max(-1.0, Math.min(1.0, state.yaw));
  }
  lookZone.addEventListener('touchstart', (e) => {
    const t = e.changedTouches[0];
    lookStart(t.identifier, t.clientX, t.clientY);
    e.preventDefault();
  }, { passive:false });
  lookZone.addEventListener('touchmove', (e) => {
    for (const t of e.changedTouches) {
      if (t.identifier === lookId) lookMove(t.clientX, t.clientY);
    }
    e.preventDefault();
  }, { passive:false });
  lookZone.addEventListener('touchend', (e) => {
    for (const t of e.changedTouches) if (t.identifier === lookId) lookId = null;
  });

  let mouseLookDown = false;
  lookZone.addEventListener('mousedown', (e) => { mouseLookDown = true; lookStart('m', e.clientX, e.clientY); });
  window.addEventListener('mousemove', (e) => { if (mouseLookDown) lookMove(e.clientX, e.clientY); });
  window.addEventListener('mouseup', () => { mouseLookDown = false; });

  // Clavier (desktop, pratique)
  const keys = {};
  window.addEventListener('keydown', (e) => keys[e.key.toLowerCase()] = true);
  window.addEventListener('keyup', (e) => keys[e.key.toLowerCase()] = false);

  // ============================================================
  // DÉMARRAGE
  // ============================================================
  function startGame() {
    if (state.running) return;
    AudioEngine.init();
    titleScreen.classList.add('hidden');
    state.running = true;
    requestAnimationFrame(loop);
  }
  titleScreen.addEventListener('touchstart', startGame, { passive:true });
  titleScreen.addEventListener('mousedown', startGame);

  // ============================================================
  // BOUCLE PRINCIPALE
  // ============================================================
  let lastTime = performance.now();

  function loop(now) {
    const dt = Math.min(0.05, (now - lastTime) / 1000);
    lastTime = now;

    if (!state.jumpscareStarted && !state.ended) {
      update(dt);
    }
    render();
    requestAnimationFrame(loop);
  }

  function update(dt) {
    // clavier optionnel (desktop)
    let inputX = joyVec.x;
    let inputY = joyVec.y;
    if (keys['z'] || keys['w'] || keys['arrowup'])    inputY -= 1;
    if (keys['s'] || keys['arrowdown'])                inputY += 1;
    if (keys['q'] || keys['a'] || keys['arrowleft'])   inputX -= 1;
    if (keys['d'] || keys['arrowright'])               inputX += 1;
    inputX = Math.max(-1, Math.min(1, inputX));
    inputY = Math.max(-1, Math.min(1, inputY));

    const forward = -inputY; // pousser le joystick vers le haut = avancer
    const strafe  = inputX;

    // déplacement relatif au regard (yaw) pour rester cohérent visuellement
    const cosY = Math.cos(state.yaw), sinY = Math.sin(state.yaw);
    const moveZ = forward * MAX_SPEED * dt;
    const moveX = strafe  * STRAFE_SPEED * dt;

    const dz = moveZ * cosY - moveX * sinY;
    const dx = moveZ * sinY + moveX * cosY;

    const prevZ = state.z;
    state.z = Math.max(0, Math.min(CORRIDOR_LENGTH - JUMPSCARE_TRIGGER_DIST + 40, state.z + dz));
    state.x = Math.max(-CORRIDOR_HALF_WIDTH, Math.min(CORRIDOR_HALF_WIDTH, state.x + dx));

    const traveled = Math.abs(state.z - prevZ) + Math.abs(dx);
    const movementStrength = Math.min(1, traveled / Math.max(dt * MAX_SPEED, 0.001));
    if (traveled > 0.02) {
      state.shake = Math.min(1, state.shake + movementStrength * dt * 3.2);
      state.shakePhase += dt * (7 + movementStrength * 10);
    }
    state.shake *= Math.pow(0.055, dt);
    if (traveled > 0.02) {
      state.distSinceStep += traveled;
      if (state.distSinceStep >= FOOTSTEP_DISTANCE) {
        state.distSinceStep = 0;
        AudioEngine.footstep();
      }
    }

    // ---- tension / battement de coeur ----
    const remaining = GIRL_Z - state.z;
    let t = 0;
    if (remaining < TENSION_START_DIST) {
      t = 1 - (remaining - JUMPSCARE_TRIGGER_DIST) / (TENSION_START_DIST - JUMPSCARE_TRIGGER_DIST);
      t = Math.max(0, Math.min(1, t));
    }
    AudioEngine.setTension(t);
    girlLight.style.opacity = 0.35 + t*0.65;
    girl.style.filter = `brightness(${0.55+t*0.35}) saturate(${0.7+t*0.3}) drop-shadow(0 0 ${10+t*22}px rgba(190,205,255,${0.2+t*0.35}))`;

    // vignette qui se resserre avec la tension (accéléré coeur -> vision qui se rétrécit)
    document.getElementById('vignette').style.filter = `brightness(${1 - t*0.18})`;

    // ---- illusion d'optique progressive ----
    let distortion = 0;
    if (remaining < OPTICAL_START_DIST) {
      distortion = 1 - (remaining - OPTICAL_PEAK_DIST) / (OPTICAL_START_DIST - OPTICAL_PEAK_DIST);
      distortion = Math.max(0, Math.min(1, distortion));
      distortion = distortion * distortion * (3 - 2 * distortion);
    }
    const distortionScale = distortion * distortion * 30;
    opticalDisplacement.setAttribute('scale', distortionScale.toFixed(2));
    scene.style.filter = distortion > 0.025 ? 'url(#opticalWarp)' : 'none';
    opticalHalo.style.opacity = (distortion * distortion * 0.72).toFixed(3);
    opticalHalo.style.transform = `scale(${(1.04 + distortion * 0.08).toFixed(3)})`;

    // ---- déclenchement du jumpscare ----
    if (remaining <= JUMPSCARE_TRIGGER_DIST && !state.jumpscareStarted) {
      triggerJumpscare();
    }
  }

  function render() {
    const transform =
      `translateZ(${state.z}px) ` +
      `rotateY(${state.yaw}rad) rotateX(${state.pitch}rad) ` +
      `translateX(${-state.x}px)`;
    world.style.transform = transform;

    const shake = state.shake;
    const horizontal = Math.sin(state.shakePhase * 1.07) * shake * 3.2;
    const vertical = Math.cos(state.shakePhase * 1.63) * shake * 2.4;
    scene.style.transform = `translate3d(${horizontal.toFixed(2)}px, ${vertical.toFixed(2)}px, 0)`;
  }

  // ============================================================
  // SÉQUENCE JUMPSCARE
  // ============================================================
  function triggerJumpscare() {
    state.jumpscareStarted = true;
    AudioEngine.tensionSurge();

    const duration = 950; // ms - la fille charge
    const start = performance.now();
    const startScale = 1;
    const endScale = 9;

    function chargeFrame(now) {
      const p = Math.min(1, (now - start) / duration);
      const eased = p * p * p; // accélération violente
      const scale = startScale + (endScale - startScale) * eased;
      girl.style.transform = `translateY(-125px) scale(${scale})`;
      girl.style.filter = `brightness(${1.1}) saturate(1.1) drop-shadow(0 0 30px rgba(255,255,255,0.5))`;
      // caméra recule légèrement / secoue pour accentuer le choc
      const shake = eased * 14;
      world.style.transform += ` translateX(${(Math.random()-0.5)*shake}px) translateY(${(Math.random()-0.5)*shake}px)`;

      if (p < 1) {
        requestAnimationFrame(chargeFrame);
      } else {
        impact();
      }
    }
    requestAnimationFrame(chargeFrame);
  }

  function impact() {
    AudioEngine.screamHit();
    flash.classList.remove('hidden');
    flash.classList.add('hit');

    setTimeout(() => {
      blackout.classList.remove('hidden');
      blackout.classList.add('on');
      startNoiseVHS();
      AudioEngine.startStatic();
      setTimeout(endSequence, 2600);
    }, 160);
  }

  // ---------- bruit VHS / TV pendant le noir ----------
  let noiseRunning = false;
  function startNoiseVHS() {
    noiseRunning = true;
    fxCanvas.style.opacity = '0.85';
    drawNoise();
  }
  function stopNoiseVHS() {
    noiseRunning = false;
    fxCanvas.style.opacity = '0';
  }
  function drawNoise() {
    if (!noiseRunning) return;
    const w = fxCanvas.width, h = fxCanvas.height;
    const imgData = ctx2d.createImageData(w, h);
    const buf = imgData.data;
    for (let i = 0; i < buf.length; i += 4) {
      const v = (Math.random() * 255) | 0;
      buf[i] = v; buf[i+1] = v; buf[i+2] = v; buf[i+3] = 255;
    }
    ctx2d.putImageData(imgData, 0, 0);
    // quelques lignes de scan horizontales
    ctx2d.fillStyle = 'rgba(0,0,0,0.15)';
    for (let y = 0; y < h; y += 3) ctx2d.fillRect(0, y, w, 1);
    requestAnimationFrame(drawNoise);
  }

  // ---------- séquence finale : lumière blanche + moniteur cardiaque ----------
  function endSequence() {
    stopNoiseVHS();
    AudioEngine.stopStatic();
    state.ended = true;

    endScreen.classList.remove('hidden');
    // forcer reflow pour activer la transition CSS
    void endScreen.offsetWidth;

    AudioEngine.startMonitor(58);

    setTimeout(() => {
      const endText = document.getElementById('endText');
      endText.textContent = '...';
      endText.style.opacity = '1';
    }, 1800);

    setTimeout(() => {
      AudioEngine.stopMonitor();
    }, 9000);
  }

})();
