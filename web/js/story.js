// =====================================================================================
//  MEMBER 4 (shared) — the "story": a scripted illustration used on the landing page and the Learn page.
//  IMPORTANT: this is only an ILLUSTRATION with fixed frames, so people can watch the idea.
//  The real simulator (all other pages) runs in C++.
//  Needs blocks.js loaded first (fileColor, fileLabel).
// =====================================================================================

const STORY_TOTAL = 20;

// Turns a list of files into the same shape the C++ program sends: blocks[], free[] ...
function buildFrame(total, files) {
  const blocks = new Array(total).fill(-1);
  files.forEach(function (f) { for (let i = f.start; i < f.start + f.len; i++) blocks[i] = f.id; });
  const free = [];
  let i = 0;
  while (i < total) {
    if (blocks[i] < 0) { let j = i; while (j < total && blocks[j] < 0) j++; free.push({ start: i, size: j - i }); i = j; }
    else i++;
  }
  return { total: total, blocks: blocks, free: free };
}

const FA = { id: 0, start: 0, len: 5 }, FB = { id: 1, start: 5, len: 4 }, FC = { id: 2, start: 9, len: 5 };
const STORY = [
  { files: [],        caption: 'Start: 20 blocks, all free. The linked list has ONE node: [start 0, size 20].' },
  { files: [FA],      caption: 'Allocate file A (5 blocks). It takes the first 5 blocks of the gap, which shrinks to [5, 15].', mark: [0, 5, 'pop'] },
  { files: [FA, FB],    caption: 'Allocate B (4 blocks). The gap shrinks to [9, 11].', mark: [5, 4, 'pop'] },
  { files: [FA, FB, FC], caption: 'Allocate C (5 blocks). The gap is now [14, 6].', mark: [9, 5, 'pop'] },
  { files: [FB, FC],    caption: 'Release A. Blocks 0–4 are free again, but B and C separate them from the gap at the end: now TWO nodes, [0,5] and [14,6].', mark: [0, 5, 'fade'] },
  { files: [FB, FC],    caption: 'Request D = 7 blocks. Free blocks: 5 + 6 = 11, but the biggest gap is only 6. The request FAILS. This is fragmentation.', bad: true },
  { files: [FC],     caption: 'Release B. Its blocks touch the gap on the left, so the two nodes MERGE into [0, 9].', mark: [5, 4, 'fade'] },
  { files: [],        caption: 'Release C. It touches the gap on its left AND on its right: three nodes become one, [0, 20]. Fragmentation is gone.', mark: [9, 5, 'fade'] }
];

// ---- tiny drawing helpers (own markup, so they work on any page) ----
function drawStoryGrid(el, frame, mark, bad) {
  const n = frame.total;
  if (el.children.length !== n) {
    el.innerHTML = '';
    for (let i = 0; i < n; i++) {
      const c = document.createElement('div');
      c.className = 'cell free';
      c.innerHTML = '<span class="idx">' + i + '</span><span class="lbl"></span>';
      el.appendChild(c);
    }
  }
  const segOf = new Array(n).fill(-1);
  frame.free.forEach(function (s, k) { for (let i = s.start; i < s.start + s.size; i++) segOf[i] = k; });
  for (let i = 0; i < n; i++) {
    const id = frame.blocks[i], c = el.children[i], lbl = c.querySelector('.lbl');
    if (id < 0) {
      c.className = 'cell free ' + (segOf[i] % 2 === 0 ? 'segA' : 'segB');
      c.style.background = ''; lbl.textContent = '';
    } else {
      c.className = 'cell used'; c.style.background = fileColor(id); lbl.textContent = fileLabel(id);
    }
  }
  if (mark) for (let i = mark[0]; i < mark[0] + mark[1]; i++) el.children[i].classList.add(mark[2]);
  if (bad) frame.free.forEach(function (s) { for (let i = s.start; i < s.start + s.size; i++) el.children[i].classList.add('fade'); });
}

function drawStoryList(el, frame) {
  let html = '<div class="ptr head">HEAD</div>';
  frame.free.forEach(function (s) {
    html += '<div class="arrow">→</div><div class="nodewrap"><div class="badges"></div><div class="node">' +
      '<div class="f"><small>start</small><b>' + s.start + '</b></div><div class="f"><small>size</small><b>' + s.size + '</b></div>' +
      '<div class="nx">●</div></div></div>';
  });
  html += '<div class="arrow">→</div><div class="ptr null">NULL</div>';
  el.innerHTML = html;
}
