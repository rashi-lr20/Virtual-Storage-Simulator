// =====================================================================================
//  MEMBER 2 — Free list view (the LINKED LIST)
//  Draws   HEAD -> [start|size|•] -> [start|size|•] -> NULL
//  exactly like the nodes in your C++ FreeList (struct FreeSegment { start; size; next; }).
//  It also shows which node First Fit / Best Fit / Worst Fit would choose for the size typed in the Request panel.
// =====================================================================================

// state   = what the C++ program reported (state.free is the list, node by node)
// preview = answer of /api/preview: {size, first, best, worst} = start of the chosen node (or -1)
function renderFreeList(state, preview) {
  const box = document.getElementById('listView');
  box.innerHTML = '';

  // animate only when the list really changed (not on every preview refresh or strategy switch)
  const sig = JSON.stringify(state.free);
  box.classList.toggle('still', sig === renderFreeList.lastSig);
  renderFreeList.lastSig = sig;

  const head = document.createElement('div');
  head.className = 'ptr head';
  head.textContent = 'HEAD';
  box.appendChild(head);

  // the biggest node gets a green outline
  let largest = 0;
  state.free.forEach(function (s) { if (s.size > largest) largest = s.size; });

  state.free.forEach(function (seg, k) {
    const arrow = document.createElement('div');
    arrow.className = 'arrow';
    arrow.textContent = '→';
    box.appendChild(arrow);

    // wrapper = node + a small row of badges above it
    const wrap = document.createElement('div');
    wrap.className = 'nodewrap';
    const node = document.createElement('div');
    node.className = 'node' + (seg.size === largest ? ' largest' : '');
    node.style.animationDelay = (k * 40) + 'ms';
    node.title = 'Free blocks ' + seg.start + ' to ' + (seg.start + seg.size - 1);

    // strategy badges (only when a size was typed and this node is chosen)
    let badges = '';
    if (preview && preview.size > 0) {
      if (preview.first === seg.start) badges += '<span class="badge first">FIRST</span>';
      if (preview.best === seg.start)  badges += '<span class="badge best">BEST</span>';
      if (preview.worst === seg.start) badges += '<span class="badge worst">WORST</span>';
    }

    wrap.innerHTML = '<div class="badges">' + badges + '</div>';
    node.innerHTML =
      '<div class="f"><small>start</small><b>' + seg.start + '</b></div>' +
      '<div class="f"><small>size</small><b>' + seg.size + '</b></div>' +
      '<div class="nx" title="next pointer">●</div>';

    node.addEventListener('mouseenter', function () { if (typeof highlightBlocks === 'function') highlightBlocks(seg.start, seg.size, true); });
    node.addEventListener('mouseleave', function () { if (typeof highlightBlocks === 'function') highlightBlocks(seg.start, seg.size, false); });
    wrap.appendChild(node);
    box.appendChild(wrap);
  });

  const arrow = document.createElement('div');
  arrow.className = 'arrow';
  arrow.textContent = '→';
  box.appendChild(arrow);

  const nul = document.createElement('div');
  nul.className = 'ptr null';
  nul.textContent = 'NULL';
  box.appendChild(nul);

  // sentence under the list
  const note = document.getElementById('previewNote');
  if (state.free.length === 0) {
    note.innerHTML = 'The list is empty: <b>every block is occupied</b>.';
  } else if (preview && preview.size > 0) {
    const names = { first: 'First Fit', best: 'Best Fit', worst: 'Worst Fit' };
    let html = 'A request of <b>' + preview.size + '</b> blocks would go to: ';
    ['first', 'best', 'worst'].forEach(function (key) {
      html += '<span class="pill ' + key + '">' + names[key] + '</span>' +
        (preview[key] >= 0 ? 'node starting at ' + preview[key] : '<em>no node big enough</em>') + ' &nbsp; ';
    });
    note.innerHTML = html;
  } else {
    note.innerHTML = 'Type a size in the Request panel to see where each strategy would allocate.';
  }
}

// -------------------------------------------------------------------------------------
// "Strategy lab": explains, for the size typed in the action bar, what each strategy does
// and how many nodes it has to look at (this is the O(F) from your complexity analysis).
// -------------------------------------------------------------------------------------
function renderStrategyLab(state, preview) {
  const box = document.getElementById('labRows');
  if (!box) return;
  const n = preview ? preview.size : 0;
  const F = state.free.length;
  document.getElementById('labSizeNum').textContent = n > 0 ? n : '–';

  const rules = [
    ['first', 'First Fit', 'walks from HEAD and stops at the first node with size ≥ ' + (n || 'n')],
    ['best', 'Best Fit', 'looks at every node and keeps the smallest one with size ≥ ' + (n || 'n')],
    ['worst', 'Worst Fit', 'looks at every node and keeps the largest one (if it is ≥ ' + (n || 'n') + ')']
  ];
  box.innerHTML = '';
  rules.forEach(function (r) {
    const key = r[0];
    let res;
    if (!(n > 0)) {
      res = 'Type a size in the bar above (or move the slider).';
    } else if (preview[key] < 0) {
      res = 'No node is big enough: the request would FAIL (nodes checked: ' + F + ' of ' + F + ').';
    } else {
      let idx = 0;
      state.free.forEach(function (seg, k) { if (seg.start === preview[key]) idx = k; });
      const seg = state.free[idx];
      const checked = key === 'first' ? idx + 1 : F;
      res = 'chooses node [start ' + seg.start + ', size ' + seg.size + '] · nodes checked: ' + checked + ' of ' + F +
        ' · leftover gap: ' + (seg.size - n);
    }
    const row = document.createElement('div');
    row.className = 'lab-row';
    row.innerHTML = '<div class="nm"><i class="sdot ' + key + '"></i>' + r[1] + '</div>' +
      '<div><div class="why-t">' + r[2] + '</div><div class="res">' + res + '</div></div>';
    box.appendChild(row);
  });
}
