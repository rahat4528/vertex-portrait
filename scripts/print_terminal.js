// True Wireframe Terminal Renderer for Low-Poly Face Portrait
// Renders vector lines with 2x4 subpixel Unicode Braille resolution

import { VERTICES, EDGES } from '../src/face_mesh.ts';

const termCols = process.stdout.columns ? Math.min(process.stdout.columns - 2, 78) : 78;
const gridW = termCols * 2;
const gridH = Math.round(gridW * (773 / 615));
const termRows = Math.ceil(gridH / 4);

const dots = new Uint8Array(termCols * termRows);

function setDot(gx, gy) {
  if (gx < 0 || gx >= gridW || gy < 0 || gy >= gridH) return;
  const col = Math.floor(gx / 2);
  const row = Math.floor(gy / 4);
  const dx = gx % 2;
  const dy = gy % 4;

  let bit = 0;
  if (dx === 0 && dy === 0) bit = 0x01;
  else if (dx === 0 && dy === 1) bit = 0x02;
  else if (dx === 0 && dy === 2) bit = 0x04;
  else if (dx === 1 && dy === 0) bit = 0x08;
  else if (dx === 1 && dy === 1) bit = 0x10;
  else if (dx === 1 && dy === 2) bit = 0x20;
  else if (dx === 0 && dy === 3) bit = 0x40;
  else if (dx === 1 && dy === 3) bit = 0x80;

  dots[row * termCols + col] |= bit;
}

function drawLine(p0, p1) {
  let gx0 = Math.round((p0[0] * (gridW - 1)) / 615);
  let gy0 = Math.round((p0[1] * (gridH - 1)) / 773);
  let gx1 = Math.round((p1[0] * (gridW - 1)) / 615);
  let gy1 = Math.round((p1[1] * (gridH - 1)) / 773);

  const dx = Math.abs(gx1 - gx0);
  const dy = -Math.abs(gy1 - gy0);
  const sx = gx0 < gx1 ? 1 : -1;
  const sy = gy0 < gy1 ? 1 : -1;
  let err = dx + dy;

  while (true) {
    setDot(gx0, gy0);
    if (gx0 === gx1 && gy0 === gy1) break;
    const e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      gx0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      gy0 += sy;
    }
  }
}

// 1. Purple top bar
for (let x = 0; x < gridW; x++) {
  setDot(x, 1);
}

// 2. All wireframe edges
for (const [v0, v1] of EDGES) {
  drawLine(VERTICES[v0], VERTICES[v1]);
}

// Output to terminal with clean ANSI styling
console.log('\n\x1b[1;35m================================================================================\x1b[0m');
console.log('\x1b[1;37m        LOW-POLY WIREFRAME FACE PORTRAIT (VECTOR TERMINAL RENDER)               \x1b[0m');
console.log('\x1b[1;35m================================================================================\x1b[0m\n');

for (let r = 0; r < termRows; r++) {
  let line = '';
  for (let c = 0; c < termCols; c++) {
    const mask = dots[r * termCols + c];
    if (mask === 0) {
      line += ' ';
    } else {
      if (r === 0) {
        line += '\x1b[38;2;160;90;190m' + String.fromCharCode(0x2800 + mask) + '\x1b[0m';
      } else {
        line += '\x1b[38;2;215;215;220m' + String.fromCharCode(0x2800 + mask) + '\x1b[0m';
      }
    }
  }
  console.log(line);
}

console.log('\n\x1b[38;2;150;150;160m[Wireframe Vector Mesh | Subpixel Braille ' + gridW + 'x' + gridH + ' dots | 488 Vertices | 773 Edges]\x1b[0m\n');
