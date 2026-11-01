
const safetyResult = document.getElementById("safety-result");
const bankerResult = document.getElementById("banker-result");
const detectionResult = document.getElementById("detection-result");
const recoveryResult = document.getElementById("recovery-result");
const systemState = document.getElementById("system-state");

// Connect to the C backend and run the real Banker's Algorithm.
async function checkSafety() {
  systemState.textContent = "Checking...";
  safetyResult.textContent = "Running the C Banker's Algorithm...";
  bankerResult.textContent = "";

  try {
    const response = await fetch("http://localhost:8001/api/safety");

    if (!response.ok) {
      throw new Error("Backend request failed.");
    }

    const result = await response.json();

    if (result.safe) {
      systemState.textContent = "Safe State";
      systemState.style.color = "#34d399";

      safetyResult.textContent =
        "The C Banker's Algorithm confirms that the system is safe.";

      bankerResult.textContent =
        "Safe sequence: " + result.sequence.join(" → ");
    } else {
      systemState.textContent = "Unsafe State";
      systemState.style.color = "#f87171";

      safetyResult.textContent =
        "The system is unsafe. No complete safe sequence was found.";

      bankerResult.textContent = "";
    }
  } catch (error) {
    systemState.textContent = "Connection Error";
    safetyResult.textContent =
      "Could not connect to the C backend. Make sure it is running on port 8001.";
    bankerResult.textContent = error.message;
  }
}

// Connect both safety buttons.
document.getElementById("check-safe").addEventListener("click", checkSafety);
document.getElementById("banker-check").addEventListener("click", checkSafety);

// These features are not connected to the C backend yet.
document.getElementById("detect-button").addEventListener("click", () => {
  detectionResult.textContent =
    "Deadlock detection will be connected in the next step.";
});

document.getElementById("recovery-button").addEventListener("click", () => {
  recoveryResult.textContent =
    "Deadlock recovery will be connected in the next step.";
});

