// ==========================================
// DEADLOCK SIMULATION SYSTEM - FRONTEND
// C BACKEND VERSION
// ==========================================

const API_URL =
  "https://super-duper-funicular-7vqwwprgxx4jhxx5g-8001.app.github.dev";


// ==========================================
// DOM ELEMENTS
// ==========================================

const stateBadge =
  document.getElementById("stateBadge");

const statusBanner =
  document.getElementById("statusBanner");

const statusTitle =
  document.getElementById("statusTitle");

const statusMessage =
  document.getElementById("statusMessage");

const bankerResult =
  document.getElementById("bankerResult");

const safeSequence =
  document.getElementById("safeSequence");

const deadlockResult =
  document.getElementById("deadlockResult");

const deadlockIcon =
  document.getElementById("deadlockIcon");

const deadlockedProcesses =
  document.getElementById("deadlockedProcesses");

const backendStatus =
  document.getElementById("backendStatus");

let lastState = null;


// ==========================================
// STATUS DISPLAY
// ==========================================

function updateStatus(type, title, message) {

  if (stateBadge) {

    stateBadge.className =
      "state-badge " + type;

    if (type === "safe") {

      stateBadge.textContent =
        "● SAFE";

    } else if (type === "unsafe") {

      stateBadge.textContent =
        "● UNSAFE";

    } else if (type === "deadlock") {

      stateBadge.textContent =
        "● DEADLOCKED";

    } else {

      stateBadge.textContent =
        "● NOT TESTED";
    }
  }


  if (statusBanner) {

    statusBanner.className =
      "status-banner " + type;
  }


  if (statusTitle) {

    statusTitle.textContent =
      title;
  }


  if (statusMessage) {

    statusMessage.textContent =
      message;
  }
}


// ==========================================
// SAFE SEQUENCE DISPLAY
// ==========================================

function displaySequence(sequence) {

  if (!safeSequence) {
    return;
  }

  safeSequence.innerHTML = "";


  if (
    !sequence ||
    sequence.length === 0
  ) {

    safeSequence.innerHTML =
      "<span>?</span>";

    return;
  }


  sequence.forEach(
    (process, index) => {

      const node =
        document.createElement("span");

      node.textContent =
        process;

      safeSequence.appendChild(node);


      if (
        index <
        sequence.length - 1
      ) {

        const arrow =
          document.createElement("b");

        arrow.textContent =
          "→";

        safeSequence.appendChild(
          arrow
        );
      }
    }
  );
}


// ==========================================
// RESOURCE UTILIZATION
// ==========================================

function updateUtilization(
  total,
  available
) {

  if (!Array.isArray(total)) {
    return;
  }

  if (!Array.isArray(available)) {
    return;
  }


  total.forEach(
    (resourceTotal, index) => {

      const percentText =
        document.getElementById(
          `r${index}Percent`
        );

      const bar =
        document.getElementById(
          `r${index}Bar`
        );


      let percentage = 0;


      if (resourceTotal > 0) {

        const used =
          resourceTotal -
          (available[index] || 0);

        percentage =
          Math.round(
            (used / resourceTotal) *
            100
          );

        percentage =
          Math.max(
            0,
            Math.min(
              100,
              percentage
            )
          );
      }


      if (percentText) {

        percentText.textContent =
          percentage + "%";
      }


      if (bar) {

        bar.style.width =
          percentage + "%";
      }
    }
  );
}


// ==========================================
// LOAD SYSTEM STATE FROM C BACKEND
// ==========================================

async function loadSystemState() {

  const response =
    await fetch(
      `${API_URL}/api/state`
    );


  if (!response.ok) {

    throw new Error(
      `State endpoint returned HTTP ${response.status}`
    );
  }


  const state =
    await response.json();

  lastState = state;


  console.log(
    "C backend state:",
    state
  );


  // ========================================
  // BACKEND STATUS
  // ========================================

  if (backendStatus) {

    backendStatus.textContent =
      "● C Backend Connected";

    backendStatus.className =
      "text-safe";
  }


  // ========================================
  // PROCESS COUNT
  // ========================================

  const processCount =
    document.getElementById(
      "processCount"
    );

  if (processCount) {

    processCount.value =
      state.processes ?? 0;
  }


  // ========================================
  // RESOURCE COUNT
  // ========================================

  const resourceCount =
    document.getElementById(
      "resourceCount"
    );

  if (resourceCount) {

    resourceCount.value =
      state.resources ?? 0;
  }


  // ========================================
  // TOTAL RESOURCES
  // ========================================

  if (Array.isArray(state.total)) {

    state.total.forEach(
      (value, index) => {

        const input =
          document.getElementById(
            `totalR${index}`
          );

        if (input) {

          input.value =
            value;
        }
      }
    );
  }


  // ========================================
  // AVAILABLE VECTOR
  // ========================================

  if (
    Array.isArray(
      state.available
    )
  ) {

    state.available.forEach(
      (value, index) => {

        const cell =
          document.getElementById(
            `availableR${index}`
          );

        if (cell) {

          cell.textContent =
            value;
        }
      }
    );
  }


  // ========================================
  // RESOURCE UTILIZATION
  // ========================================

  updateUtilization(
    state.total || [],
    state.available || []
  );


  // ========================================
  // MATRIX
  // ========================================

  const rows =
    document.querySelectorAll(
      "#matrixBody tr"
    );


  rows.forEach(
    (row, processIndex) => {

      // Hide processes not currently
      // used by the backend.

      if (
        processIndex >=
        (state.processes || 0)
      ) {

        row.style.display =
          "none";

        return;
      }


      row.style.display = "";


      const allocation =
        row.querySelectorAll(
          ".allocation"
        );

      const maximum =
        row.querySelectorAll(
          ".maximum"
        );

      const needs =
        row.querySelectorAll(
          ".need"
        );

      const pending =
        row.querySelectorAll(
          ".pending"
        );


      // ====================================
      // ALLOCATION MATRIX
      // ====================================

      if (
        state.allocation &&
        state.allocation[
          processIndex
        ]
      ) {

        state.allocation[
          processIndex
        ].forEach(
          (
            value,
            resourceIndex
          ) => {

            if (
              allocation[
                resourceIndex
              ]
            ) {

              allocation[
                resourceIndex
              ].value =
                value;
            }
          }
        );
      }


      // ====================================
      // MAXIMUM MATRIX
      // ====================================

      if (
        state.maximum &&
        state.maximum[
          processIndex
        ]
      ) {

        state.maximum[
          processIndex
        ].forEach(
          (
            value,
            resourceIndex
          ) => {

            if (
              maximum[
                resourceIndex
              ]
            ) {

              maximum[
                resourceIndex
              ].value =
                value;
            }
          }
        );
      }


      // ====================================
      // NEED MATRIX
      // ====================================

      if (
        state.need &&
        state.need[
          processIndex
        ]
      ) {

        state.need[
          processIndex
        ].forEach(
          (
            value,
            resourceIndex
          ) => {

            if (
              needs[
                resourceIndex
              ]
            ) {

              needs[
                resourceIndex
              ].textContent =
                value;
            }
          }
        );
      }


      // ====================================
      // PENDING REQUEST MATRIX
      // ====================================

      if (
        state.request &&
        state.request[
          processIndex
        ]
      ) {

        state.request[
          processIndex
        ].forEach(
          (
            value,
            resourceIndex
          ) => {

            if (
              pending[
                resourceIndex
              ]
            ) {

              pending[
                resourceIndex
              ].textContent =
                value;
            }
          }
        );
      }


      // ====================================
      // ACTIVE / TERMINATED PROCESS
      // ====================================

      if (
        state.active &&
        state.active[
          processIndex
        ] === 0
      ) {

        row.style.opacity =
          "0.35";

      } else {

        row.style.opacity =
          "1";
      }
    }
  );


  // ========================================
  // DRAW RESOURCE ALLOCATION GRAPH
  // ========================================

  drawResourceAllocationGraph(
    state
  );


  return state;
}


// ==========================================
// BANKER'S ALGORITHM
// ==========================================

async function runBankerAlgorithm() {

  if (bankerResult) {

    bankerResult.textContent =
      "Running Banker's Algorithm...";
  }


  if (safeSequence) {

    safeSequence.innerHTML =
      "<span>...</span>";
  }


  try {

    const response =
      await fetch(
        `${API_URL}/api/safety`
      );


    if (!response.ok) {

      throw new Error(
        `Safety endpoint returned HTTP ${response.status}`
      );
    }


    const result =
      await response.json();


    console.log(
      "Banker result:",
      result
    );


    if (result.safe) {

      updateStatus(
        "safe",
        "SAFE STATE",
        "All processes can complete. A safe sequence exists."
      );


      displaySequence(
        result.sequence || []
      );


      if (bankerResult) {

        bankerResult.textContent =
          "Safe sequence found: " +
          (result.sequence || [])
            .join(" → ");

        bankerResult.className =
          "result-text text-safe";
      }


    } else {

      updateStatus(
        "unsafe",
        "UNSAFE STATE",
        "The system cannot guarantee that all processes will complete."
      );


      if (safeSequence) {

        safeSequence.innerHTML =
          "<span>!</span>";
      }


      if (bankerResult) {

        bankerResult.textContent =
          "No complete safe sequence was found.";

        bankerResult.className =
          "result-text text-warning";
      }
    }


    return result;


  } catch (error) {

    console.error(
      "Banker error:",
      error
    );


    updateStatus(
      "neutral",
      "CONNECTION ERROR",
      error.message
    );


    throw error;
  }
}


// ==========================================
// DEADLOCK DETECTION
// ==========================================

async function detectDeadlock() {

  if (deadlockResult) {

    deadlockResult.textContent =
      "Running deadlock detection...";
  }


  if (deadlockedProcesses) {

    deadlockedProcesses.innerHTML =
      "";
  }


  // Remove previous red process highlight

  document
    .querySelectorAll(
      ".deadlocked-node"
    )
    .forEach(node => {

      node.classList.remove(
        "deadlocked-node"
      );
    });


  try {

    const response =
      await fetch(
        `${API_URL}/api/detection`
      );


    if (!response.ok) {

      throw new Error(
        `Detection endpoint returned HTTP ${response.status}`
      );
    }


    const result =
      await response.json();


    console.log(
      "Detection result:",
      result
    );


    if (result.deadlock) {

      updateStatus(
        "deadlock",
        "DEADLOCK DETECTED",
        `${result.count} process(es) are involved in a deadlock.`
      );


      if (deadlockIcon) {

        deadlockIcon.textContent =
          "!";

        deadlockIcon.className =
          "large-icon text-danger";
      }


      if (deadlockResult) {

        deadlockResult.textContent =
          "Deadlocked processes: " +
          (result.processes || [])
            .join(", ");
      }


      if (deadlockedProcesses) {

        (result.processes || [])
          .forEach(process => {

            const tag =
              document.createElement(
                "span"
              );

            tag.textContent =
              process;

            deadlockedProcesses
              .appendChild(tag);


            const graphNode =
              document.getElementById(
                "graph" + process
              );


            if (graphNode) {

              graphNode.classList.add(
                "deadlocked-node"
              );
            }
          });
      }


    } else {

      if (deadlockIcon) {

        deadlockIcon.textContent =
          "✓";

        deadlockIcon.className =
          "large-icon text-safe";
      }


      if (deadlockResult) {

        deadlockResult.textContent =
          "No deadlock detected.";
      }


      updateStatus(
        "safe",
        "NO DEADLOCK",
        "No processes are currently deadlocked."
      );
    }


    return result;


  } catch (error) {

    console.error(
      "Detection error:",
      error
    );


    if (deadlockResult) {

      deadlockResult.textContent =
        "Deadlock detection failed: " +
        error.message;
    }


    if (backendStatus) {

      backendStatus.textContent =
        "● Backend Offline";

      backendStatus.className =
        "text-danger";
    }


    throw error;
  }
}


// ==========================================
// END OF PART 1
// ==========================================
// ==========================================
// PART 2
// EXAMPLES, RESET, REQUEST & RECOVERY
// ==========================================


// ==========================================
// SAFE EXAMPLE
// ==========================================

const safeExampleButton =
  document.getElementById(
    "safeExample"
  );

if (safeExampleButton) {

  safeExampleButton.addEventListener(
    "click",
    async () => {

      try {

        const response =
          await fetch(
            `${API_URL}/api/reset`
          );

        if (!response.ok) {

          throw new Error(
            `Reset endpoint returned HTTP ${response.status}`
          );
        }

        await response.json();

        await loadSystemState();

        clearDeadlockDisplay();

        await runBankerAlgorithm();

      } catch (error) {

        console.error(
          "Safe example error:",
          error
        );

        updateStatus(
          "neutral",
          "ERROR",
          "Safe example error: " +
            error.message
        );
      }
    }
  );
}


// ==========================================
// UNSAFE EXAMPLE
// ==========================================

const unsafeExampleButton =
  document.getElementById(
    "unsafeExample"
  );

if (unsafeExampleButton) {

  unsafeExampleButton.addEventListener(
    "click",
    async () => {

      try {

        const response =
          await fetch(
            `${API_URL}/api/example/unsafe`
          );

        if (!response.ok) {

          throw new Error(
            `Unsafe endpoint returned HTTP ${response.status}`
          );
        }

        await response.json();

        await loadSystemState();

        clearDeadlockDisplay();

        await runBankerAlgorithm();

      } catch (error) {

        console.error(
          "Unsafe example error:",
          error
        );

        updateStatus(
          "neutral",
          "ERROR",
          "Unsafe example error: " +
            error.message
        );
      }
    }
  );
}


// ==========================================
// DEADLOCK EXAMPLE
// ==========================================

const deadlockExampleButton =
  document.getElementById(
    "deadlockExample"
  );

if (deadlockExampleButton) {

  deadlockExampleButton.addEventListener(
    "click",
    async () => {

      try {

        const response =
          await fetch(
            `${API_URL}/api/example/deadlock`
          );

        if (!response.ok) {

          throw new Error(
            `Deadlock endpoint returned HTTP ${response.status}`
          );
        }

        const data =
          await response.json();

        console.log(
          "Deadlock example loaded:",
          data
        );


        // Load deadlock state from backend

        const state =
          await loadSystemState();


        // Clear previous safe sequence

        if (safeSequence) {

          safeSequence.innerHTML =
            "<span>!</span>";
        }


        if (bankerResult) {

          bankerResult.textContent =
            "No safe sequence exists for this deadlocked state.";

          bankerResult.className =
            "result-text text-warning";
        }


        // Draw RAG using deadlock state

        drawResourceAllocationGraph(
          state
        );


        // Run actual C deadlock detector

        await detectDeadlock();


        // Redraw arrows after detection.
        // Process node highlighting remains visible.

        requestAnimationFrame(() => {

          drawResourceAllocationGraph(
            state
          );
        });


      } catch (error) {

        console.error(
          "Deadlock example error:",
          error
        );

        updateStatus(
          "neutral",
          "ERROR",
          "Deadlock example error: " +
            error.message
        );
      }
    }
  );
}


// ==========================================
// RESET SYSTEM
// ==========================================

const resetButton =
  document.getElementById(
    "resetSystem"
  );

if (resetButton) {

  resetButton.addEventListener(
    "click",
    async () => {

      try {

        const response =
          await fetch(
            `${API_URL}/api/reset`
          );

        if (!response.ok) {

          throw new Error(
            `Reset endpoint returned HTTP ${response.status}`
          );
        }

        await response.json();

        await loadSystemState();


        updateStatus(
          "neutral",
          "SYSTEM RESET",
          "System restored to the original safe state."
        );


        if (safeSequence) {

          safeSequence.innerHTML =
            "<span>?</span>";
        }


        if (bankerResult) {

          bankerResult.textContent =
            "Run Banker's Algorithm to analyse the current state.";

          bankerResult.className =
            "result-text";
        }


        clearDeadlockDisplay();


        const requestResult =
          document.getElementById(
            "requestResult"
          );

        if (requestResult) {

          requestResult.textContent =
            "System reset. Ready for a new request.";
        }


      } catch (error) {

        console.error(
          "Reset error:",
          error
        );

        updateStatus(
          "neutral",
          "RESET ERROR",
          "Reset error: " +
            error.message
        );
      }
    }
  );
}


// ==========================================
// CLEAR DEADLOCK DISPLAY
// ==========================================

function clearDeadlockDisplay() {

  if (deadlockIcon) {

    deadlockIcon.textContent =
      "?";

    deadlockIcon.className =
      "large-icon";
  }


  if (deadlockResult) {

    deadlockResult.textContent =
      "Deadlock detection has not been run.";
  }


  if (deadlockedProcesses) {

    deadlockedProcesses.innerHTML =
      "";
  }


  document
    .querySelectorAll(
      ".deadlocked-node"
    )
    .forEach(node => {

      node.classList.remove(
        "deadlocked-node"
      );
    });
}


// ==========================================
// CREATE SYSTEM
// ==========================================

const createSystemButton =
  document.getElementById(
    "createSystem"
  );

if (createSystemButton) {

  createSystemButton.addEventListener(
    "click",
    () => {

      const processes =
        document.getElementById(
          "processCount"
        )?.value || 0;

      const resources =
        document.getElementById(
          "resourceCount"
        )?.value || 0;


      updateStatus(
        "neutral",
        "SYSTEM CONFIGURATION",
        `${processes} processes and ${resources} resource types selected.`
      );
    }
  );
}


// ==========================================
// RESOURCE REQUEST
// ==========================================

const sendRequestButton =
  document.getElementById(
    "sendRequest"
  );

if (sendRequestButton) {

  sendRequestButton.addEventListener(
    "click",
    async () => {

      const process =
        Number(
          document.getElementById(
            "requestProcess"
          )?.value || 0
        );


      const r0 =
        Number(
          document.getElementById(
            "requestR0"
          )?.value || 0
        );


      const r1 =
        Number(
          document.getElementById(
            "requestR1"
          )?.value || 0
        );


      const r2 =
        Number(
          document.getElementById(
            "requestR2"
          )?.value || 0
        );


      const mode =
        document.getElementById(
          "requestMode"
        )?.value;


      const banker =
        mode === "banker"
          ? 1
          : 0;


      const requestResult =
        document.getElementById(
          "requestResult"
        );


      if (requestResult) {

        requestResult.textContent =
          "Sending request to C backend...";
      }


      try {

        const response =
          await fetch(
            `${API_URL}/api/request?p=${process}&r0=${r0}&r1=${r1}&r2=${r2}&banker=${banker}`
          );


        if (!response.ok) {

          throw new Error(
            `Request endpoint returned HTTP ${response.status}`
          );
        }


        const data =
          await response.json();


        console.log(
          "Resource request result:",
          data
        );


        if (requestResult) {

          requestResult.textContent =
            data.message ||
            "Request completed.";
        }


        if (
          data.result ===
          "granted"
        ) {

          updateStatus(
            "safe",
            "REQUEST GRANTED",
            `P${process} received [${r0}, ${r1}, ${r2}].`
          );


        } else if (
          data.result ===
          "waiting"
        ) {

          updateStatus(
            "unsafe",
            "PROCESS WAITING",
            data.message ||
              "The process must wait for resources."
          );


        } else if (
          data.result ===
          "unsafe"
        ) {

          updateStatus(
            "unsafe",
            "UNSAFE REQUEST",
            data.message ||
              "The request would make the system unsafe."
          );


        } else {

          updateStatus(
            "neutral",
            "INVALID REQUEST",
            data.message ||
              "Request could not be completed."
          );
        }


        // Reload matrices, available vector,
        // utilization and graph.

        await loadSystemState();


      } catch (error) {

        console.error(
          "Resource request error:",
          error
        );


        if (requestResult) {

          requestResult.textContent =
            "Request failed: " +
            error.message;
        }


        updateStatus(
          "neutral",
          "REQUEST ERROR",
          "Resource request failed: " +
            error.message
        );
      }
    }
  );
}


// ==========================================
// TERMINATE PROCESS - DEADLOCK RECOVERY
// ==========================================

const terminateButton =
  document.getElementById(
    "terminateProcess"
  );

if (terminateButton) {

  terminateButton.addEventListener(
    "click",
    async () => {

      try {

        updateStatus(
          "neutral",
          "RECOVERY RUNNING",
          "Checking for deadlocked processes..."
        );


        const response =
          await fetch(
            `${API_URL}/api/recovery/terminate`
          );


        if (!response.ok) {

          throw new Error(
            `Recovery endpoint returned HTTP ${response.status}`
          );
        }


        const result =
          await response.json();


        console.log(
          "Recovery result:",
          result
        );


        if (deadlockResult) {

          deadlockResult.textContent =
            result.message ||
            "Recovery completed.";
        }


        // Reload state after terminating
        // the selected victim.

        const state =
          await loadSystemState();


        // Run detection again to prove
        // whether the deadlock was removed.

        await detectDeadlock();


        requestAnimationFrame(() => {

          drawResourceAllocationGraph(
            state
          );
        });


      } catch (error) {

        console.error(
          "Recovery error:",
          error
        );


        updateStatus(
          "neutral",
          "RECOVERY ERROR",
          "Recovery error: " +
            error.message
        );
      }
    }
  );
}

// ==========================================
// RESOURCE PREEMPTION RECOVERY
// ==========================================

const preemptButton =
  document.getElementById(
    "preemptResources"
  );


if (preemptButton) {

  preemptButton.addEventListener(
    "click",
    async () => {

      try {

        updateStatus(
          "neutral",
          "PREEMPTION RUNNING",
          "Attempting to recover from deadlock using resource preemption..."
        );


        const response =
          await fetch(
            `${API_URL}/api/recovery/preempt`
          );


        if (!response.ok) {

          throw new Error(
            `Preemption endpoint returned HTTP ${response.status}`
          );
        }


        const result =
          await response.json();


        console.log(
          "Preemption result:",
          result
        );


        if (deadlockResult) {

          deadlockResult.textContent =
            result.message ||
            "Preemption completed.";
        }


        // Reload matrices and RAG

        const state =
          await loadSystemState();


        // Check whether deadlock remains

        await detectDeadlock();


        requestAnimationFrame(() => {

          drawResourceAllocationGraph(
            state
          );
        });


      } catch (error) {

        console.error(
          "Preemption error:",
          error
        );


        updateStatus(
          "neutral",
          "PREEMPTION ERROR",
          "Preemption failed: " +
            error.message
        );
      }
    }
  );
}
// ==========================================
// END OF PART 2
// ==========================================
// ==========================================
// PART 3
// RESOURCE ALLOCATION GRAPH - RAG
// ==========================================


// ==========================================
// DRAW RESOURCE ALLOCATION GRAPH
// ==========================================

function drawResourceAllocationGraph(state) {

  const svg =
    document.getElementById(
      "ragLines"
    );

  const canvas =
    document.getElementById(
      "ragCanvas"
    );


  if (!svg || !canvas || !state) {
    return;
  }


  // ========================================
  // CLEAR OLD ARROWS
  // ========================================

  svg.innerHTML = "";


  // ========================================
  // GRAPH SIZE
  // ========================================

  const canvasRect =
    canvas.getBoundingClientRect();


  svg.setAttribute(
    "width",
    canvasRect.width
  );

  svg.setAttribute(
    "height",
    canvasRect.height
  );

  svg.setAttribute(
    "viewBox",
    `0 0 ${canvasRect.width} ${canvasRect.height}`
  );


  // ========================================
  // CREATE ARROW DEFINITIONS
  // ========================================

  const defs =
    document.createElementNS(
      "http://www.w3.org/2000/svg",
      "defs"
    );


  // BLUE arrow
  // Resource -> Process

  const allocationMarker =
    createArrowMarker(
      "allocationArrow",
      "#3b82f6"
    );


  // RED arrow
  // Process -> Resource

  const requestMarker =
    createArrowMarker(
      "requestArrow",
      "#ef4444"
    );


  defs.appendChild(
    allocationMarker
  );

  defs.appendChild(
    requestMarker
  );

  svg.appendChild(defs);


  const processes =
    Number(state.processes || 0);

  const resources =
    Number(state.resources || 0);


  // ========================================
  // SHOW / HIDE PROCESS NODES
  // ========================================

  for (let p = 0; p < 10; p++) {

    const processNode =
      document.getElementById(
        `graphP${p}`
      );

    if (processNode) {

      processNode.style.display =
        p < processes
          ? ""
          : "none";
    }
  }


  // ========================================
  // SHOW / HIDE RESOURCE NODES
  // ========================================

  for (let r = 0; r < 10; r++) {

    const resourceNode =
      document.getElementById(
        `graphR${r}`
      );

    if (resourceNode) {

      resourceNode.style.display =
        r < resources
          ? ""
          : "none";
    }
  }


  // ========================================
  // ALLOCATION EDGES
  //
  // RESOURCE ------> PROCESS
  // BLUE SOLID
  // ========================================

  for (
    let p = 0;
    p < processes;
    p++
  ) {

    for (
      let r = 0;
      r < resources;
      r++
    ) {

      const count =
        Number(
          state.allocation?.[p]?.[r] ||
          0
        );


      if (count > 0) {

        drawGraphEdge(
          svg,
          canvas,

          // FROM RESOURCE
          `graphR${r}`,

          // TO PROCESS
          `graphP${p}`,

          "#3b82f6",
          "allocationArrow",
          false,

          // Curve direction
          -1
        );
      }
    }
  }


  // ========================================
  // REQUEST EDGES
  //
  // PROCESS - - - -> RESOURCE
  // RED DASHED
  // ========================================

  for (
    let p = 0;
    p < processes;
    p++
  ) {

    for (
      let r = 0;
      r < resources;
      r++
    ) {

      const count =
        Number(
          state.request?.[p]?.[r] ||
          0
        );


      if (count > 0) {

        drawGraphEdge(
          svg,
          canvas,

          // FROM PROCESS
          `graphP${p}`,

          // TO RESOURCE
          `graphR${r}`,

          "#ef4444",
          "requestArrow",
          true,

          // Opposite curve direction
          1
        );
      }
    }
  }
}


// ==========================================
// CREATE SVG ARROW MARKER
// ==========================================

function createArrowMarker(
  id,
  color
) {

  const marker =
    document.createElementNS(
      "http://www.w3.org/2000/svg",
      "marker"
    );


  marker.setAttribute(
    "id",
    id
  );


  marker.setAttribute(
    "markerWidth",
    "10"
  );


  marker.setAttribute(
    "markerHeight",
    "10"
  );


  // Arrow head position

  marker.setAttribute(
    "refX",
    "8"
  );


  marker.setAttribute(
    "refY",
    "3"
  );


  marker.setAttribute(
    "orient",
    "auto"
  );


  marker.setAttribute(
    "markerUnits",
    "strokeWidth"
  );


  const path =
    document.createElementNS(
      "http://www.w3.org/2000/svg",
      "path"
    );


  path.setAttribute(
    "d",
    "M0,0 L0,6 L9,3 z"
  );


  path.setAttribute(
    "fill",
    color
  );


  marker.appendChild(path);

  return marker;
}


// ==========================================
// DRAW ONE RAG EDGE
// ==========================================

function drawGraphEdge(
  svg,
  canvas,
  fromId,
  toId,
  color,
  markerId,
  dashed,
  curveDirection = 0
) {

  const from =
    document.getElementById(
      fromId
    );

  const to =
    document.getElementById(
      toId
    );


  if (!from || !to) {
    return;
  }


  const canvasRect =
    canvas.getBoundingClientRect();

  const fromRect =
    from.getBoundingClientRect();

  const toRect =
    to.getBoundingClientRect();


  // Don't draw hidden nodes

  if (
    fromRect.width === 0 ||
    fromRect.height === 0 ||
    toRect.width === 0 ||
    toRect.height === 0
  ) {

    return;
  }


  // ========================================
  // NODE CENTRES
  // ========================================

  const fromCX =
    fromRect.left -
    canvasRect.left +
    fromRect.width / 2;


  const fromCY =
    fromRect.top -
    canvasRect.top +
    fromRect.height / 2;


  const toCX =
    toRect.left -
    canvasRect.left +
    toRect.width / 2;


  const toCY =
    toRect.top -
    canvasRect.top +
    toRect.height / 2;


  // ========================================
  // DIRECTION VECTOR
  // ========================================

  const dx =
    toCX - fromCX;

  const dy =
    toCY - fromCY;


  const distance =
    Math.sqrt(
      dx * dx +
      dy * dy
    );


  if (distance === 0) {
    return;
  }


  const ux =
    dx / distance;

  const uy =
    dy / distance;


  // ========================================
  // FIND NODE TYPES
  // ========================================

  const fromIsProcess =
    fromId.includes(
      "graphP"
    );

  const toIsProcess =
    toId.includes(
      "graphP"
    );


  // ========================================
  // START PADDING
  // ========================================

  let fromPadding;


  if (fromIsProcess) {

    // Process is circular

    fromPadding =
      Math.min(
        fromRect.width,
        fromRect.height
      ) / 2 + 3;

  } else {

    // Resource is rectangular

    fromPadding =
      getRectangleEdgeDistance(
        fromRect.width,
        fromRect.height,
        ux,
        uy
      ) + 3;
  }


  // ========================================
  // END PADDING
  // ========================================

  let toPadding;


  if (toIsProcess) {

    // Process circle

    toPadding =
      Math.min(
        toRect.width,
        toRect.height
      ) / 2 + 8;

  } else {

    // Resource rectangle

    toPadding =
      getRectangleEdgeDistance(
        toRect.width,
        toRect.height,
        ux,
        uy
      ) + 8;
  }


  // ========================================
  // FINAL START POINT
  // ========================================

  const x1 =
    fromCX +
    ux * fromPadding;


  const y1 =
    fromCY +
    uy * fromPadding;


  // ========================================
  // FINAL END POINT
  // ========================================

  const x2 =
    toCX -
    ux * toPadding;


  const y2 =
    toCY -
    uy * toPadding;


  // ========================================
  // CURVED EDGE
  //
  // Makes request/allocation arrows easier
  // to see when they cross.
  // ========================================

  const middleX =
    (x1 + x2) / 2;

  const middleY =
    (y1 + y2) / 2;


  // Perpendicular direction

  const perpendicularX =
    -uy;

  const perpendicularY =
    ux;


  // Small curve so the graph still
  // looks clean and professional.

  const curveAmount =
    curveDirection * 18;


  const controlX =
    middleX +
    perpendicularX *
      curveAmount;


  const controlY =
    middleY +
    perpendicularY *
      curveAmount;


  // ========================================
  // CREATE SVG PATH
  // ========================================

  const path =
    document.createElementNS(
      "http://www.w3.org/2000/svg",
      "path"
    );


  path.setAttribute(
    "d",
    `M ${x1} ${y1}
     Q ${controlX} ${controlY}
     ${x2} ${y2}`
  );


  path.setAttribute(
    "fill",
    "none"
  );


  path.setAttribute(
    "stroke",
    color
  );


  path.setAttribute(
    "stroke-width",
    "2.4"
  );


  path.setAttribute(
    "stroke-linecap",
    "round"
  );


  path.setAttribute(
    "stroke-linejoin",
    "round"
  );


  path.setAttribute(
    "marker-end",
    `url(#${markerId})`
  );


  // ========================================
  // RED REQUEST LINE = DASHED
  // ========================================

  if (dashed) {

    path.setAttribute(
      "stroke-dasharray",
      "7 6"
    );
  }


  svg.appendChild(path);
}


// ==========================================
// RECTANGLE EDGE CALCULATION
//
// Finds approximately where the arrow
// should meet a rectangular resource node.
// ==========================================

function getRectangleEdgeDistance(
  width,
  height,
  ux,
  uy
) {

  const halfWidth =
    width / 2;

  const halfHeight =
    height / 2;


  const absX =
    Math.abs(ux);

  const absY =
    Math.abs(uy);


  let xDistance =
    Infinity;

  let yDistance =
    Infinity;


  if (absX > 0.0001) {

    xDistance =
      halfWidth / absX;
  }


  if (absY > 0.0001) {

    yDistance =
      halfHeight / absY;
  }


  return Math.min(
    xDistance,
    yDistance
  );
}


// ==========================================
// REFRESH RESOURCE ALLOCATION GRAPH
// ==========================================

const refreshGraphButton =
  document.getElementById(
    "refreshGraph"
  );


if (refreshGraphButton) {

  refreshGraphButton.addEventListener(
    "click",
    async () => {

      const originalText =
        refreshGraphButton.textContent;


      try {

        // Disable button while loading

        refreshGraphButton.disabled =
          true;


        refreshGraphButton.textContent =
          "Refreshing...";


        // Get latest system state
        // directly from C backend

        const state =
          await loadSystemState();


        if (!state) {

          throw new Error(
            "Could not load system state."
          );
        }


        // Wait for browser layout before
        // recalculating arrow positions.

        requestAnimationFrame(() => {

          drawResourceAllocationGraph(
            state
          );
        });


        refreshGraphButton.textContent =
          "Refreshed ✓";


        if (statusMessage) {

          statusMessage.textContent =
            "Resource Allocation Graph refreshed.";
        }


        setTimeout(() => {

          refreshGraphButton.textContent =
            originalText;

        }, 1000);


      } catch (error) {

        console.error(
          "Graph refresh error:",
          error
        );


        refreshGraphButton.textContent =
          "Refresh Failed";


        setTimeout(() => {

          refreshGraphButton.textContent =
            originalText;

        }, 1200);


      } finally {

        // IMPORTANT:
        // Enable Refresh Graph button again.

        refreshGraphButton.disabled =
          false;
      }
    }
  );
}


// ==========================================
// REDRAW GRAPH WHEN WINDOW RESIZES
// ==========================================

let graphResizeTimer = null;


window.addEventListener(
  "resize",
  () => {

    clearTimeout(
      graphResizeTimer
    );


    graphResizeTimer =
      setTimeout(
        () => {

          try {

            if (lastState) {

              drawResourceAllocationGraph(
                lastState
              );
            }

          } catch (error) {

            console.error(
              "Graph resize redraw error:",
              error
            );
          }

        },
        150
      );
  }
);


// ==========================================
// END OF PART 3
// ==========================================
// ==========================================
// PART 4
// MAIN BUTTONS, NEED MATRIX & INITIAL LOAD
// ==========================================


// ==========================================
// RUN BANKER BUTTON
// ==========================================

const runBankerButton =
  document.getElementById(
    "runBanker"
  );


if (runBankerButton) {

  runBankerButton.addEventListener(
    "click",
    async () => {

      try {

        await runBankerAlgorithm();

      } catch (error) {

        console.error(
          "Banker button error:",
          error
        );
      }
    }
  );
}


// ==========================================
// DETECT DEADLOCK BUTTON
// ==========================================

const detectDeadlockButton =
  document.getElementById(
    "detectDeadlock"
  );


if (detectDeadlockButton) {

  detectDeadlockButton.addEventListener(
    "click",
    async () => {

      try {

        await detectDeadlock();

      } catch (error) {

        console.error(
          "Deadlock button error:",
          error
        );
      }
    }
  );
}


// ==========================================
// MANUAL NEED MATRIX CALCULATION
//
// NEED = MAXIMUM - ALLOCATION
// ==========================================

function calculateNeedMatrix() {

  const rows =
    document.querySelectorAll(
      "#matrixBody tr"
    );


  rows.forEach(row => {

    const allocation =
      row.querySelectorAll(
        ".allocation"
      );


    const maximum =
      row.querySelectorAll(
        ".maximum"
      );


    const needs =
      row.querySelectorAll(
        ".need"
      );


    needs.forEach(
      (cell, index) => {

        const alloc =
          Number(
            allocation[
              index
            ]?.value
          ) || 0;


        const max =
          Number(
            maximum[
              index
            ]?.value
          ) || 0;


        cell.textContent =
          Math.max(
            0,
            max - alloc
          );
      }
    );
  });
}


// ==========================================
// UPDATE NEED MATRIX WHEN USER EDITS
// ALLOCATION OR MAXIMUM VALUES
// ==========================================

document
  .querySelectorAll(
    ".allocation, .maximum"
  )
  .forEach(input => {

    input.addEventListener(
      "input",
      calculateNeedMatrix
    );
  });


// ==========================================
// INITIALIZE SYSTEM
// ==========================================

async function initializeSystem() {

  try {

    // Get initial state from
    // the C backend

    const state =
      await loadSystemState();


    lastState =
      state;


    // Draw again after browser layout
    // has fully settled.

    requestAnimationFrame(() => {

      drawResourceAllocationGraph(
        state
      );
    });


    updateStatus(
      "neutral",
      "READY",
      "C backend connected. Select an example or run an algorithm."
    );


    console.log(
      "Deadlock Simulation System initialized."
    );


  } catch (error) {

    console.error(
      "Initial connection error:",
      error
    );


    if (backendStatus) {

      backendStatus.textContent =
        "● Backend Offline";

      backendStatus.className =
        "text-danger";
    }


    updateStatus(
      "neutral",
      "BACKEND OFFLINE",
      "Start ./web/deadlock_backend and refresh this page."
    );
  }
}


// ==========================================
// START APPLICATION
// ==========================================

initializeSystem();


// ==========================================
// END OF SCRIPT
// ==========================================