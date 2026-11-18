// =====================================================================================
//  MEMBER 3 — Statistics views
//  Shows the numbers computed by your C++ FragmentationAnalyzer:
//  free blocks, occupied blocks, free segments, largest segment, fragmentation, failure counters,
//  and whether the array and the linked list agree (checkConsistency).
//  Used by analysis.html (renderStats, renderWhyFailed) and simulator.html (renderMiniStats).
// =====================================================================================

function card(label, value) {
  return '<div class="stat"><small>' + label + '</small><b>' + value + '</b></div>';
}

function counter(label, value, kind) {
  return '<div class="counter ' + kind + '"><span>' + label + '</span><b>' + value + '</b></div>';
}

// small version for the Simulator page
function renderMiniStats(state) {
  const s = state.stats;
  const el = document.getElementById('miniStats');
  if (!el) return;
  el.innerHTML = card('Occupied', s.occupied) + card('Free', s.free) + card('Free segments', s.segments) + card('Largest gap', s.largest) +
    '<div class="stat wide"><small>Fragmentation</small><b>' + s.fragmentation.toFixed(3) + '</b>' +
    '<div class="meter" style="margin-top:8px"><i style="width:' + Math.round(s.fragmentation * 100) + '%"></i></div></div>';
}

// full statistics page
function renderStats(state) {
  const s = state.stats;
  const c = state.counters;

  // 1) number cards
  document.getElementById('statCards').innerHTML =
    card('Total blocks', s.total) + card('Occupied', s.occupied) + card('Free blocks', s.free) +
    card('Free segments', s.segments) + card('Largest segment', s.largest);

  // 2) fragmentation meter  (0 = free space in one piece, 1 = free space in many tiny pieces)
  const pct = Math.round(s.fragmentation * 100);
  document.getElementById('fragBlock').innerHTML =
    '<div class="big-number">' + s.fragmentation.toFixed(3) + '</div>' +
    '<div class="meter" style="margin-top:12px" role="img" aria-label="Fragmentation ' + pct + ' percent"><i style="width:' + pct + '%"></i></div>' +
    '<div class="axis-note"><span>0 · one piece</span><span>1 · many tiny pieces</span></div>' +
    '<div class="note">' + (s.free === 0 ? 'No free space.' :
       s.segments <= 1 ? 'Free space is <b>in one piece</b>.' :
       'Free space is split into <b>' + s.segments + ' pieces</b>.') + '</div>';

  // 3) history chart (the C++ program remembers it, so it survives page changes)
  document.getElementById('histBlock').innerHTML = historyChart(state.history, state.opCount);

  // 4) bar that shows how the free space is split (explains WHY big requests can fail)
  let segHtml;
  if (state.free.length === 0) {
    segHtml = '<div class="note">Nothing is free.</div>';
  } else {
    segHtml = '<div class="seg-bar">';
    state.free.forEach(function (seg) {
      segHtml += '<i class="' + (seg.size === s.largest ? 'big' : '') + '" style="flex:' + seg.size + '" title="start ' +
        seg.start + ', size ' + seg.size + '">' + seg.size + '</i>';
    });
    segHtml += '</div><div class="note">Total free <b>' + s.free + '</b>, but the biggest piece is only <b>' + s.largest +
      '</b>. A contiguous request larger than ' + s.largest + ' fails even though ' + s.free + ' blocks are free.</div>';
  }
  document.getElementById('segBlock').innerHTML = segHtml;

  // 5) counters
  document.getElementById('counters').innerHTML =
    counter('Successful', c.success, 'good') +
    counter('Failed: fragmentation', c.failFrag, 'bad') +
    counter('Failed: no space', c.failNoSpace, 'warn') +
    counter('Rejected', c.rejected, 'warn') +
    counter('Bad releases', c.badReleases, 'warn');

  // 6) array and list agree?
  document.getElementById('consistency').innerHTML = state.consistent
    ? '<span class="ok">✓ Consistent</span> the array and the linked list describe the same free space'
    : '<span class="no">✗ MISMATCH</span> the array and the linked list disagree: there is a bug';
}

// "Why did the last request fail?"
function renderWhyFailed(state) {
  const el = document.getElementById('whyBox');
  if (!el) return;
  const a = state.action, s = state.stats;
  if (a.type === 'allocate' && !a.ok) {
    const n = a.length;
    let text, eq, kind = 'bad';
    if (a.result.indexOf('fragmentation') >= 0) {
      text = 'The request for <b>' + n + '</b> blocks failed because of <b>fragmentation</b>: enough free space exists in total, but not in one piece.';
      eq = 'free = ' + s.free + ' ≥ ' + n + ' (enough in total)\nlargest segment = ' + s.largest + ' < ' + n + ' (no single gap is big enough)';
    } else if (a.result.indexOf('not enough') >= 0) {
      text = 'The request for <b>' + n + '</b> blocks failed because there is <b>not enough free space</b> at all.';
      eq = 'free = ' + s.free + ' < ' + n;
    } else {
      text = 'The request was <b>rejected</b> as invalid (' + a.result + ').';
      eq = '';
    }
    el.className = 'why ' + kind;
    el.innerHTML = text + (eq ? '<span class="eq">' + eq.replace(/\n/g, '<br>') + '</span>' : '');
  } else if (state.counters.failFrag + state.counters.failNoSpace > 0) {
    el.className = 'why';
    el.innerHTML = 'The last operation did not fail. So far: <b>' + state.counters.failFrag + '</b> fragmentation failure(s) and <b>' +
      state.counters.failNoSpace + '</b> out-of-space failure(s).';
  } else {
    el.className = 'why';
    el.innerHTML = 'No request has failed yet. Allocate and release files on the <a href="simulator.html">Simulator</a> page, or press <b>Random fill</b>, until a big request fails.';
  }
}

// line chart of fragmentation after every operation (SVG)
function historyChart(history, opCount) {
  if (!history || history.length < 2) {
    return '<div class="note">Do a few operations to see the fragmentation trend. The C++ program remembers every step.</div>';
  }
  const w = 600, h = 180, padL = 30, padR = 8, padT = 10, padB = 8;
  const step = (w - padL - padR) / (history.length - 1);
  const y = function (v) { return padT + (1 - v) * (h - padT - padB); };
  const pts = history.map(function (v, i) { return (padL + i * step).toFixed(1) + ',' + y(v).toFixed(1); });
  const last = history[history.length - 1];
  let grid = '';
  [0, 0.5, 1].forEach(function (g) {
    grid += '<line x1="' + padL + '" x2="' + (w - padR) + '" y1="' + y(g) + '" y2="' + y(g) + '" stroke="#243050" stroke-dasharray="3 4"/>' +
      '<text x="4" y="' + (y(g) + 4) + '" fill="#8b95b5" font-size="11">' + g + '</text>';
  });
  return '<svg class="hist-chart" viewBox="0 0 ' + w + ' ' + h + '" preserveAspectRatio="none" role="img" aria-label="Fragmentation after each operation">' +
    '<defs><linearGradient id="hg" x1="0" x2="0" y1="0" y2="1"><stop offset="0" stop-color="#6c8cff" stop-opacity=".35"/><stop offset="1" stop-color="#6c8cff" stop-opacity="0"/></linearGradient></defs>' +
    grid +
    '<polygon points="' + padL + ',' + y(0) + ' ' + pts.join(' ') + ' ' + (padL + (history.length - 1) * step) + ',' + y(0) + '" fill="url(#hg)"/>' +
    '<polyline points="' + pts.join(' ') + '" fill="none" stroke="#6c8cff" stroke-width="2.5" stroke-linejoin="round" stroke-linecap="round"/>' +
    '<circle cx="' + (padL + (history.length - 1) * step) + '" cy="' + y(last) + '" r="4.5" fill="#fff" stroke="#6c8cff" stroke-width="2"/></svg>' +
    '<div class="axis-note"><span>oldest shown</span><span>after operation ' + opCount + ': ' + last.toFixed(3) + '</span></div>';
}
