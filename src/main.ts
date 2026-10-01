// Low-Poly Wireframe Face Portrait
// Re-creation matching the reference image geometry exactly

import { VERTICES, EDGES, TRIANGLES, SHADING_PATHS, Pt } from './face_mesh.ts';

export { VERTICES, EDGES, TRIANGLES, SHADING_PATHS };
export type { Pt };
export type Tri = [Pt, Pt, Pt];

export interface DrawablePolyline {
  type: 'polyline';
  points: Pt[];
  close?: boolean;
}

export interface DrawableTri {
  type: 'triangle';
  points: Tri;
}

export type DrawableItem = DrawableTri | DrawablePolyline;

// Seeded PRNG: Mulberry32 (Deterministic LCG-like generator)
export function mulberry32(seed: number): () => number {
  let s = seed >>> 0;
  return function (): number {
    s = (s + 0x6d2b79f5) >>> 0;
    let t = Math.imul(s ^ (s >>> 15), 1 | s);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

// Master Face Data Specification
export const FACE_DATA = {
  canvas: {
    width: 615,
    height: 773,
    bgColor: '#2F2E33',
    topBar: {
      x: 0,
      y: 0,
      width: 615,
      height: 8,
      color: '#5A3A6A',
    },
  },
  style: {
    strokeColor: 'rgba(215, 215, 220, 0.90)',
    lineWidth: 1,
    shadingColor: 'rgba(255, 255, 255, 0.08)',
  },
  landmarks: {
    topBar: { y: [0, 8], color: '#5A3A6A' },
    browRight: { root: [199, 159], peak: [285, 117], tail: [361, 136] },
    browLeftProfile: { root: [90, 110], peak: [105, 78], tail: [125, 110] },
    eyeRight: { outer: [235, 195], inner: [337, 183], irisCenter: [268, 188] },
    eyeLeftProfile: { outer: [98, 222], inner: [132, 215], pupil: [115, 220] },
    nose: { root: [152, 260], bridge: [162, 315], tip: [186, 400], nostril: [202, 403] },
    lips: { cupidBow: [158, 492], mouthLeft: [122, 508], mouthRight: [226, 503], lowerLipBottom: [168, 554] },
    jaw: { chinTip: [195, 620], angle: [345, 555], ear: [530, 360] },
    cheekbone: [420, 320],
    neck: [340, 710],
  },
  vertices: VERTICES,
  edges: EDGES,
  triangles: TRIANGLES,
};

// Procedural strip generator (for backwards compatibility & tests)
export function strip(top: Pt[], bottom: Pt[]): Tri[] {
  const tris: Tri[] = [];
  let i = 0;
  let j = 0;
  while (i < top.length - 1 || j < bottom.length - 1) {
    if (i >= top.length - 1) {
      tris.push([top[i], bottom[j], bottom[j + 1]]);
      j++;
    } else if (j >= bottom.length - 1) {
      tris.push([top[i], bottom[j], top[i + 1]]);
      i++;
    } else {
      const d1 = Math.hypot(top[i + 1][0] - bottom[j][0], top[i + 1][1] - bottom[j][1]);
      const d2 = Math.hypot(top[i][0] - bottom[j + 1][0], top[i][1] - bottom[j + 1][1]);
      if (d1 < d2) {
        tris.push([top[i], bottom[j], top[i + 1]]);
        i++;
      } else {
        tris.push([top[i], bottom[j], bottom[j + 1]]);
        j++;
      }
    }
  }
  return tris;
}

// Procedural triangle fan generator
export function fan(center: Pt, ring: Pt[]): Tri[] {
  const tris: Tri[] = [];
  for (let i = 0; i < ring.length - 1; i++) {
    tris.push([center, ring[i], ring[i + 1]]);
  }
  return tris;
}

// Draw a single triangle
export function drawTriangle(
  ctx: CanvasRenderingContext2D,
  a: Pt,
  b: Pt,
  c: Pt
): void {
  ctx.beginPath();
  ctx.moveTo(a[0], a[1]);
  ctx.lineTo(b[0], b[1]);
  ctx.lineTo(c[0], c[1]);
  ctx.closePath();
  ctx.stroke();
}

// Draw soft translucent crescent shading shapes
export function drawSoftShading(ctx: CanvasRenderingContext2D): void {
  ctx.save();
  ctx.fillStyle = FACE_DATA.style.shadingColor;

  // Crescent Shape A: Jawline highlight
  ctx.beginPath();
  ctx.moveTo(190, 560);
  ctx.quadraticCurveTo(350, 590, 535, 380);
  ctx.quadraticCurveTo(380, 530, 190, 560);
  ctx.closePath();
  ctx.fill();

  // Crescent Shape B: Cheekbone highlight
  ctx.beginPath();
  ctx.moveTo(390, 380);
  ctx.quadraticCurveTo(470, 350, 525, 280);
  ctx.quadraticCurveTo(470, 365, 390, 380);
  ctx.closePath();
  ctx.fill();

  ctx.restore();
}

// Master draw function
export function drawScene(
  ctx: CanvasRenderingContext2D,
  animate = false,
  _seed = 7,
  onComplete?: () => void
): () => void {
  let animationFrameId: number | null = null;
  let isCancelled = false;

  // 1. Solid Background
  ctx.fillStyle = FACE_DATA.canvas.bgColor;
  ctx.fillRect(0, 0, FACE_DATA.canvas.width, FACE_DATA.canvas.height);

  // 2. Purple Top Bar
  const { topBar } = FACE_DATA.canvas;
  ctx.fillStyle = topBar.color;
  ctx.fillRect(topBar.x, topBar.y, topBar.width, topBar.height);

  // 3. Soft Shading Crescents
  drawSoftShading(ctx);

  // Configure stroke style
  ctx.strokeStyle = FACE_DATA.style.strokeColor;
  ctx.lineWidth = FACE_DATA.style.lineWidth;
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';

  const edges = EDGES;

  if (!animate) {
    ctx.beginPath();
    for (let i = 0; i < edges.length; i++) {
      const [v0, v1] = edges[i];
      const p0 = VERTICES[v0];
      const p1 = VERTICES[v1];
      ctx.moveTo(p0[0], p0[1]);
      ctx.lineTo(p1[0], p1[1]);
    }
    ctx.stroke();
    if (onComplete) onComplete();
  } else {
    let index = 0;
    const batchSize = 6; // Draw 6 segments per frame for a smooth, captivating animation

    const renderStep = () => {
      if (isCancelled) return;
      ctx.beginPath();
      for (let k = 0; k < batchSize && index < edges.length; k++, index++) {
        const [v0, v1] = edges[index];
        const p0 = VERTICES[v0];
        const p1 = VERTICES[v1];
        ctx.moveTo(p0[0], p0[1]);
        ctx.lineTo(p1[0], p1[1]);
      }
      ctx.stroke();

      if (index < edges.length) {
        animationFrameId = requestAnimationFrame(renderStep);
      } else {
        if (onComplete) onComplete();
      }
    };

    animationFrameId = requestAnimationFrame(renderStep);
  }

  return () => {
    isCancelled = true;
    if (animationFrameId !== null) {
      cancelAnimationFrame(animationFrameId);
    }
  };
}

// Optional Bonus Flags
export const ANIMATE = false;
export const DIRECT_PRINT_WHEN_DONE = true;

// DOM Initialization
function init(): void {
  const canvas = document.getElementById('faceCanvas') as HTMLCanvasElement | null;
  if (!canvas) return;

  canvas.width = FACE_DATA.canvas.width;
  canvas.height = FACE_DATA.canvas.height;
  const ctx = canvas.getContext('2d');
  if (!ctx) return;

  const animateCheckbox = document.getElementById('animateToggle') as HTMLInputElement | null;
  const autoPrintCheckbox = document.getElementById('autoPrintToggle') as HTMLInputElement | null;
  const redrawBtn = document.getElementById('redrawBtn') as HTMLButtonElement | null;
  const printBtn = document.getElementById('printBtn') as HTMLButtonElement | null;
  const downloadBtn = document.getElementById('downloadBtn') as HTMLButtonElement | null;
  const seedInput = document.getElementById('seedInput') as HTMLInputElement | null;
  const statusSpan = document.getElementById('statusText') as HTMLSpanElement | null;

  let cancelPrev: (() => void) | null = null;

  const triggerDraw = () => {
    if (cancelPrev) cancelPrev();
    const shouldAnimate = animateCheckbox ? animateCheckbox.checked : ANIMATE;
    const currentSeed = seedInput ? parseInt(seedInput.value, 10) || 7 : 7;

    if (statusSpan) {
      statusSpan.textContent = shouldAnimate ? 'Generating with animation...' : 'Rendered (Exact Wireframe Mesh)';
    }

    cancelPrev = drawScene(ctx, shouldAnimate, currentSeed, () => {
      if (statusSpan) statusSpan.textContent = `Completed (${FACE_DATA.vertices.length} Vertices, ${FACE_DATA.edges.length} Edges)`;
      const shouldAutoPrint = autoPrintCheckbox ? autoPrintCheckbox.checked : false;
      if (shouldAutoPrint) {
        setTimeout(() => {
          window.print();
        }, 350);
      }
    });
  };

  if (redrawBtn) redrawBtn.addEventListener('click', triggerDraw);
  if (printBtn) {
    printBtn.addEventListener('click', () => {
      window.print();
    });
  }
  if (downloadBtn) {
    downloadBtn.addEventListener('click', () => {
      const a = document.createElement('a');
      a.download = 'vertex_face_portrait.png';
      a.href = canvas.toDataURL('image/png');
      a.click();
    });
  }
  if (animateCheckbox) animateCheckbox.addEventListener('change', triggerDraw);
  if (seedInput) seedInput.addEventListener('change', triggerDraw);

  // Initial draw
  triggerDraw();
}

if (typeof window !== 'undefined') {
  window.addEventListener('DOMContentLoaded', init);
}
