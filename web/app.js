(() => {
  const state = { cursor: 0, launching: false, selectedId: null,
    socket: null, socketConnected: false, pollTimer: null, fallbackTimer: null };
  const appsElement = document.getElementById("apps");
  const statusElement = document.getElementById("status");
  const statusText = statusElement.querySelector(".status-text");
  const titleElement = document.getElementById("console-title");
  const terminal = document.getElementById("terminal");
  const exitCode = document.getElementById("exit-code");
  const refreshButton = document.getElementById("refresh-apps");
  const clearButton = document.getElementById("clear-output");

  function setStatus(label, kind) {
    statusText.textContent = label;
    statusElement.className = "status-pill status-" + kind;
  }
  function setOutput(text) {
    terminal.textContent = text;
    terminal.scrollTop = terminal.scrollHeight;
  }
  function appendOutput(text) {
    if (terminal.querySelector(".terminal-placeholder")) terminal.textContent = "";
    terminal.textContent += text;
    terminal.scrollTop = terminal.scrollHeight;
  }
  function setButtonsDisabled(disabled) {
    appsElement.querySelectorAll("button").forEach((button) => { button.disabled = disabled; });
  }
  function applyStatus(status) {
    if (status.running) {
      setStatus("Running", "running");
      exitCode.textContent = "Streaming output";
    } else if (status.finished) {
      setStatus("Finished · exit " + status.exit_code, "finished");
      exitCode.textContent = "Exit code " + status.exit_code;
      state.launching = false;
      setButtonsDisabled(false);
    } else if (!state.launching) {
      setStatus(state.socketConnected ? "Live link" : "Ready", "ready");
      exitCode.textContent = "Waiting for a run";
    }
  }
  function renderApps(apps) {
    appsElement.textContent = "";
    if (!apps.length) {
      const empty = document.createElement("div");
      empty.className = "empty-card";
      empty.textContent = "No Python apps found.";
      appsElement.appendChild(empty);
      return;
    }
    apps.forEach((app) => {
      const button = document.createElement("button");
      button.className = "app-card";
      button.type = "button";
      button.dataset.appId = app.id;
      const title = document.createElement("span");
      title.className = "app-card-title";
      title.textContent = app.name;
      const id = document.createElement("span");
      id.className = "app-card-id";
      id.textContent = app.id;
      button.append(title, id);
      button.addEventListener("click", () => launch(app));
      appsElement.appendChild(button);
    });
  }
  async function refreshApps() {
    try {
      const response = await fetch("/api/apps");
      if (!response.ok) throw new Error("HTTP " + response.status);
      renderApps(await response.json());
      if (!state.launching && !state.socketConnected) setStatus("Ready", "ready");
    } catch (error) {
      setStatus("Connection lost", "error");
      appsElement.textContent = "";
      const empty = document.createElement("div");
      empty.className = "empty-card";
      empty.textContent = "Unable to load applications: " + error;
      appsElement.appendChild(empty);
    }
  }
  async function launch(app) {
    if (state.launching) return;
    state.launching = true;
    state.selectedId = app.id;
    state.cursor = 0;
    titleElement.textContent = app.name;
    exitCode.textContent = "Running";
    setOutput("[launcher] starting " + app.id + "\n");
    setStatus("Starting", "running");
    setButtonsDisabled(true);
    try {
      const response = await fetch("/api/launch?app=" + encodeURIComponent(app.id));
      if (!response.ok) {
        appendOutput("[launcher] " + await response.text() + "\n");
        state.launching = false;
        setButtonsDisabled(false);
        setStatus("Launch failed", "error");
      }
    } catch (error) {
      appendOutput("[launcher] request failed: " + error + "\n");
      state.launching = false;
      setButtonsDisabled(false);
      setStatus("Launch failed", "error");
    }
  }
  function handleSocketMessage(message) {
    let event;
    try {
      event = JSON.parse(message);
    } catch (error) {
      return;
    }
    if (event.type === "log") appendOutput(event.data || "");
    if (event.type === "clear") {
      state.cursor = 0;
      setOutput("");
      exitCode.textContent = "Output cleared";
    }
    if (event.type === "status") applyStatus(event);
  }
  function startPolling() {
    if (state.pollTimer === null) poll();
  }
  function connectSocket() {
    const protocol = location.protocol === "https:" ? "wss:" : "ws:";
    try {
      const socket = new WebSocket(protocol + "//" + location.host + "/ws");
      state.socket = socket;
      state.fallbackTimer = window.setTimeout(() => {
        if (!state.socketConnected && state.socket === socket) {
          socket.close();
        }
      }, 3000);
      socket.onopen = () => {
        state.socketConnected = true;
        if (state.fallbackTimer !== null) {
          window.clearTimeout(state.fallbackTimer);
          state.fallbackTimer = null;
        }
        if (state.pollTimer !== null) {
          window.clearTimeout(state.pollTimer);
          state.pollTimer = null;
        }
        setStatus("Live link", "ready");
      };
      socket.onmessage = (event) => handleSocketMessage(event.data);
      socket.onerror = () => socket.close();
      socket.onclose = () => {
        state.socketConnected = false;
        state.socket = null;
        if (state.fallbackTimer !== null) {
          window.clearTimeout(state.fallbackTimer);
          state.fallbackTimer = null;
        }
        setStatus("Using polling fallback", "error");
        // One failed WebSocket attempt falls back to polling for this page.
        // A reload can try WebSockets again without creating a reconnect storm.
        startPolling();
      };
    } catch (error) {
      startPolling();
    }
  }
  async function poll() {
    state.pollTimer = null;
    if (state.socketConnected) return;
    try {
      const logsResponse = await fetch("/api/logs?since=" + state.cursor);
      const next = logsResponse.headers.get("X-Log-Next");
      if (next) state.cursor = Number(next);
      const logs = await logsResponse.text();
      if (logs) appendOutput(logs);
      applyStatus(await (await fetch("/api/status")).json());
    } catch (error) {
      setStatus("Connection lost", "error");
      exitCode.textContent = "Retrying…";
    }
    if (!state.socketConnected) state.pollTimer = window.setTimeout(poll, 1000);
  }
  refreshButton.addEventListener("click", refreshApps);
  clearButton.addEventListener("click", async () => {
    state.cursor = 0;
    setOutput("");
    exitCode.textContent = "Output cleared";
    try {
      const response = await fetch("/api/logs/clear");
      if (!response.ok) throw new Error("HTTP " + response.status);
    } catch (error) {
      exitCode.textContent = "Clear failed: " + error;
    }
  });
  refreshApps();
  connectSocket();
})();
