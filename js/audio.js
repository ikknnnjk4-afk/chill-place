/* ============================================================
   AUDIO — tout est synthétisé via WebAudio, aucune ressource
   externe, aucun fichier son requis : 100% offline / APK-safe.
   ============================================================ */

const AudioEngine = (() => {
  let ctx = null;
  let master, ambientGain, heartGain, droneGain, noiseGain;
  let heartLoopHandle = null;
  let heartRate = 0;      // battements par minute
  let heartVolume = 0;    // 0..1 cible
  let heartVolCurrent = 0;
  let started = false;
  let footToggle = false;

  function init() {
    if (started) return;
    started = true;
    ctx = new (window.AudioContext || window.webkitAudioContext)();

    master = ctx.createGain();
    master.gain.value = 0.9;
    master.connect(ctx.destination);

    ambientGain = ctx.createGain();
    ambientGain.gain.value = 0.0;
    ambientGain.connect(master);

    heartGain = ctx.createGain();
    heartGain.gain.value = 0.0;
    heartGain.connect(master);

    droneGain = ctx.createGain();
    droneGain.gain.value = 0.05;
    droneGain.connect(master);

    noiseGain = ctx.createGain();
    noiseGain.gain.value = 0.0;
    noiseGain.connect(master);

    startAmbientDrone();
    scheduleHeartLoop();
  }

  // Bruit blanc / rose utilitaire
  function makeNoiseBuffer(duration = 1) {
    const len = Math.floor(ctx.sampleRate * duration);
    const buf = ctx.createBuffer(1, len, ctx.sampleRate);
    const data = buf.getChannelData(0);
    let b0=0,b1=0,b2=0;
    for (let i=0;i<len;i++){
      const white = Math.random()*2-1;
      b0 = 0.99765*b0 + white*0.0990460;
      b1 = 0.96300*b1 + white*0.2965164;
      b2 = 0.57000*b2 + white*1.0526913;
      data[i] = (b0+b1+b2+white*0.1848) * 0.15;
    }
    return buf;
  }

  // Drone d'ambiance grave et menaçant, en boucle
  function startAmbientDrone() {
    const osc1 = ctx.createOscillator();
    osc1.type = 'sine';
    osc1.frequency.value = 42;
    const osc2 = ctx.createOscillator();
    osc2.type = 'sine';
    osc2.frequency.value = 45.2; // léger battement
    const lfo = ctx.createOscillator();
    lfo.frequency.value = 0.08;
    const lfoGain = ctx.createGain();
    lfoGain.gain.value = 0.03;
    lfo.connect(lfoGain);
    lfoGain.connect(droneGain.gain);

    osc1.connect(droneGain);
    osc2.connect(droneGain);
    osc1.start(); osc2.start(); lfo.start();

    // bruit filtré très bas en fond, texture "air de couloir"
    const src = ctx.createBufferSource();
    src.buffer = makeNoiseBuffer(4);
    src.loop = true;
    const lp = ctx.createBiquadFilter();
    lp.type = 'lowpass';
    lp.frequency.value = 260;
    const hg = ctx.createGain();
    hg.gain.value = 0.35;
    src.connect(lp); lp.connect(hg); hg.connect(ambientGain);
    ambientGain.gain.value = 0.06;
    src.start();
  }

  // Un battement de coeur "lub-dub"
  function heartThump(time, intensity) {
    [ [time,60,0.9], [time+0.14,46,0.55] ].forEach(([t, freq, amp]) => {
      const osc = ctx.createOscillator();
      osc.type = 'sine';
      osc.frequency.setValueAtTime(freq, t);
      const g = ctx.createGain();
      g.gain.setValueAtTime(0.0001, t);
      g.gain.exponentialRampToValueAtTime(amp*intensity, t+0.02);
      g.gain.exponentialRampToValueAtTime(0.0001, t+0.22);
      osc.connect(g); g.connect(heartGain);
      osc.start(t); osc.stop(t+0.3);
    });
  }

  function scheduleHeartLoop() {
    const tick = () => {
      if (!ctx) return;
      heartVolCurrent += (heartVolume - heartVolCurrent) * 0.15;
      heartGain.gain.setTargetAtTime(heartVolCurrent, ctx.currentTime, 0.1);
      if (heartVolCurrent > 0.01) {
        heartThump(ctx.currentTime + 0.02, Math.min(1, 0.4 + heartVolCurrent));
      }
      const bpm = 55 + heartRate * 90; // 55 -> 145 bpm
      const interval = 60000 / bpm;
      heartLoopHandle = setTimeout(tick, interval);
    };
    tick();
  }

  function setTension(t) {
    // t = 0..1 (0 = loin, 1 = juste devant elle)
    heartVolume = Math.min(1, t);
    heartRate = t;
    droneGain.gain.setTargetAtTime(0.05 + t*0.12, ctx ? ctx.currentTime : 0, 0.3);
  }

  function footstep() {
    if (!ctx) return;
    footToggle = !footToggle;
    const t = ctx.currentTime;
    const src = ctx.createBufferSource();
    src.buffer = makeNoiseBuffer(0.18);
    const bp = ctx.createBiquadFilter();
    bp.type = 'bandpass';
    bp.frequency.value = footToggle ? 340 : 300;
    bp.Q.value = 0.9;
    const g = ctx.createGain();
    g.gain.setValueAtTime(0.0001, t);
    g.gain.exponentialRampToValueAtTime(0.5, t+0.01);
    g.gain.exponentialRampToValueAtTime(0.0001, t+0.16);
    src.connect(bp); bp.connect(g); g.connect(master);
    src.start(t); src.stop(t+0.2);
  }

  // Rugissement / montée sonore juste avant le screamer
  function tensionSurge() {
    if (!ctx) return;
    const t = ctx.currentTime;
    const osc = ctx.createOscillator();
    osc.type = 'sawtooth';
    osc.frequency.setValueAtTime(60, t);
    osc.frequency.exponentialRampToValueAtTime(280, t+0.9);
    const g = ctx.createGain();
    g.gain.setValueAtTime(0.0001, t);
    g.gain.exponentialRampToValueAtTime(0.5, t+0.85);
    g.gain.exponentialRampToValueAtTime(0.0001, t+1.05);
    const lp = ctx.createBiquadFilter();
    lp.type = 'lowpass'; lp.frequency.value = 900;
    osc.connect(lp); lp.connect(g); g.connect(master);
    osc.start(t); osc.stop(t+1.1);
  }

  // Cri strident au moment de l'impact
  function screamHit() {
    if (!ctx) return;
    const t = ctx.currentTime;
    const osc = ctx.createOscillator();
    osc.type = 'sawtooth';
    osc.frequency.setValueAtTime(1800, t);
    osc.frequency.exponentialRampToValueAtTime(90, t+0.9);
    const g = ctx.createGain();
    g.gain.setValueAtTime(0.7, t);
    g.gain.exponentialRampToValueAtTime(0.0001, t+1.0);
    const dist = ctx.createWaveShaper();
    const curve = new Float32Array(44100);
    for (let i=0;i<44100;i++){ const x=i*2/44100-1; curve[i]=Math.tanh(x*4); }
    dist.curve = curve;
    osc.connect(dist); dist.connect(g); g.connect(master);
    osc.start(t); osc.stop(t+1.0);

    // bruit blanc explosif superposé
    const src = ctx.createBufferSource();
    src.buffer = makeNoiseBuffer(0.6);
    const ng = ctx.createGain();
    ng.gain.setValueAtTime(0.6, t);
    ng.gain.exponentialRampToValueAtTime(0.0001, t+0.55);
    src.connect(ng); ng.connect(master);
    src.start(t);

    // stop heartbeat & drone brutalement
    heartVolume = 0; heartVolCurrent = 0;
    if (ctx) {
      heartGain.gain.cancelScheduledValues(t);
      heartGain.gain.setValueAtTime(0, t+0.05);
      droneGain.gain.cancelScheduledValues(t);
      droneGain.gain.setValueAtTime(0, t+0.3);
    }
  }

  // Bruit statique TV continu (pendant l'écran noir)
  let staticSrc = null;
  function startStatic() {
    if (!ctx) return;
    stopStatic();
    staticSrc = ctx.createBufferSource();
    staticSrc.buffer = makeNoiseBuffer(2);
    staticSrc.loop = true;
    const hp = ctx.createBiquadFilter();
    hp.type = 'highpass'; hp.frequency.value = 800;
    noiseGain.gain.setValueAtTime(0, ctx.currentTime);
    noiseGain.gain.linearRampToValueAtTime(0.22, ctx.currentTime + 0.4);
    staticSrc.connect(hp); hp.connect(noiseGain);
    staticSrc.start();
  }
  function stopStatic() {
    if (staticSrc) {
      try { staticSrc.stop(); } catch(e){}
      staticSrc = null;
    }
    if (noiseGain && ctx) noiseGain.gain.setTargetAtTime(0, ctx.currentTime, 0.4);
  }

  // Moniteur cardiaque (bip régulier) pendant la lumière blanche finale
  let monitorHandle = null;
  function startMonitor(bpm = 62) {
    stopMonitor();
    const beep = () => {
      if (!ctx) return;
      const t = ctx.currentTime;
      const osc = ctx.createOscillator();
      osc.type = 'sine';
      osc.frequency.value = 880;
      const g = ctx.createGain();
      g.gain.setValueAtTime(0.0001, t);
      g.gain.exponentialRampToValueAtTime(0.25, t+0.015);
      g.gain.exponentialRampToValueAtTime(0.0001, t+0.22);
      osc.connect(g); g.connect(master);
      osc.start(t); osc.stop(t+0.25);
      monitorHandle = setTimeout(beep, 60000/bpm);
    };
    beep();
  }
  function stopMonitor() {
    if (monitorHandle) clearTimeout(monitorHandle);
    monitorHandle = null;
  }

  function setMasterVolume(v) {
    if (master && ctx) master.gain.setTargetAtTime(v, ctx.currentTime, 0.5);
  }

  return {
    init, setTension, footstep, tensionSurge, screamHit,
    startStatic, stopStatic, startMonitor, stopMonitor, setMasterVolume
  };
})();
