// =====================================================================================
//  MEMBER 4 — shared by every page.
//   * App.api(...)      talks to the C++ program (server.cpp)
//   * App.start(...)    builds the top navigation + the action bar, loads the current state and
//                       calls YOUR page's render function every time something changes
//   * the C++ program keeps ONE simulation, so the state is kept when you move between pages
// =====================================================================================

const $ = function (id) { return document.getElementById(id); };

const PAGES = [
  ['learn.html', 'Learn'],
  ['simulator.html', 'Simulator'],
  ['freelist.html', 'Free list'],
  ['analysis.html', 'Statistics'],
  ['compare.html', 'Compare']
];

const App = {
  state: null,
  preview: null,
  renderFn: null,
  previewFn: null,
  busy: false
};

// ---------- talking to the C++ server ----------
App.api = async function (path, params, method) {
  const qs = new URLSearchParams(params || {}).toString();
  const res = await fetch('/api/' + path + (qs ? '?' + qs : ''), { method: method || 'POST' });
  const data = await res.json();
  if (!res.ok || data.error) throw new Error(data.error || 'Server error');
  return data;
};

// ---------- small UI helpers ----------
App.caption = function (text, kind) {
  const c = $('caption');
  if (!c) return;
  c.className = 'caption' + (kind ? ' ' + kind : '');
  c.innerHTML = text;
};

let toastTimer = null;
App.toast = function (text, kind) {
  let t = $('toast');
  if (!t) { t = document.createElement('div'); t.id = 'toast'; t.className = 'toast'; document.body.appendChild(t); }
  t.textContent = text;
  t.className = 'toast show ' + (kind || '');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(function () { t.className = 'toast'; }, 2400);
};

App.safe = function (name, fn) {
  try { fn(); } catch (e) {
    console.error(name, e);
    let b = $('errorBanner');
    if (!b) { b = document.createElement('div'); b.id = 'errorBanner'; b.className = 'error-banner'; const page = document.querySelector('.page'); if (page) page.prepend(b); }
    b.hidden = false;
    b.textContent = 'Problem in ' + name + ': ' + e.message + ' (press F12 to see details)';
  }
};

const sleep = function (ms) { return new Promise(function (r) { setTimeout(r, ms); }); };

// ---------- navigation bar ----------
function buildNav(active) {
  const host = $('nav');
  if (!host) return;
  let links = '';
  PAGES.forEach(function (p) {
    links += '<a href="' + p[0] + '"' + (p[0] === active ? ' class="active"' : '') + '>' + p[1] + '</a>';
  });
  host.className = 'nav';
  host.innerHTML =
    '<a class="brand" href="index.html"><span class="logo" aria-hidden="true">▦</span><span>Storage<b>Sim</b></span></a>' +
    '<div class="links">' + links + '</div>' +
    '<div class="nav-right"><span class="pill" id="navStrategy">…</span><span class="pill" id="navBlocks">…</span></div>';
}

// ---------- action bar ----------
function buildActionBar(mode) {
  const host = $('actionBar');
  if (!host) return;
  host.className = 'actionbar';
  let html =
    '<div class="ab-row">' +
      '<div class="ab-group"><div class="seg" id="strategySeg" role="group" aria-label="Allocation strategy">' +
        '<button data-s="first">First Fit</button><button data-s="best">Best Fit</button><button data-s="worst">Worst Fit</button></div></div>' +
      '<div class="ab-group">' +
        '<label class="field">Blocks <input id="totalBlocks" type="number" min="5" max="400" value="40"></label>' +
        '<button id="btnReset" class="btn">Reset</button>' +
        '<button id="btnRandom" class="btn">🎲 Random fill</button>' +
      '</div>' +
    '</div>';
  if (mode === 'full') {
    html +=
      '<div class="ab-row">' +
        '<form id="formAlloc" class="ab-group" autocomplete="off">' +
          '<label class="field id">File <input id="allocId" maxlength="4" value="A" aria-label="File id (letter or number)"></label>' +
          '<label class="field">Blocks <input id="allocSize" type="number" min="1" value="5"></label>' +
          '<button class="btn primary" type="submit">Allocate</button>' +
        '</form>' +
        '<div class="quick" id="quickSizes"><span class="muted">Size:</span>' +
          '<button type="button" data-n="2">2</button><button type="button" data-n="4">4</button><button type="button" data-n="6">6</button>' +
          '<button type="button" data-n="8">8</button><button type="button" data-n="12">12</button></div>' +
        '<div class="ab-sep"></div>' +
        '<form id="formRelease" class="ab-group" autocomplete="off">' +
          '<label class="field id">File <input id="relId" maxlength="4" value="A" aria-label="File id to release"></label>' +
          '<button class="btn danger" type="submit">Release</button>' +
        '</form>' +
      '</div>';
  }
  host.innerHTML = html;

  document.querySelectorAll('#strategySeg button').forEach(function (b) {
    b.addEventListener('click', function () {
      App.run(function () { return App.api('strategy', { name: b.dataset.s }); }).then(App.refreshPreview);
    });
  });
  $('btnReset').addEventListener('click', function () { App.reset($('totalBlocks').value).catch(function (e) { App.toast(e.message, 'bad'); }); });
  $('btnRandom').addEventListener('click', function () {
    const max = Math.max(2, Math.floor((App.state ? App.state.total : 40) / 6));
    App.run(function () { return App.api('random', { count: 14, seed: Math.floor(Math.random() * 1000) + 1, max: max }); }).then(App.refreshPreview);
  });

  if (mode === 'full') {
    $('formAlloc').addEventListener('submit', function (ev) {
      ev.preventDefault();
      App.run(function () { return App.api('allocate', { id: $('allocId').value.trim(), size: $('allocSize').value }); }).then(function () {
        if (App.state && App.state.action.ok) $('relId').value = $('allocId').value;
        App.refreshPreview();
      });
    });
    $('formRelease').addEventListener('submit', function (ev) { ev.preventDefault(); requestRelease($('relId').value.trim()); });
    $('allocSize').addEventListener('input', App.refreshPreview);
    document.querySelectorAll('#quickSizes button').forEach(function (b) {
      b.addEventListener('click', function () { $('allocSize').value = b.dataset.n; App.refreshPreview(); });
    });
  }
}

// ---------- state ----------
App.setState = function (s, quiet) {
  App.state = s;
  const action = quiet ? null : s.action;

  const ns = $('navStrategy'), nb = $('navBlocks');
  if (ns) { ns.textContent = s.strategyName; ns.className = 'pill s-' + s.strategy; }
  if (nb) nb.textContent = s.total + ' blocks';
  document.querySelectorAll('#strategySeg button').forEach(function (b) { b.classList.toggle('active', b.dataset.s === s.strategy); });
  if ($('totalBlocks') && document.activeElement !== $('totalBlocks')) $('totalBlocks').value = s.total;

  const eb = $('errorBanner'); if (eb) eb.hidden = true;
  if (App.renderFn) App.renderFn(s, action, App.preview);
  renderLog(s);

  if (!quiet && s.action && s.action.message && s.action.type !== 'none') {
    App.caption(s.action.message + (s.action.result ? ' → ' + s.action.result : ''),
      s.action.type === 'allocate' ? (s.action.ok ? 'good' : 'bad') : '');
  }
  suggestNextId();
};

App.run = async function (promiseFn) {
  try { App.setState(await promiseFn()); }
  catch (e) { App.toast(e.message, 'bad'); App.caption(e.message, 'bad'); }
};

App.reset = async function (blocks, strategy) {
  App.preview = null;
  const params = { blocks: blocks };
  if (strategy) params.strategy = strategy;
  App.setState(await App.api('init', params));
  await App.refreshPreview();
};

App.refreshPreview = async function () {
  const box = $('allocSize');
  if (!box) return;
  const n = parseInt(box.value, 10);
  try {
    App.preview = n > 0 ? await App.api('preview', { size: n }, 'GET') : null;
    if (App.state && App.previewFn) App.previewFn(App.state, App.preview);
  } catch (e) { }
};

function requestRelease(label) {
  return App.run(function () { return App.api('release', { id: label }); }).then(App.refreshPreview);
}

function renderLog(s) {
  const list = $('logList');
  if (!list) return;
  list.innerHTML = '';
  if (s.log.length === 0) { list.innerHTML = '<li><span>Nothing yet.</span></li>'; return; }
  s.log.slice().reverse().forEach(function (e) {
    const li = document.createElement('li');
    li.className = e.ok ? '' : 'bad';
    li.innerHTML = '<strong></strong><span></span>';
    li.querySelector('strong').textContent = e.text;
    li.querySelector('span').textContent = e.result;
    list.appendChild(li);
  });
}

function suggestNextId() {
  const box = $('allocId');
  if (!box || !App.state) return;
  const used = {};
  App.state.files.forEach(function (f) { used[f.label] = true; });
  for (let i = 0; i < 26; i++) {
    const letter = String.fromCharCode(65 + i);
    if (!used[letter]) { if (document.activeElement !== box) box.value = letter; return; }
  }
}

// ---------- guided demo ----------
App.runDemo = async function () {
  if (App.busy) return;
  App.busy = true;
  document.querySelectorAll('button').forEach(function (b) { b.disabled = true; });
  try {
    await App.reset(20, 'first');
    const steps = [
      ['Allocate file A: 5 blocks. First Fit takes the first gap that is big enough.', 'allocate', 'A', 5],
      ['Allocate file B: 4 blocks.', 'allocate', 'B', 4],
      ['Allocate file C: 5 blocks.', 'allocate', 'C', 5],
      ['Release A. Its 5 blocks are free again, but B and C separate them from the free blocks at the end.', 'release', 'A'],
      ['Allocate D: 7 blocks. Free blocks: 5 + 6 = 11, but the biggest single gap is only 6. This is fragmentation.', 'allocate', 'D', 7]
    ];
    for (const s of steps) {
      App.caption('<b>Demo:</b> ' + s[0]);
      await sleep(1700);
      App.setState(s[1] === 'allocate' ? await App.api('allocate', { id: s[2], size: s[3] }) : await App.api('release', { id: s[2] }));
      App.caption('<b>Demo:</b> ' + s[0], s[1] === 'allocate' && !App.state.action.ok ? 'bad' : '');
      await sleep(1500);
    }
  } catch (e) { App.toast(e.message, 'bad'); }
  document.querySelectorAll('button').forEach(function (b) { b.disabled = false; });
  App.busy = false;
};

// ---------- start a page ----------
App.start = function (opts) {
  App.renderFn = opts.render || null;
  App.previewFn = opts.onPreview || null;
  buildNav(opts.page);
  buildActionBar(opts.bar || 'none');
  if ($('btnDemo')) $('btnDemo').addEventListener('click', App.runDemo);

  App.api('state', {}, 'GET')
    .then(function (s) { App.setState(s, true); return App.refreshPreview(); })
    .catch(function (e) {
      App.caption('Cannot reach the C++ program: ' + e.message + '. Is <code>gui_server</code> running? Start it, then reload this page.', 'bad');
    });
};
