// ── Confirm JS loaded ──────────────────────────────────────────────
document.getElementById('jsStatus').textContent = '✓ served';
document.getElementById('jsStatus').classList.remove('pending');
document.getElementById('card-js').classList.add('ok');

// ── Confirm CSS loaded (check a computed style we set) ─────────────
const bodyBg = getComputedStyle(document.body).backgroundColor;
// If CSS loaded, background will be the dark colour, not white
const cssOk = bodyBg !== 'rgba(0, 0, 0, 0)' && bodyBg !== 'rgb(255, 255, 255)';
const cssStatusEl = document.getElementById('cssStatus');
cssStatusEl.textContent = cssOk ? '✓ served' : '✗ missing';
cssStatusEl.classList.remove('pending');
if (cssOk) document.getElementById('card-css').classList.add('ok');

// Mark HTML card too
document.getElementById('card-html').classList.add('ok');

// ── Live clock ─────────────────────────────────────────────────────
const clockEl = document.getElementById('clock');
function tick() {
  clockEl.textContent = new Date().toLocaleTimeString([], { hour12: false });
}
tick();
setInterval(tick, 1000);

// ── Ripple canvas ──────────────────────────────────────────────────
const canvas = document.getElementById('canvas');
const ctx    = canvas.getContext('2d');

let W, H;
const ripples = [];

function resize() {
  const rect = canvas.getBoundingClientRect();
  W = canvas.width  = rect.width  * devicePixelRatio;
  H = canvas.height = rect.height * devicePixelRatio;
  ctx.scale(devicePixelRatio, devicePixelRatio);
}
resize();
window.addEventListener('resize', resize);

const COLORS = ['#3dffa0', '#ffd84d', '#70b8ff', '#ff7eb3'];

function spawnRipple(x, y) {
  ripples.push({
    x, y,
    r: 0,
    maxR: 90 + Math.random() * 60,
    color: COLORS[Math.floor(Math.random() * COLORS.length)],
    alpha: 1,
    speed: 1.8 + Math.random() * 1.2,
  });
}

canvas.addEventListener('click', e => {
  const rect = canvas.getBoundingClientRect();
  spawnRipple(e.clientX - rect.left, e.clientY - rect.top);
});

// Seed a gentle auto-ripple every few seconds if idle
let lastInteraction = 0;
canvas.addEventListener('click', () => { lastInteraction = Date.now(); });
setInterval(() => {
  if (Date.now() - lastInteraction > 3000) {
    const W2 = canvas.getBoundingClientRect().width;
    const H2 = canvas.getBoundingClientRect().height;
    spawnRipple(
      W2 * 0.15 + Math.random() * W2 * 0.7,
      H2 * 0.15 + Math.random() * H2 * 0.7
    );
  }
}, 2200);

function draw() {
  const W2 = canvas.getBoundingClientRect().width;
  const H2 = canvas.getBoundingClientRect().height;

  ctx.clearRect(0, 0, W2, H2);

  for (let i = ripples.length - 1; i >= 0; i--) {
    const rp = ripples[i];
    rp.r     += rp.speed;
    rp.alpha  = 1 - rp.r / rp.maxR;

    if (rp.alpha <= 0) { ripples.splice(i, 1); continue; }

    ctx.beginPath();
    ctx.arc(rp.x, rp.y, rp.r, 0, Math.PI * 2);
    ctx.strokeStyle = rp.color;
    ctx.globalAlpha = rp.alpha * 0.75;
    ctx.lineWidth   = 2;
    ctx.stroke();

    // inner fill dot at origin
    if (rp.r < 8) {
      ctx.beginPath();
      ctx.arc(rp.x, rp.y, 3, 0, Math.PI * 2);
      ctx.fillStyle  = rp.color;
      ctx.globalAlpha = rp.alpha;
      ctx.fill();
    }

    ctx.globalAlpha = 1;
  }

  requestAnimationFrame(draw);
}
draw();
