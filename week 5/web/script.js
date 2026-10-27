const NAMES = ["FIFO", "LRU", "Optimal"];
const COLORS = ["#e74c3c", "#3498db", "#2ecc71"];

// calls the C function and reads the result
function runAlgo(a, refs, nf) {
  const n = refs.length, size = n * (2 + nf);
  const rp = Module._malloc(n * 4), op = Module._malloc(size * 4);
  Module.HEAP32.set(refs, rp >> 2);
  const faults = Module._simulate(a, rp, n, nf, op);
  const out = Array.from(Module.HEAP32.slice(op >> 2, (op >> 2) + size));
  Module._free(rp); Module._free(op);
  return { faults, hits: n - faults, out };
}

// builds the step-by-step table for one algorithm
function stepTable(a, res, n, nf) {
  let h = `<h3>${NAMES[a]}</h3><table><tr><th>Page</th>`;
  for (let i = 1; i <= nf; i++) h += `<th>Frame ${i}</th>`;
  h += "<th>Status</th></tr>";
  for (let i = 0; i < n; i++) {
    const b = i * (2 + nf), hit = res.out[b + 1];
    h += `<tr class="${hit ? "hit" : "fault"}"><td>${res.out[b]}</td>`;
    for (let j = 0; j < nf; j++) {
      const v = res.out[b + 2 + j];
      h += `<td>${v === -1 ? "—" : v}</td>`;
    }
    h += `<td>${hit ? "Hit" : "Fault"}</td></tr>`;
  }
  h += "</table>";
  h += `<p>Total: ${n} | Hits: ${res.hits} | Faults: ${res.faults} | ` +
       `Hit ratio: ${(res.hits * 100 / n).toFixed(2)}% | ` +
       `Fault ratio: ${(res.faults * 100 / n).toFixed(2)}%</p>`;
  return h;
}

// runs when the Run button is clicked
function run() {
  const err = document.getElementById("error");
  const outDiv = document.getElementById("output");
  err.textContent = ""; outDiv.innerHTML = "";

  // check the reference string
  const parts = document.getElementById("refs").value.split(/[\s,]+/).filter(Boolean);
  if (parts.length === 0) return err.textContent = "Reference string is empty.";
  if (!parts.every(p => /^\d+$/.test(p)))
    return err.textContent = "Use only whole numbers (0 or more).";
  if (parts.length > 100) return err.textContent = "Maximum 100 pages.";
  const refs = parts.map(Number);

  // check the frames
  const fs = document.getElementById("frames").value;
  if (!/^\d+$/.test(fs) || +fs < 1 || +fs > 10)
    return err.textContent = "Frames must be a number from 1 to 10.";
  const nf = +fs, n = refs.length;

  // run the chosen algorithm(s)
  const choice = document.getElementById("algo").value;
  const list = choice === "all" ? [0, 1, 2] : [+choice];
  const results = {};
  let html = "";
  for (const a of list) {
    results[a] = runAlgo(a, refs, nf);
    html += stepTable(a, results[a], n, nf);
  }

  // comparison table and chart
  if (choice === "all") {
    html += "<h3>Comparison</h3><table><tr><th>Algorithm</th><th>Page Hits</th>" +
            "<th>Page Faults</th><th>Hit Ratio</th><th>Fault Ratio</th></tr>";
    let best = 0;
    for (const a of list) {
      const r = results[a];
      if (r.faults < results[best].faults) best = a;
      html += `<tr><td>${NAMES[a]}</td><td>${r.hits}</td><td>${r.faults}</td>` +
              `<td>${(r.hits * 100 / n).toFixed(2)}%</td>` +
              `<td>${(r.faults * 100 / n).toFixed(2)}%</td></tr>`;
    }
    html += `</table><p><b>Fewest page faults: ${NAMES[best]} ` +
            `(${results[best].faults} faults)</b></p><h3>Page Faults Chart</h3>`;
    for (const a of list) {
      const w = Math.max(results[a].faults / n * 100, 4);
      html += `<div class="bar" style="width:${w}%;background:${COLORS[a]}">` +
              `${NAMES[a]}: ${results[a].faults}</div>`;
    }
  }
  outDiv.innerHTML = html;
}