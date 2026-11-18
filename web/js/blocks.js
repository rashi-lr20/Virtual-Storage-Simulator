// Each file id gets its own colour. (id * 47 spreads the colours around the colour wheel.)
function fileColor(id) {
  const hue = (id * 47 + 205) % 360;
  return 'hsl(' + hue + ' 72% 62%)';
}

// 0 -> "A", 1 -> "B" ... 25 -> "Z", bigger ids are shown as numbers
function fileLabel(id) {
  return id < 26 ? String.fromCharCode(65 + id) : String(id);
}

// Called by common.js every time the C++ state changes.
//   state  = everything the C++ program reported (blocks, free list, stats ...)
//   action = what just happened (allocate / release ...), so we can animate it
function renderBlocks(state, action) {
  const grid = document.getElementById('blockGrid');
  const n = state.total;

  // 1) choose a cell size so that big storages still fit on the screen
  const size = n <= 60 ? 46 : n <= 150 ? 34 : n <= 300 ? 24 : 18;
  grid.style.setProperty('--cell', size + 'px');

  // 2) make the right number of cells (only when the storage size changed)
  if (grid.children.length !== n) {
    grid.innerHTML = '';
    for (let i = 0; i < n; i++) {
      const cell = document.createElement('div');
      cell.className = 'cell free';
      cell.innerHTML = '<span class="idx">' + i + '</span><span class="lbl"></span>';
      grid.appendChild(cell);
    }
  }

  // 3) work out which free segment each free block belongs to (so we can tint the gaps)
  const segOf = new Array(n).fill(-1);
  state.free.forEach(function (seg, k) {
    for (let i = seg.start; i < seg.start + seg.size; i++) segOf[i] = k;
  });

  // 4) colour every cell according to the array
  for (let i = 0; i < n; i++) {
    const id = state.blocks[i];
    const cell = grid.children[i];
    const lbl = cell.querySelector('.lbl');
    cell.classList.toggle('hide-idx', size < 30);
    cell.classList.remove('pop', 'fade', 'hl');

    if (id < 0) {                                         // free block
      cell.className = 'cell free ' + (segOf[i] % 2 === 0 ? 'segA' : 'segB') + (size < 30 ? ' hide-idx' : '');
      cell.style.background = '';
      lbl.textContent = '';
      cell.title = 'Block ' + i + ': free';
    } else {                                              // occupied block
      cell.className = 'cell used' + (size < 30 ? ' hide-idx' : '');
      cell.style.background = fileColor(id);
      lbl.textContent = size >= 24 ? fileLabel(id) : '';
      cell.title = 'Block ' + i + ': file ' + fileLabel(id);
    }
  }

  // 5) animate what just happened
  const panel = document.getElementById('panelBlocks');
  panel.classList.remove('shake');
  if (action && action.start >= 0 && action.length > 0) {
    const css = action.type === 'allocate' ? 'pop' : action.type === 'release' ? 'fade' : '';
    if (css) {
      for (let i = action.start; i < action.start + action.length && i < n; i++) {
        grid.children[i].classList.add(css);
      }
    }
  }
  if (action && action.type === 'allocate' && !action.ok) {
    void panel.offsetWidth;            // restart the animation
    panel.classList.add('shake');      // failed request: shake the panel
  }

  // 6) one chip per file: shows where it lives, and a button to release it
  const chips = document.getElementById('fileChips');
  chips.innerHTML = '';
  if (state.files.length === 0) {
    chips.innerHTML = '<span class="empty">No files yet. Use the Request panel to allocate one.</span>';
  }
  state.files.forEach(function (f) {
    const chip = document.createElement('div');
    chip.className = 'chip';
    chip.innerHTML =
      '<span class="dot" style="background:' + fileColor(f.id) + '"></span>' +
      '<b>' + f.label + '</b>' +
      '<span class="muted">blocks ' + f.start + '–' + (f.start + f.length - 1) + ' (' + f.length + ')</span>' +
      '<button title="Release file ' + f.label + '" aria-label="Release file ' + f.label + '">✕</button>';
    chip.querySelector('button').addEventListener('click', function () { requestRelease(f.label); });
    chip.addEventListener('mouseenter', function () { highlightBlocks(f.start, f.length, true); });
    chip.addEventListener('mouseleave', function () { highlightBlocks(f.start, f.length, false); });
    chips.appendChild(chip);
  });
}

// Lights up a range of blocks (used when you hover a chip or a linked-list node).
function highlightBlocks(start, length, on) {
  const grid = document.getElementById('blockGrid');
  for (let i = start; i < start + length && i < grid.children.length; i++) {
    grid.children[i].classList.toggle('hl', on);
  }
}