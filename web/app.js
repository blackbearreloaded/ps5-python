(() => {
  const state = { launching: false, selectedId: null,
    socket: null, socketConnected: false, fallbackTimer: null,
    view: "apps", replBusy: false, scriptBusy: false, scriptDirty: false,
    replCommandOpen: false,
    replHistory: [], replHistoryIndex: -1 };
  const appsElement = document.getElementById("apps");
  const statusElement = document.getElementById("status");
  const statusText = statusElement.querySelector(".status-text");
  const titleElement = document.getElementById("console-title");
  const terminal = document.getElementById("terminal");
  const exitCode = document.getElementById("exit-code");
  const refreshButton = document.getElementById("refresh-apps");
  const clearButton = document.getElementById("clear-output");
  const appsSidebar = document.getElementById("apps-sidebar");
  const appConsole = document.getElementById("app-console");
  const replConsole = document.getElementById("repl-console");
  const menuApps = document.getElementById("menu-apps");
  const menuRepl = document.getElementById("menu-repl");
  const menuScript = document.getElementById("menu-script");
  const replShell = document.querySelector(".repl-shell");
  const replTerminal = document.getElementById("repl-terminal");
  const replForm = document.getElementById("repl-form");
  const replInput = document.getElementById("repl-input");
  const replClear = document.getElementById("repl-clear");
  const replReset = document.getElementById("repl-reset");
  const replConnection = document.getElementById("repl-connection");
  const scriptConsole = document.getElementById("script-console");
  const scriptInput = document.getElementById("script-input");
  const scriptOutput = document.getElementById("script-output");
  const scriptRun = document.getElementById("script-run");
  const scriptClear = document.getElementById("script-clear");
  const scriptDirty = document.getElementById("script-dirty");
  const scriptStatus = document.getElementById("script-status");
  const scriptRuntime = document.getElementById("script-runtime");
  const scriptHighlight = document.querySelector("#script-highlight code");
  const replHighlight = document.querySelector("#repl-highlight code");
  const themeToggle = document.getElementById("theme-toggle");
  const themes = [
    { id: "studio", label: "Studio" },
    { id: "terminal", label: "Terminal" },
    { id: "paper", label: "Paper" }
  ];

  function applyTheme(themeId) {
    const theme = themes.find((item) => item.id === themeId) || themes[0];
    document.documentElement.dataset.theme = theme.id;
    themeToggle.textContent = "Theme: " + theme.label;
    themeToggle.dataset.theme = theme.id;
    window.localStorage.setItem("python-ps5-theme", theme.id);
  }

  function renderHighlight(source, target) {
    if (window.hljs) {
      target.innerHTML = window.hljs.highlight(source, { language: "python" }).value;
    } else {
      target.textContent = source;
    }
  }

  function syncHighlightScroll(input, target) {
    const layer = target.parentElement;
    layer.scrollTop = input.scrollTop;
    layer.scrollLeft = input.scrollLeft;
  }

  function updateViewUrl(view) {
    const url = new URL(window.location.href);
    const viewName = view === "repl" ? "interpreter" : view === "script" ? "script" : "applications";
    url.searchParams.set("view", viewName);
    window.history.replaceState(null, "", url.pathname +
      (url.searchParams.toString() ? "?" + url.searchParams.toString() : "") +
      url.hash);
  }

  function showView(view, updateUrl = true) {
    state.view = view;
    const repl = view === "repl";
    const script = view === "script";
    appsSidebar.classList.toggle("hidden", repl || script);
    appConsole.classList.toggle("hidden", repl || script);
    replConsole.classList.toggle("hidden", !repl);
    scriptConsole.classList.toggle("hidden", !script);
    replConsole.setAttribute("aria-hidden", repl ? "false" : "true");
    scriptConsole.setAttribute("aria-hidden", script ? "false" : "true");
    menuApps.classList.toggle("active", !repl);
    menuApps.classList.toggle("active", view === "apps");
    menuRepl.classList.toggle("active", repl);
    menuScript.classList.toggle("active", script);
    menuApps.setAttribute("aria-selected", view === "apps" ? "true" : "false");
    menuRepl.setAttribute("aria-selected", repl ? "true" : "false");
    menuScript.setAttribute("aria-selected", script ? "true" : "false");
    if (updateUrl) updateViewUrl(view);
    if (repl) replInput.focus();
    if (script) scriptInput.focus();
  }

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
  function appendRepl(text) {
    replTerminal.textContent += text;
    replShell.scrollTop = replShell.scrollHeight;
  }
  function resetReplPrompt() {
    replTerminal.textContent = "CPython 3.14.7 WebREPL\n" +
      "Connected to the running python-web.elf.\n" +
      "Use Enter to evaluate a line; Shift+Enter inserts a new line.\n";
    replShell.scrollTop = replShell.scrollHeight;
    state.replCommandOpen = false;
  }
  function resizeReplInput() {
    replInput.style.height = "auto";
    const height = Math.min(replInput.scrollHeight, 150) + "px";
    replInput.parentElement.style.height = height;
    replInput.style.height = height;
    renderHighlight(replInput.value, replHighlight);
    syncHighlightScroll(replInput, replHighlight);
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
    if (event.type === "repl") {
      if (state.scriptBusy) {
        const scriptData = event.data || "";
        scriptOutput.textContent = scriptData || "(script completed without output)";
        scriptStatus.textContent = event.ok ? "Completed" : "Failed";
        scriptStatus.className = event.ok ? "repl-hint repl-connected" : "repl-hint repl-error";
        scriptRuntime.textContent = event.ok ? "Script finished" : "Script raised an exception";
        state.scriptBusy = false;
        scriptRun.disabled = false;
        return;
      }
      replConnection.textContent = event.ok ? "Evaluation complete" : "Evaluation failed";
      replConnection.className = event.ok ? "repl-hint" : "repl-hint repl-error";
      const data = event.data || "";
      if (state.replCommandOpen) {
        appendRepl("\n");
        state.replCommandOpen = false;
      }
      if (data) appendRepl(data.endsWith("\n") ? data : data + "\n");
      state.replBusy = false;
      replInput.disabled = false;
      replInput.focus();
    }
    if (event.type === "repl_reset") {
      resetReplPrompt();
      replInput.value = "";
      resizeReplInput();
      state.replHistory = [];
      state.replHistoryIndex = -1;
      state.replBusy = false;
      replInput.disabled = false;
      replConnection.textContent = "Interpreter restarted";
      replConnection.className = "repl-hint repl-connected";
      replInput.focus();
    }
    if (event.type === "clear") {
      setOutput("");
      exitCode.textContent = "Output cleared";
    }
    if (event.type === "status") applyStatus(event);
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
        setStatus("Live link", "ready");
        replConnection.textContent = "WebREPL connected";
        replConnection.className = "repl-hint repl-connected";
        scriptRuntime.textContent = "WebREPL connected";
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
        setStatus("Live link unavailable", "error");
        replConnection.textContent = "WebREPL requires the live WebSocket";
        replConnection.className = "repl-hint repl-error";
        scriptRuntime.textContent = "WebSocket unavailable";
      };
    } catch (error) {
      setStatus("Live link unavailable", "error");
      replConnection.textContent = "WebREPL requires the live WebSocket";
      replConnection.className = "repl-hint repl-error";
      scriptRuntime.textContent = "WebSocket unavailable";
    }
  }
  refreshButton.addEventListener("click", refreshApps);
  clearButton.addEventListener("click", async () => {
    setOutput("");
    exitCode.textContent = "Output cleared";
    try {
      const response = await fetch("/api/logs/clear");
      if (!response.ok) throw new Error("HTTP " + response.status);
    } catch (error) {
      exitCode.textContent = "Clear failed: " + error;
    }
  });
  replForm.addEventListener("submit", (event) => {
    event.preventDefault();
    const source = replInput.value;
    if (state.replBusy) return;
    if (!state.socketConnected || state.socket === null) {
      replConnection.textContent = "WebSocket unavailable";
      replConnection.className = "repl-hint repl-error";
      return;
    }
    if (source.trim()) {
      state.replHistory = state.replHistory.filter((item) => item !== source);
      state.replHistory.push(source);
      state.replHistoryIndex = -1;
    }
    appendRepl(">>> " + source);
    state.replCommandOpen = true;
    state.socket.send(source.endsWith("\n") ? source : source + "\n");
    replInput.value = "";
    resizeReplInput();
    state.replBusy = true;
    replInput.disabled = true;
    replConnection.textContent = "Evaluating…";
    replConnection.className = "repl-hint";
  });
  replInput.addEventListener("keydown", (event) => {
    if (event.ctrlKey && event.key.toLowerCase() === "l") {
      event.preventDefault();
      resetReplPrompt();
      replInput.value = "";
      resizeReplInput();
      state.replHistoryIndex = -1;
      return;
    }
    if (event.key === "Enter" && !event.shiftKey) {
      event.preventDefault();
      replForm.requestSubmit();
      return;
    }
    if (event.key === "ArrowUp" && !event.shiftKey && state.replHistory.length) {
      event.preventDefault();
      if (state.replHistoryIndex < state.replHistory.length - 1)
        state.replHistoryIndex += 1;
      replInput.value = state.replHistory[state.replHistory.length - 1 - state.replHistoryIndex];
      resizeReplInput();
      replInput.setSelectionRange(replInput.value.length, replInput.value.length);
    } else if (event.key === "ArrowDown" && !event.shiftKey && state.replHistoryIndex >= 0) {
      event.preventDefault();
      state.replHistoryIndex -= 1;
      replInput.value = state.replHistoryIndex < 0 ? "" :
        state.replHistory[state.replHistory.length - 1 - state.replHistoryIndex];
      resizeReplInput();
      replInput.setSelectionRange(replInput.value.length, replInput.value.length);
    }
  });
  replInput.addEventListener("input", resizeReplInput);
  function runScript() {
    if (state.scriptBusy) return;
    if (!state.socketConnected || state.socket === null) {
      scriptStatus.textContent = "WebSocket unavailable";
      scriptStatus.className = "repl-hint repl-error";
      return;
    }
    const source = scriptInput.value;
    if (!source.trim()) {
      scriptStatus.textContent = "Nothing to run";
      scriptStatus.className = "repl-hint repl-error";
      return;
    }
    state.scriptBusy = true;
    scriptRun.disabled = true;
    scriptStatus.textContent = "Running…";
    scriptStatus.className = "repl-hint";
    scriptRuntime.textContent = "Evaluating complete script…";
    scriptOutput.textContent = "";
    state.scriptDirty = false;
    scriptDirty.textContent = "Saved";
    state.socket.send(source.endsWith("\n") ? source : source + "\n");
  }
  scriptRun.addEventListener("click", runScript);
  scriptInput.addEventListener("input", () => {
    state.scriptDirty = true;
    scriptDirty.textContent = "Unsaved edits";
    renderHighlight(scriptInput.value, scriptHighlight);
    syncHighlightScroll(scriptInput, scriptHighlight);
  });
  scriptInput.addEventListener("scroll", () => syncHighlightScroll(scriptInput, scriptHighlight));
  replInput.addEventListener("scroll", () => syncHighlightScroll(replInput, replHighlight));
  scriptInput.addEventListener("keydown", (event) => {
    if (event.ctrlKey && event.key === "Enter") {
      event.preventDefault();
      runScript();
    }
  });
  scriptClear.addEventListener("click", () => {
    scriptInput.value = "";
    renderHighlight("", scriptHighlight);
    scriptOutput.textContent = "Paste a complete Python script above, then run it here.";
    state.scriptDirty = false;
    scriptDirty.textContent = "Saved";
    scriptStatus.textContent = "Ready";
    scriptStatus.className = "repl-hint";
  });
  themeToggle.addEventListener("click", () => {
    const current = themes.findIndex((theme) => theme.id === document.documentElement.dataset.theme);
    applyTheme(themes[(current + 1) % themes.length].id);
  });
  replClear.addEventListener("click", () => {
    resetReplPrompt();
    replInput.value = "";
    resizeReplInput();
    state.replHistoryIndex = -1;
  });
  replReset.addEventListener("click", async () => {
    if (state.replBusy) return;
    replConnection.textContent = "Restarting interpreter…";
    replConnection.className = "repl-hint";
    replReset.disabled = true;
    try {
      const response = await fetch("/api/repl/reset");
      if (!response.ok) throw new Error(await response.text());
      resetReplPrompt();
      replInput.value = "";
      resizeReplInput();
      state.replHistory = [];
      state.replHistoryIndex = -1;
    } catch (error) {
      replConnection.textContent = "Restart failed: " + error;
      replConnection.className = "repl-hint repl-error";
    } finally {
      replReset.disabled = false;
    }
  });
  menuApps.addEventListener("click", () => showView("apps"));
  menuRepl.addEventListener("click", () => showView("repl"));
  menuScript.addEventListener("click", () => showView("script"));
  const requestedView = new URLSearchParams(window.location.search).get("view");
  showView(requestedView === "applications" ? "apps" : requestedView === "script" ? "script" : "repl", false);
  applyTheme(window.localStorage.getItem("python-ps5-theme") || "studio");
  renderHighlight(scriptInput.value, scriptHighlight);
  resizeReplInput();
  refreshApps();
  connectSocket();
})();
