
const NAMES = ["FIFO", "LRU", "Optimal"];
const COLORS = ["#e57373", "#66bb6a", "#648bd8"];
const CLASSES = ["fifo", "lru", "optimal"];

// Run a C algorithm through WebAssembly.
function runAlgo(a, refs, nf) {
  const n = refs.length;
  const size = n * (2 + nf);

  const rp = Module._malloc(n * 4);
  const op = Module._malloc(size * 4);

  if (!rp || !op) {
    if (rp) Module._free(rp);
    if (op) Module._free(op);
    throw new Error("Unable to allocate memory.");
  }

  try {
    Module.HEAP32.set(refs, rp >> 2);

    const faults = Module._simulate(a, rp, n, nf, op);

    if (faults < 0) {
      throw new Error("Invalid simulation input.");
    }

    const out = Array.from(
      Module.HEAP32.slice(
        op >> 2,
        (op >> 2) + size
      )
    );

    return {
      faults: faults,
      hits: n - faults,
      out: out
    };
  } finally {
    Module._free(rp);
    Module._free(op);
  }
}

// Build a result table for one algorithm.
function stepTable(a, res, n, nf) {
  let html = `
    <section class="algorithm-section ${CLASSES[a]}">
      <h3>${NAMES[a]}</h3>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Page</th>
  `;

  for (let i = 1; i <= nf; i++) {
    html += `<th>Frame ${i}</th>`;
  }

  html += `
              <th>Status</th>
            </tr>
          </thead>
          <tbody>
  `;

  for (let i = 0; i < n; i++) {
    const base = i * (2 + nf);
    const page = res.out[base];
    const hit = res.out[base + 1] === 1;

    html += `
      <tr class="${hit ? "hit" : "fault"}">
        <td>${page}</td>
    `;

    for (let j = 0; j < nf; j++) {
      const value = res.out[base + 2 + j];

      html += `
        <td>${value === -1 ? "—" : value}</td>
      `;
    }

    html += `
        <td>
          <span class="status-badge ${hit ? "hit-badge" : "fault-badge"}">
            ${hit ? "Hit" : "Fault"}
          </span>
        </td>
      </tr>
    `;
  }

  const hitRatio = (res.hits * 100 / n).toFixed(2);
  const faultRatio = (res.faults * 100 / n).toFixed(2);

  html += `
          </tbody>
        </table>
      </div>

      <div class="result-summary">
        <span class="summary-item">
          Total: <strong>${n}</strong>
        </span>

        <span class="summary-item">
          Hits: <strong>${res.hits}</strong>
        </span>

        <span class="summary-item">
          Faults: <strong>${res.faults}</strong>
        </span>

        <span class="summary-item">
          Hit ratio: <strong>${hitRatio}%</strong>
        </span>

        <span class="summary-item">
          Fault ratio: <strong>${faultRatio}%</strong>
        </span>
      </div>
    </section>
  `;

  return html;
}

// Build the comparison table and chart.
function comparisonTable(results, n) {
  let html = `
    <section class="comparison-section">
      <h3>Algorithm Comparison</h3>

      <div class="table-wrap">
        <table class="comparison-table">
          <thead>
            <tr>
              <th>Algorithm</th>
              <th>Page Hits</th>
              <th>Page Faults</th>
              <th>Hit Ratio</th>
              <th>Fault Ratio</th>
            </tr>
          </thead>
          <tbody>
  `;

  let best = 0;

  for (let a = 0; a < 3; a++) {
    const r = results[a];

    if (r.faults < results[best].faults) {
      best = a;
    }

    const hitRatio = (r.hits * 100 / n).toFixed(2);
    const faultRatio = (r.faults * 100 / n).toFixed(2);

    html += `
      <tr>
        <td>${NAMES[a]}</td>
        <td>${r.hits}</td>
        <td>${r.faults}</td>
        <td>${hitRatio}%</td>
        <td>${faultRatio}%</td>
      </tr>
    `;
  }

  html += `
          </tbody>
        </table>
      </div>

      <p class="best-result">
        Fewest Page Faults:
        <strong>${NAMES[best]}</strong>
        (${results[best].faults} faults)
      </p>

      <h3>Page Fault Comparison Chart</h3>
      <div class="chart-container">
  `;

  for (let a = 0; a < 3; a++) {
    const r = results[a];
    const width = (r.faults / n) * 100;

    html += `
      <div class="chart-row">
        <span class="chart-label">${NAMES[a]}</span>

        <div class="chart-track">
          <div
            class="chart-bar"
            style="
              width: ${width}%;
              background: ${COLORS[a]};
            "
          ></div>
        </div>

        <strong>${r.faults}</strong>
      </div>
    `;
  }

  html += `
      </div>
    </section>
  `;

  return html;
}

// Run button function.
function run() {
  const error = document.getElementById("error");
  const output = document.getElementById("output");

  error.textContent = "";
  output.innerHTML = "";

  // Read reference string.
  const text = document.getElementById("refs").value.trim();

  if (!text) {
    error.textContent = "Please enter a reference string.";
    return;
  }

  const parts = text.split(/[\s,]+/).filter(Boolean);

  if (parts.length > 100) {
    error.textContent = "Maximum 100 page references allowed.";
    return;
  }

  if (!parts.every(p => /^\d+$/.test(p))) {
    error.textContent = "Enter only non-negative whole numbers.";
    return;
  }

  const refs = parts.map(Number);

  if (
    !refs.every(Number.isSafeInteger) ||
    refs.some(p => p > 9999)
  ) {
    error.textContent = "Page numbers must be between 0 and 9999.";
    return;
  }

  // Read frame count.
  const frameText = document.getElementById("frames").value;

  if (
    !/^\d+$/.test(frameText) ||
    Number(frameText) < 1 ||
    Number(frameText) > 10
  ) {
    error.textContent = "Frames must be between 1 and 10.";
    return;
  }

  const nf = Number(frameText);
  const n = refs.length;

  // Read algorithm choice.
  const choice = document.getElementById("algo").value;

  const algorithms =
    choice === "all"
      ? [0, 1, 2]
      : [Number(choice)];

  if (
    algorithms.some(a => !Number.isInteger(a) || a < 0 || a > 2)
  ) {
    error.textContent = "Please select a valid algorithm.";
    return;
  }

  const results = {};
  let html = "";

  try {
    for (const a of algorithms) {
      results[a] = runAlgo(a, refs, nf);
      html += stepTable(a, results[a], n, nf);
    }

    if (choice === "all") {
      html += comparisonTable(results, n);
    }

    output.innerHTML = html;
  } catch (err) {
    error.textContent =
      err.message || "An error occurred during simulation.";
  }
}
