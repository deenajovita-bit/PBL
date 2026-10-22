
const safetyResult = document.getElementById("safety-result");
const bankerResult = document.getElementById("banker-result");
const detectionResult = document.getElementById("detection-result");
const recoveryResult = document.getElementById("recovery-result");
const systemState = document.getElementById("system-state");

// Example data for the front-end preview.
// These results are illustrative and are not yet connected to the C backend.
function showSafetyPreview() {
  const sequence = ["P1", "P3", "P4", "P0", "P2"];

  systemState.textContent = "Preview only";
  systemState.className = "";
  systemState.style.color = "#fbbf24";

  safetyResult.textContent =
    "Preview only — the live Banker's Algorithm has not been connected yet.";

  bankerResult.textContent =
    "Example safe sequence: " + sequence.join(" → ") +
    ". This sequence must be verified against the actual input before being treated as a result.";
}

document.getElementById("check-safe").addEventListener("click", showSafetyPreview);
document.getElementById("banker-check").addEventListener("click", showSafetyPreview);

document.getElementById("detect-button").addEventListener("click", () => {
  detectionResult.textContent =
    "Preview only — connect the C deadlock detection algorithm to identify actual deadlocks.";
});

document.getElementById("recovery-button").addEventListener("click", () => {
  recoveryResult.textContent =
    "Preview only — no process or resource has been changed. Connect the C recovery module to perform recovery.";
});
