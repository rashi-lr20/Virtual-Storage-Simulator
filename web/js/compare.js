// =====================================================================================
//  MEMBER 4 — Comparison view
//  Draws the results of WorkloadSimulator::run for First / Best / Worst Fit.
//  The numbers come from the C++ program. This file only draws them.
// =====================================================================================

const STRATS = [['first', 'First Fit'], ['best', 'Best Fit'], ['worst', 'Worst Fit']];

function fmt(v, digits) {
  const n = Number(v);
  return Number.isInteger(n) && !digits ? String(n) : n.toFixed(digits === undefined ? 1 : digits);
}

// One horizontal bar chart. rows = array of results, key = which number to draw
function barChart(title, rows, key, digits) {
  let max = 0;
  rows.forEach(function (r) { max = Math.max(max, Number(r[key])); });
  let html = '<div class="chart"><h3>' + title + '</h3>';
  rows.forEach(function (r) {
    const pct = max > 0 ? (Number(r[key]) / max) * 100 : 0;
    html += '<div class="bar-row"><span><i class="sdot ' + r.strategy + '"></i>' + r.name.replace(' Fit', '') + '</span>' +
      '<div class="track"><div class="fill ' + r.strategy + '" style="width:' + pct + '%"></div></div>' +
      '<span class="val">' + fmt(r[key], digits) + '</span></div>';
  });
  return html + '</div>';
}

function resultTable(rows, averaged) {
  let html = '<table><thead><tr><th>Strategy</th><th>Success</th><th>Failed (frag.)</th><th>Failed (full)</th>' +
    '<th>Free segs</th><th>Largest</th><th>Avg frag.</th><th>Time (us)</th></tr></thead><tbody>';
  rows.forEach(function (r) {
    html += '<tr><td><i class="sdot ' + r.strategy + '"></i>' + r.name + '</td>' +
      '<td>' + fmt(r.success, averaged ? 1 : 0) + '</td><td>' + fmt(r.failFrag, averaged ? 1 : 0) + '</td>' +
      '<td>' + fmt(r.failNoSpace, averaged ? 1 : 0) + '</td><td>' + fmt(r.freeSegments, averaged ? 1 : 0) + '</td>' +
      '<td>' + fmt(r.largest, averaged ? 1 : 0) + '</td><td>' + Number(r.avgFrag).toFixed(3) + '</td>' +
      '<td>' + fmt(r.timeUs, 1) + '</td></tr>';
  });
  return html + '</tbody></table>';
}

function allConsistent(rows) {
  return rows.every(function (r) { return r.consistent; });
}

// data = answer of /api/compare
function renderCompare(data) {
  const out = document.getElementById('cmpOut');
  const rows = data.rows;
  out.innerHTML =
    '<div class="size-title">One workload <span>' + data.blocks + ' blocks · ' + data.ops + ' operations · seed ' + data.seed +
    ' · request size 1–' + data.max + '</span></div>' +
    resultTable(rows, false) +
    '<div class="charts">' +
      barChart('Successful allocations', rows, 'success', 0) +
      barChart('Failed because of fragmentation', rows, 'failFrag', 0) +
      barChart('Average fragmentation', rows, 'avgFrag', 3) +
    '</div>' +
    '<p class="hint">' + (allConsistent(rows) ? '✓ Array and linked list agreed after every step. ' : '✗ A consistency check failed. ') +
    'Try other seeds: one workload is not enough to draw a conclusion.</p>';
}

// data = answer of /api/experiment  (3 sizes, averaged over several seeds)
function renderExperiment(data) {
  const out = document.getElementById('cmpOut');
  let html = '';
  let ok = true;
  data.sizes.forEach(function (sz) {
    if (!allConsistent(sz.rows)) ok = false;
    html += '<div class="size-title">' + sz.name + ' <span>' + sz.blocks + ' blocks · ' + sz.ops + ' operations · average of ' +
      data.seeds + ' seeds</span></div>' + resultTable(sz.rows, true) +
      '<div class="charts">' +
        barChart('Failed (fragmentation)', sz.rows, 'failFrag', 1) +
        barChart('Average fragmentation', sz.rows, 'avgFrag', 3) +
      '</div><div style="height:18px"></div>';
  });
  html += '<p class="hint">' + (ok ? '✓ Array and linked list agreed after every step in every run. ' : '✗ A consistency check failed. ') +
    (data.csv ? 'Every individual run was saved to <code>results/results.csv</code> (open it in Excel for your report).' :
      'Create an empty <code>results</code> folder to also get a CSV file.') + '</p>';
  out.innerHTML = html;
}

// -------------------------------------------------------------------------------------
// Wiring for compare.html: presets, the Compare button and the experiment button
// -------------------------------------------------------------------------------------
function initComparePage() {
  const $ = function (id) { return document.getElementById(id); };

  document.querySelectorAll('.presets button').forEach(function (b) {
    b.addEventListener('click', function () {
      const p = b.dataset.p.split(',');           // blocks, operations, seed, max size
      $('cmpBlocks').value = p[0]; $('cmpOps').value = p[1]; $('cmpSeed').value = p[2]; $('cmpMax').value = p[3];
    });
  });

  $('btnCompare').addEventListener('click', async function () {
    $('cmpOut').innerHTML = '<p class="muted">Running in C++ ...</p>';
    try {
      const data = await App.api('compare', { blocks: $('cmpBlocks').value, ops: $('cmpOps').value, seed: $('cmpSeed').value, max: $('cmpMax').value });
      App.safe('compare.js', function () { renderCompare(data); });
    } catch (e) { $('cmpOut').innerHTML = '<p class="muted">' + e.message + '</p>'; }
  });

  $('btnExperiment').addEventListener('click', async function () {
    $('cmpOut').innerHTML = '<p class="muted">Running the small, medium and large experiments in C++ (a few seconds) ...</p>';
    try {
      const data = await App.api('experiment');
      App.safe('compare.js', function () { renderExperiment(data); });
    } catch (e) { $('cmpOut').innerHTML = '<p class="muted">' + e.message + '</p>'; }
  });
}
