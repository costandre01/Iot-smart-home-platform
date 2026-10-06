const STORAGE_KEY = "iot-smart-home-demo-v1";
const HISTORY_LIMIT = 48;

const initialState = () => ({
  sensors: {
    temperatura: 22.8,
    humidade: 48.0,
    dioxidoCarbono: 520,
    amonio: 0.82,
    benzeno: 0.04,
    oxidoNitrogenio: 0.07,
    alcool: 0.03,
    fumo: 0.05,
    movimento: 86
  },
  actuators: {
    buzzer: false,
    luz: false,
    porta: false,
    fan: false
  },
  history: [],
  cameraTimestamp: null
});

function loadState() {
  try {
    const stored = JSON.parse(localStorage.getItem(STORAGE_KEY));
    return stored && stored.sensors && stored.actuators ? stored : initialState();
  } catch {
    return initialState();
  }
}

let state = loadState();
let toastTimer;

const clamp = (value, min, max) => Math.min(max, Math.max(min, value));
const jitter = (value, amount, min, max, decimals = 2) =>
  Number(clamp(value + (Math.random() - 0.5) * amount, min, max).toFixed(decimals));

function saveState() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
}

function nowTime() {
  return new Intl.DateTimeFormat("en-GB", {
    dateStyle: "medium",
    timeStyle: "medium"
  }).format(new Date());
}

function toast(message) {
  const el = document.getElementById("toast");
  el.textContent = message;
  el.classList.add("show");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.remove("show"), 2200);
}

function recordHistory() {
  state.history.unshift({
    time: new Date().toISOString(),
    temperatura: state.sensors.temperatura,
    humidade: state.sensors.humidade,
    dioxidoCarbono: state.sensors.dioxidoCarbono,
    amonio: state.sensors.amonio,
    benzeno: state.sensors.benzeno,
    oxidoNitrogenio: state.sensors.oxidoNitrogenio,
    alcool: state.sensors.alcool,
    fumo: state.sensors.fumo,
    movimento: state.sensors.movimento,
    luz: state.actuators.luz,
    porta: state.actuators.porta
  });
  state.history = state.history.slice(0, HISTORY_LIMIT);
}

function simulateTelemetry() {
  state.sensors.temperatura = jitter(state.sensors.temperatura, 0.42, 18, 30, 1);
  state.sensors.humidade = jitter(state.sensors.humidade, 1.8, 32, 72, 1);
  state.sensors.dioxidoCarbono = Math.round(jitter(state.sensors.dioxidoCarbono, 26, 390, 950, 0));
  state.sensors.amonio = jitter(state.sensors.amonio, 0.08, 0.1, 2.4, 2);
  state.sensors.benzeno = jitter(state.sensors.benzeno, 0.012, 0.01, 0.22, 3);
  state.sensors.oxidoNitrogenio = jitter(state.sensors.oxidoNitrogenio, 0.018, 0.01, 0.35, 3);
  state.sensors.alcool = jitter(state.sensors.alcool, 0.012, 0.01, 0.3, 3);
  state.sensors.fumo = jitter(state.sensors.fumo, 0.02, 0.01, 0.45, 3);
  state.sensors.movimento = Math.round(jitter(state.sensors.movimento, 18, 18, 180, 0));

  // Mimics the original project's automatic reactions without taking control away from the visitor.
  if (state.sensors.temperatura > 26 || state.sensors.humidade > 65) {
    state.actuators.fan = true;
  }

  recordHistory();
  saveState();
  render();
}

function formatSensor(name, value) {
  if (["benzeno", "oxidoNitrogenio", "alcool", "fumo"].includes(name)) return Number(value).toFixed(3);
  if (name === "amonio") return Number(value).toFixed(2);
  if (["temperatura", "humidade"].includes(name)) return Number(value).toFixed(1);
  return String(value);
}

function render() {
  document.querySelectorAll("[data-sensor]").forEach((el) => {
    const name = el.dataset.sensor;
    el.textContent = formatSensor(name, state.sensors[name]);
  });

  document.getElementById("buzzerSwitch").checked = state.actuators.buzzer;
  document.getElementById("luzSwitch").checked = state.actuators.luz;
  document.getElementById("portaSwitch").checked = state.actuators.porta;
  document.getElementById("fanSwitch").checked = state.actuators.fan;
  document.getElementById("lightState").textContent = state.actuators.luz ? "ON" : "OFF";
  document.getElementById("doorState").textContent = state.actuators.porta ? "OPEN" : "CLOSED";
  document.getElementById("doorVisual").classList.toggle("open", state.actuators.porta);
  document.getElementById("lastUpdate").textContent = `Last virtual device update: ${nowTime()}`;
  renderHistory();
}

function bindSwitch(id, key, onLabel, offLabel) {
  document.getElementById(id).addEventListener("change", (event) => {
    state.actuators[key] = event.target.checked;
    recordHistory();
    saveState();
    render();
    toast(`${onLabel}: ${event.target.checked ? "ON" : offLabel}`);
  });
}

function setView(name) {
  document.querySelectorAll(".view").forEach((view) => view.classList.remove("active"));
  document.querySelectorAll(".nav-link[data-view]").forEach((link) => link.classList.toggle("active", link.dataset.view === name));
  document.getElementById(`${name}View`).classList.add("active");
  if (name === "history") renderHistory();
  window.scrollTo({ top: 0, behavior: "smooth" });
}

function renderHistory() {
  const body = document.getElementById("historyBody");
  const filter = document.getElementById("historyFilter")?.value || "all";
  const entries = state.history;

  if (!entries.length) {
    body.innerHTML = '<tr class="empty-row"><td colspan="7">The simulator has not generated history yet.</td></tr>';
    return;
  }

  body.innerHTML = entries.map((entry) => {
    const visibility = {
      climate: [entry.temperatura, entry.humidade, "—", "—", "—", "—"],
      air: ["—", "—", entry.dioxidoCarbono, "—", "—", "—"],
      movement: ["—", "—", "—", entry.movimento, entry.luz ? "ON" : "OFF", "—"],
      door: ["—", "—", "—", "—", "—", entry.porta ? "OPEN" : "CLOSED"],
      all: [entry.temperatura, entry.humidade, entry.dioxidoCarbono, entry.movimento, entry.luz ? "ON" : "OFF", entry.porta ? "OPEN" : "CLOSED"]
    }[filter];

    const time = new Intl.DateTimeFormat("en-GB", { hour: "2-digit", minute: "2-digit", second: "2-digit" }).format(new Date(entry.time));
    return `<tr>
      <td>${time}</td>
      <td>${visibility[0] === "—" ? "—" : `${Number(visibility[0]).toFixed(1)} °C`}</td>
      <td>${visibility[1] === "—" ? "—" : `${Number(visibility[1]).toFixed(1)} %`}</td>
      <td>${visibility[2] === "—" ? "—" : `${visibility[2]} ppm`}</td>
      <td>${visibility[3] === "—" ? "—" : `${visibility[3]} cm`}</td>
      <td>${visibility[4]}</td>
      <td>${visibility[5]}</td>
    </tr>`;
  }).join("");
}

function captureCamera() {
  const timestamp = nowTime();
  state.cameraTimestamp = new Date().toISOString();
  saveState();
  const frame = document.getElementById("cameraFrame");
  frame.innerHTML = `<div class="camera-snapshot">
    <span class="timestamp">VIRTUAL RPI CAMERA · ${timestamp}</span>
    <div class="room">
      <strong>Smart Home telemetry snapshot</strong>
      <p>Temperature ${state.sensors.temperatura.toFixed(1)}°C · Humidity ${state.sensors.humidade.toFixed(1)}% · Movement ${state.sensors.movimento} cm</p>
      <p>Light ${state.actuators.luz ? "ON" : "OFF"} · Door ${state.actuators.porta ? "OPEN" : "CLOSED"} · Buzzer ${state.actuators.buzzer ? "ON" : "OFF"}</p>
    </div>
    <span class="timestamp">SIMULATED CAPTURE — no physical camera connected</span>
  </div>`;
  document.getElementById("cameraTimestamp").textContent = `Captured at ${timestamp}`;
  toast("Virtual camera snapshot captured");
}

function resetDemo() {
  state = initialState();
  recordHistory();
  saveState();
  document.getElementById("cameraFrame").innerHTML = `<div class="camera-placeholder"><span class="camera-icon">◎</span><strong>No snapshot captured</strong><small>Trigger the virtual camera to simulate the original workflow.</small></div>`;
  document.getElementById("cameraTimestamp").textContent = "No capture yet";
  render();
  toast("Demo reset");
}

document.querySelectorAll(".nav-link[data-view]").forEach((button) => button.addEventListener("click", () => setView(button.dataset.view)));
document.querySelectorAll("[data-history]").forEach((button) => button.addEventListener("click", () => {
  const map = { air: "air", climate: "climate", movement: "movement", door: "door" };
  setView("history");
  document.getElementById("historyFilter").value = map[button.dataset.history] || "all";
  renderHistory();
}));

document.getElementById("historyFilter").addEventListener("change", renderHistory);
document.getElementById("clearHistory").addEventListener("click", () => { state.history = []; saveState(); renderHistory(); toast("Demo history cleared"); });
document.getElementById("captureButton").addEventListener("click", captureCamera);
document.getElementById("resetDemo").addEventListener("click", resetDemo);

bindSwitch("buzzerSwitch", "buzzer", "Buzzer", "OFF");
bindSwitch("luzSwitch", "luz", "Light", "OFF");
bindSwitch("portaSwitch", "porta", "Door", "CLOSED");
bindSwitch("fanSwitch", "fan", "Fan", "OFF");

if (!state.history.length) recordHistory();
render();
setInterval(simulateTelemetry, 3500);
