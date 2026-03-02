#include "RestApiServer.h"

#include "TimeMath.h"

namespace {
String ipToString(const IPAddress& ip) {
    return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

const char kWebPage[] = R"HTML(
<!DOCTYPE html>
<html lang="fr">
    <head>
      <meta charset="UTF-8"/>
      <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
      <title>M2609 Horloge Ininterrompue</title>
      <style>
        :root {
          --bg: #f0f0f5;
          --card: #ffffff;
          --text: #1a1a2e;
          --subtext: #666;
          --border: #e0e0e8;
          --accent: #4f6ef7;
          --accent-h: #3a55d4;
          --off: #c8c8d4;
          --on: #4f6ef7;
          --shadow: 0 2px 14px rgba(0,0,0,0.07);
          --radius: 14px;
          --green: #3dba7a;
          --red: #e05252;
        }
        [data-theme="dark"] {
          --bg: #0f0f1a;
          --card: #1a1a2e;
          --text: #e8e8f4;
          --subtext: #888;
          --border: #2a2a42;
          --accent: #6c7ef5;
          --accent-h: #8a9af8;
          --off: #2e2e48;
          --on: #6c7ef5;
          --shadow: 0 2px 18px rgba(0,0,0,0.45);
          --green: #3dba7a;
          --red: #e05252;
        }

        * { box-sizing: border-box; margin: 0; padding: 0; }

        body {
          font-family: 'Segoe UI', system-ui, sans-serif;
          background: var(--bg);
          color: var(--text);
          min-height: 100vh;
          transition: background .3s, color .3s;
        }

        /* ===== HEADER ===== */
        header {
          display: flex;
          align-items: center;
          justify-content: space-between;
          padding: 16px 28px;
          background: var(--card);
          border-bottom: 1px solid var(--border);
          box-shadow: var(--shadow);
          position: sticky;
          top: 0;
          z-index: 100;
        }

        #themeBtn {
          display: flex; align-items: center; gap: 7px;
          background: var(--bg);
          border: 1px solid var(--border);
          border-radius: 20px;
          padding: 6px 14px;
          cursor: pointer;
          color: var(--text);
          font-size: 0.83rem;
          font-weight: 500;
          transition: border-color .2s, color .2s;
          user-select: none;
        }
        #themeBtn:hover { border-color: var(--accent); color: var(--accent); }

        header h1 {
          font-size: 1.05rem;
          font-weight: 700;
          letter-spacing: .02em;
          margin: 0;
        }

        /* Centre le titre et place le sous-titre en dessous */
        .titleWrap {
          position: absolute;
          left: 50%;
          top: 50%;
          transform: translate(-50%, -50%);
          display: flex;
          flex-direction: column;
          align-items: center;
          gap: 4px;
          pointer-events: none;
        }
        .titleWrap .sub {
          font-size: .82rem;
          color: var(--subtext);
          pointer-events: auto;
        }

        /* Connection status badge */
        #connBadge {
          display: flex;
          align-items: center;
          gap: 7px;
          font-size: 0.82rem;
          font-weight: 600;
          padding: 6px 14px;
          border-radius: 20px;
          border: 1px solid var(--border);
          background: var(--bg);
          transition: all .3s;
          user-select: none;
          cursor: default;
          outline: none;
        }
        #connBadge:disabled { opacity: 1; }
        #connBadge.disconnected { cursor: pointer; }
        #connBadge .dot {
          width: 9px; height: 9px;
          border-radius: 50%;
          flex-shrink: 0;
          transition: background .3s;
        }
        #connBadge.connected   { border-color: var(--green); color: var(--green); }
        #connBadge.connected .dot { background: var(--green); box-shadow: 0 0 6px var(--green); }
        #connBadge.disconnected { border-color: var(--red); color: var(--red); }
        #connBadge.disconnected .dot { background: var(--red); }

        /* ===== IP MODAL ===== */
        #ipModalBackdrop {
          position: fixed;
          inset: 0;
          background: rgba(0, 0, 0, 0.45);
          display: none;
          align-items: center;
          justify-content: center;
          z-index: 200;
          padding: 18px;
        }
        #ipModalBackdrop.open { display: flex; }
        #ipModal {
          width: 100%;
          max-width: 460px;
          background: var(--card);
          border: 1px solid var(--border);
          border-radius: var(--radius);
          box-shadow: var(--shadow);
          padding: 18px;
        }
        #ipModal .modal-head {
          display: flex;
          align-items: center;
          justify-content: space-between;
          margin-bottom: 12px;
        }
        #ipModal .modal-title {
          font-size: .9rem;
          font-weight: 700;
          letter-spacing: .04em;
          text-transform: uppercase;
          color: var(--subtext);
        }
        #ipModalClose {
          border: 1px solid var(--border);
          background: var(--bg);
          color: var(--text);
          border-radius: 8px;
          width: 32px;
          height: 32px;
          cursor: pointer;
          font-size: 1rem;
          line-height: 1;
        }
        #ipModal .modal-help {
          font-size: .82rem;
          color: var(--subtext);
          margin-bottom: 8px;
        }

        /* ===== MAIN ===== */
        main {
          max-width: 600px;
          margin: 0 auto;
          padding: 28px 18px;
          display: flex;
          flex-direction: column;
          gap: 18px;
        }

        /* ===== CARD ===== */
        .card {
          background: var(--card);
          border: 1px solid var(--border);
          border-radius: var(--radius);
          padding: 20px 22px;
          box-shadow: var(--shadow);
        }
        .card-title {
          font-size: 0.7rem;
          font-weight: 700;
          text-transform: uppercase;
          letter-spacing: .12em;
          color: var(--subtext);
          margin-bottom: 14px;
        }

        /* ===== ROW ===== */
        .row {
          display: flex;
          align-items: center;
          justify-content: space-between;
          padding: 10px 0;
          border-bottom: 1px solid var(--border);
        }
        .row:last-child { border-bottom: none; }
        .row-info .label { font-size: .95rem; font-weight: 500; display: flex; align-items: center; gap: 8px; }
        .row-info .sub   { font-size: .77rem; color: var(--subtext); margin-top: 2px; }

        /* ===== TOGGLE SWITCH (draggable) ===== */
        .toggle-wrap {
          position: relative;
          width: 52px;
          height: 28px;
          flex-shrink: 0;
          cursor: pointer;
          user-select: none;
          -webkit-user-select: none;
        }
        .toggle-track {
          position: absolute;
          inset: 0;
          border-radius: 28px;
          background: var(--off);
          transition: background .25s;
        }
        .toggle-thumb {
          position: absolute;
          top: 4px;
          left: 4px;
          width: 20px;
          height: 20px;
          border-radius: 50%;
          background: white;
          box-shadow: 0 1px 5px rgba(0,0,0,.25);
          transition: transform .25s, left .25s;
          pointer-events: none;
        }
        .toggle-wrap.on .toggle-track { background: var(--on); }
        .toggle-wrap.on .toggle-thumb { transform: translateX(24px); }

        /* ===== CLOCK CARD ===== */
        #clockCard .time {
          font-size: 2rem;
          font-weight: 800;
          letter-spacing: .06em;
          color: var(--accent);
          font-variant-numeric: tabular-nums;
          margin-bottom: 4px;
        }
        #clockCard .date {
          font-size: .82rem;
          color: var(--subtext);
        }

    

        /* ===== MANUAL FORM ===== */
        .notice {
          font-size: .79rem;
          color: var(--subtext);
          margin-bottom: 10px;
        }
        .form-inner {
          display: flex;
          flex-direction: column;
          gap: 12px;
        }
        .form-inner.disabled {
          opacity: .35;
          pointer-events: none;
        }
        .form-inner label {
          font-size: .8rem;
          color: var(--subtext);
          margin-bottom: 4px;
          display: block;
        }
        .form-inner input[type="datetime-local"] {
          width: 100%;
          padding: 10px 13px;
          border-radius: 10px;
          border: 1px solid var(--border);
          background: var(--bg);
          color: var(--text);
          font-size: .95rem;
          outline: none;
          transition: border .2s;
        }
        .form-inner input[type="datetime-local"]:focus {
          border-color: var(--accent);
        }
        /* Style pour l'input IP — même visuel que le champ datetime-local */
        #arduinoIpInput,
        .row-info input[type="text"] {
          width: 100%;
          padding: 10px 13px;
          border-radius: 10px;
          border: 1px solid var(--border);
          background: var(--bg);
          color: var(--text);
          font-size: .95rem;
          outline: none;
          transition: border .2s;
          margin-top: 8px;
        }
        #arduinoIpInput:focus,
        .row-info input[type="text"]:focus {
          border-color: var(--accent);
        }
        #sendBtn {
          padding: 11px 22px;
          border-radius: 10px;
          border: none;
          background: var(--accent);
          color: white;
          font-size: .93rem;
          font-weight: 600;
          cursor: pointer;
          transition: background .2s, transform .1s;
          align-self: flex-start;
        }
        #sendBtn:hover { background: var(--accent-h); }
        #sendBtn:active { transform: scale(.97); }

        #sentResult {
          font-size: .88rem;
          color: var(--accent);
          font-weight: 600;
          min-height: 1.3em;
        }

        footer {
          text-align: center;
          padding: 16px;
          font-size: .73rem;
          color: var(--subtext);
        }
      </style>
    </head>
    <body data-theme="light">
        <header>
          <button id="themeBtn" onclick="toggleTheme()">
            <span id="themeIcon">🌙</span>
            <span id="themeLabel">Mode sombre</span>
          </button>

          <div class="titleWrap">
            <h1>M2609 Horloge Ininterrompue</h1>
            <div class="sub">Ambroise de Gramont & David Goletta - février 2026</div>
          </div>

          <button id="connBadge" class="disconnected" type="button" aria-label="Ouvrir la configuration IP" disabled>
            <span class="dot"></span>
            <span id="connLabel">Déconnecté</span>
          </button>
        </header>

        <div id="ipModalBackdrop" aria-hidden="true">
          <div id="ipModal" role="dialog" aria-modal="true" aria-labelledby="ipModalTitle">
            <div class="modal-head">
              <div class="modal-title" id="ipModalTitle">IP Arduino</div>
              <button id="ipModalClose" type="button" aria-label="Fermer">✕</button>
            </div>
            <div class="modal-help">Adresse IP de l'Arduino (Exemple: 192.168.4.1)</div>
            <input type="text" id="arduinoIpInput" placeholder="Entrez l'IP de l'Arduino">
          </div>
        </div>

        <main>

          <!-- Sources -->
          <div class="card">
            <div class="card-title">Sources de temps</div>

            <div class="row">
              <div class="row-info">
                <div class="label">DAB</div>
                <div class="sub">Synchronisation via signal DAB+</div>
              </div>
              <div class="toggle-wrap" id="dabToggle" data-id="dab"></div>
            </div>

            <div class="row">
              <div class="row-info">
                <div class="label">NTP</div>
                <div class="sub">Synchronisation réseau (Internet)</div>
              </div>
              <div class="toggle-wrap" id="ntpToggle" data-id="ntp"></div>
            </div>

            <div class="row">
              <div class="row-info">
                <div class="label">GPS</div>
                <div class="sub">Synchronisation satellite GPS</div>
              </div>
              <div class="toggle-wrap" id="gpsToggle" data-id="gps"></div>
            </div>
          </div>

          <!-- Horloge Arduino -->
          <div class="card" id="clockCard">
            <div class="card-title">Heure Arduino</div>
            <div class="clockRow">
              <div class="digitalWrap">
                <div class="time" id="clockTime">--:--:--</div>
                <div class="date" id="clockDate">Aucune heure définie</div>
              </div>
            </div>
          </div>

          <!-- Fuseau horaire -->
          <div class="card">
            <div class="card-title">Fuseau horaire</div>
            <div class="row">
              <div class="row-info">
                <div class="label">Sélectionnez un fuseau</div>
                <select id="timezoneSelect" style="margin-top:8px; width:100%; padding:10px 13px; border-radius:10px; border:1px solid var(--border); background:var(--bg); color:var(--text);">
                  <option value="-720">UTC -12:00</option>
                  <option value="-660">UTC -11:00</option>
                  <option value="-600">UTC -10:00</option>
                  <option value="-540">UTC -09:00</option>
                  <option value="-480">UTC -08:00</option>
                  <option value="-420">UTC -07:00</option>
                  <option value="-360">UTC -06:00</option>
                  <option value="-300">UTC -05:00</option>
                  <option value="-240">UTC -04:00</option>
                  <option value="-180">UTC -03:00</option>
                  <option value="-120">UTC -02:00</option>
                  <option value="-60">UTC -01:00</option>
                  <option value="0" selected>UTC ±00:00</option>
                  <option value="60">UTC +01:00</option>
                  <option value="120">UTC +02:00</option>
                  <option value="180">UTC +03:00</option>
                  <option value="240">UTC +04:00</option>
                  <option value="300">UTC +05:00</option>
                  <option value="360">UTC +06:00</option>
                  <option value="420">UTC +07:00</option>
                  <option value="480">UTC +08:00</option>
                  <option value="540">UTC +09:00</option>
                  <option value="600">UTC +10:00</option>
                  <option value="660">UTC +11:00</option>
                  <option value="720">UTC +12:00</option>
                </select>
              </div>
            </div>
          </div>

          <!-- Réglage manuel -->
          <div class="card">
            <div class="card-title">Réglage manuel</div>
            <p class="notice" id="formNotice">Désactivez toutes les sources pour activer le réglage.</p>
            <div class="form-inner disabled" id="formInner">
              <div>
                <label for="dtInput">Date et heure locale</label>
                <input type="datetime-local" id="dtInput" step="1"/>
              </div>
              <button id="sendBtn" onclick="sendTime()">Envoyer à l'Arduino</button>
              <div id="sentResult"></div>
            </div>
          </div>

        </main>

        <footer><a href="mailto:david.goletta@eduvaud.ch,ambroise.degramont@eduvaud.ch">Nous contacter</a></footer>

        <script>
          // ================================================================
          // État local
          // ================================================================
          const state = {
            dab: false,
            ntp: false,
            gps: false,
            timeSet: false,
            // référence locale pour faire tourner l'horloge entre les polls
            localBaseEpoch: null,    // Date.now() quand on a reçu l'heure
            arduinoBaseEpoch: null,  // epoch correspondant à l'heure Arduino reçue
          };

          // ================================================================
          // THÈME
          // ================================================================
          function toggleTheme() {
              const dark = document.body.getAttribute("data-theme") === "dark";
              const newTheme = dark ? "light" : "dark";
              document.body.setAttribute("data-theme", newTheme);
              localStorage.setItem("theme", newTheme);
              document.getElementById("themeIcon").textContent  = dark ? "🌙" : "☀️";
              document.getElementById("themeLabel").textContent = dark ? "Mode sombre" : "Mode clair";
          }

          // Load theme from localStorage on init
          (function() {
              const saved = localStorage.getItem("theme") || "light";
              document.body.setAttribute("data-theme", saved);
              const isDark = saved === "dark";
              document.getElementById("themeIcon").textContent  = isDark ? "☀️" : "🌙";
              document.getElementById("themeLabel").textContent = isDark ? "Mode clair" : "Mode sombre";
          })();

          // ================================================================
          // BASE URL POUR LES REQUÊTES (sera mis à jour par l'input IP)
          // ================================================================
          let BASE = "";
          let isConnected = false;

          // Load IP from localStorage on init
          (function() {
            const savedIp = localStorage.getItem("arduinoIp") || "";
            if (savedIp) {
              document.getElementById("arduinoIpInput").value = savedIp;
              BASE = `http://${savedIp}`;
            } else {
              setTimeout(openIpModal, 0);
            }
          })();

          // Update BASE and localStorage on input change
          document.getElementById("arduinoIpInput").addEventListener("change", (e) => {
            const ip = e.target.value.trim();
            if (ip) {
              BASE = `http://${ip}`;
              localStorage.setItem("arduinoIp", ip);
              pollStatus(); // tester la connexion immédiatement
            }
          });

          function openIpModal() {
            if (isConnected) return;
            const backdrop = document.getElementById("ipModalBackdrop");
            backdrop.classList.add("open");
            backdrop.setAttribute("aria-hidden", "false");
            document.getElementById("arduinoIpInput").focus();
          }

          function closeIpModal() {
            const backdrop = document.getElementById("ipModalBackdrop");
            backdrop.classList.remove("open");
            backdrop.setAttribute("aria-hidden", "true");
          }

          document.getElementById("connBadge").addEventListener("click", openIpModal);
          document.getElementById("ipModalClose").addEventListener("click", closeIpModal);
          document.getElementById("ipModalBackdrop").addEventListener("click", (e) => {
            if (e.target.id === "ipModalBackdrop") closeIpModal();
          });
          document.addEventListener("keydown", (e) => {
            if (e.key === "Escape") closeIpModal();
          });

          // ================================================================
          // TOGGLE SWITCH — construction + drag
          // ================================================================
          function buildToggle(wrap) {
            const track = document.createElement("div");
            track.className = "toggle-track";
            const thumb = document.createElement("div");
            thumb.className = "toggle-thumb";
            wrap.appendChild(track);
            wrap.appendChild(thumb);

            let dragging = false;
            let startX = 0;
            let startState = false;
            const THRESHOLD = 12; // px drag minimum pour considérer c'est un drag

            function getClientX(e) {
              return e.touches ? e.touches[0].clientX : e.clientX;
            }

            function pointerDown(e) {
              dragging = false;
              startX = getClientX(e);
              startState = wrap.classList.contains("on");
              document.addEventListener("mousemove", pointerMove);
              document.addEventListener("mouseup",   pointerUp);
              document.addEventListener("touchmove", pointerMove, {passive:true});
              document.addEventListener("touchend",  pointerUp);
            }

            function pointerMove(e) {
              const dx = getClientX(e) - startX;
              if (Math.abs(dx) > THRESHOLD) dragging = true;
              if (dragging) {
                // Feedback visuel pendant le drag
                const wantOn = startState ? dx > -THRESHOLD/2 : dx > THRESHOLD/2;
                wrap.classList.toggle("on", wantOn);
              }
            }

            function pointerUp(e) {
              document.removeEventListener("mousemove", pointerMove);
              document.removeEventListener("mouseup",   pointerUp);
              document.removeEventListener("touchmove", pointerMove);
              document.removeEventListener("touchend",  pointerUp);

              let newState;
              if (dragging) {
                newState = wrap.classList.contains("on");
              } else {
                // Simple clic → toggle
                newState = !startState;
              }

              // Revenir à l'état précédent le temps de la réponse
              wrap.classList.toggle("on", startState);
              applyToggle(wrap, newState);
            }

            wrap.addEventListener("mousedown",  pointerDown);
            wrap.addEventListener("touchstart", pointerDown, {passive:true});
          }

          function applyToggle(wrap, newState) {
            const id = wrap.getAttribute("data-id");
            sendSource(id, wrap, newState);
          }

          // ================================================================
          // REQUÊTES
          // ================================================================

          async function sendSource(src, wrap, want) {
            const val = want ? "1" : "0";
            try {
              await fetch(BASE + "/toggle-source", {
                method: "POST",
                headers: { "Content-Type": "application/x-www-form-urlencoded" },
                body: `source=${src}&value=${val}`
              });
              state[src] = want;
              wrap.classList.toggle("on", want);
              updateFormState();
            } catch (e) {
              wrap.classList.toggle("on", state[src]);
            }
          }

          async function sendTime() {
            const input = document.getElementById("dtInput").value;
            if (!input) return;
            const dt = new Date(input);
            const pad = n => String(n).padStart(2, "0");
            const body = [
              `year=${dt.getFullYear()}`,
              `month=${pad(dt.getMonth()+1)}`,
              `day=${pad(dt.getDate())}`,
              `hour=${pad(dt.getHours())}`,
              `minute=${pad(dt.getMinutes())}`,
              `second=${pad(dt.getSeconds())}`
            ].join("&");

            const resultEl = document.getElementById("sentResult");
            resultEl.textContent = "Envoi en cours…";
            try {
              const res = await fetch(BASE + "/set-time", {
                method: "POST",
                headers: { "Content-Type": "application/x-www-form-urlencoded" },
                body
              });
              const text = await res.text();
              const trimmed = text.trim();
              resultEl.textContent = "✓ Heure définie : " + trimmed;
              // Stocker la base pour le ticker local
              syncLocalClock(trimmed);
            } catch (e) {
              resultEl.textContent = "✗ Erreur de connexion.";
            }
          }

          // ================================================================
          // HORLOGE
          // ================================================================
          // Reçoit "YYYY-MM-DD HH:MM:SS", stocke la base epoch pour le ticker
          function syncLocalClock(arduinoTimeStr) {
            if (!arduinoTimeStr || arduinoTimeStr === "Non definie") {
              state.timeSet = false;
              return;
            }
            // Parser "YYYY-MM-DD HH:MM:SS"
            const [datePart, timePart] = arduinoTimeStr.split(" ");
            if (!datePart || !timePart) return;
            const [y, mo, d] = datePart.split("-").map(Number);
            const [h, mi, s] = timePart.split(":").map(Number);
            // Créer un objet Date en heure locale (on ignore les fuseaux, on affiche tel quel)
            const base = new Date(y, mo - 1, d, h, mi, s, 0);
            state.arduinoBaseEpoch = base.getTime();
            state.localBaseEpoch   = Date.now();
            state.timeSet = true;
          }

          function tickClock() {
            if (!state.timeSet) {
              document.getElementById("clockTime").textContent = "--:--:--";
              document.getElementById("clockDate").textContent = "Aucune heure définie";
              return;
            }
            const elapsed = Date.now() - state.localBaseEpoch;
            const now = new Date(state.arduinoBaseEpoch + elapsed);

            const pad = n => String(n).padStart(2, "0");
            const hh = pad(now.getHours());
            const mm = pad(now.getMinutes());
            const ss = pad(now.getSeconds());

            const months = ["jan","fév","mar","avr","mai","jun","jul","aoû","sep","oct","nov","déc"];
            const dateStr = `${now.getDate()} ${months[now.getMonth()]} ${now.getFullYear()}`;

            document.getElementById("clockTime").textContent = `${hh}:${mm}:${ss}`;
            document.getElementById("clockDate").textContent = dateStr;

          }

          // ================================================================
          // ÉTAT FORMULAIRE
          // ================================================================
          function updateFormState() {
            const allOff = !state.dab && !state.ntp && !state.gps;
            document.getElementById("formInner").classList.toggle("disabled", !allOff);
            document.getElementById("formNotice").style.display = allOff ? "none" : "block";
          }

          // ================================================================
          // PRÉ-REMPLIR DATE/HEURE LOCALE
          // ================================================================
          function setDefaultDatetime() {
            const now = new Date();
            const pad = n => String(n).padStart(2, "0");
            const str = `${now.getFullYear()}-${pad(now.getMonth()+1)}-${pad(now.getDate())}T` +
                        `${pad(now.getHours())}:${pad(now.getMinutes())}:${pad(now.getSeconds())}`;
            document.getElementById("dtInput").value = str;
          }

          // ================================================================
          // FUSEAU HORAIRE — persistance simple
          // ================================================================
          function initTimezoneSelect() {
            const sel = document.getElementById('timezoneSelect');
            if (!sel) return;
            const saved = localStorage.getItem('timezoneOffset');
            if (saved) sel.value = saved;
            sel.addEventListener('change', (e) => {
              localStorage.setItem('timezoneOffset', e.target.value);
            });
          }

          function getSelectedTimezoneOffsetMinutes() {
            const saved = localStorage.getItem('timezoneOffset');
            if (saved) return parseInt(saved, 10);
            const sel = document.getElementById('timezoneSelect');
            return sel ? parseInt(sel.value, 10) : 0;
          }

          // ================================================================
          // ANALOG CLOCK (canvas)
          // ================================================================
          let _analog = { ctx: null, size: 0, scale: 1 };
          function initAnalog() {
            const c = document.getElementById('analogClock');
            if (!c || !c.getContext) return;
            const ctx = c.getContext('2d');
            // Handle high-DPI displays and keep drawing coordinates in CSS pixels
            const rect = c.getBoundingClientRect();
            const dpr = window.devicePixelRatio || 1;
            const cssW = rect.width;
            const cssH = rect.height;
            const w = Math.round(cssW * dpr);
            const h = Math.round(cssH * dpr);
            c.width = w;
            c.height = h;
            c.style.width = cssW + 'px';
            c.style.height = cssH + 'px';
            // Reset transform and set scale so drawing uses CSS pixels coordinates
            ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
            _analog = { ctx, cssW, cssH, size: Math.min(cssW, cssH), dpr };
            // Draw initial static face
            drawAnalog(new Date());
          }

          function drawAnalog(now) {
            if (!_analog.ctx) return;
            const ctx = _analog.ctx;
            const cssW = _analog.cssW;
            const cssH = _analog.cssH;
            const size = _analog.size;
            const cx = cssW / 2;
            const cy = cssH / 2;
            const r = Math.min(cssW, cssH) / 2 * 0.9;
          }

          // ================================================================
          // POLLING STATUT
          // ================================================================
          async function pollStatus() {
            const badge = document.getElementById("connBadge");
            const label = document.getElementById("connLabel");
            try {
              const res = await fetch(BASE + "/status", { signal: AbortSignal.timeout(2000) });
              const data = await res.json();

              isConnected = true;
              badge.className = "connected";
              badge.disabled = true;
              label.textContent = "Connecté";
              closeIpModal();

              // Sync états
              state.dab = data.dab;
              state.ntp = data.ntp;
              state.gps = data.gps;

              document.getElementById("dabToggle").classList.toggle("on", data.dab);
              document.getElementById("ntpToggle").classList.toggle("on", data.ntp);
              document.getElementById("gpsToggle").classList.toggle("on", data.gps);

              updateFormState();

              // Sync horloge depuis Arduino si heure définie
              if (data.timeSet) {
                syncLocalClock(data.time);
              }
            } catch(e) {
              isConnected = false;
              badge.className = "disconnected";
              badge.disabled = false;
              label.textContent = "Déconnecté";
              console.warn("Erreur de connexion à l'Arduino:", e);
            }
          }

          // ================================================================
          // INIT
          // ================================================================
          document.querySelectorAll(".toggle-wrap").forEach(buildToggle);
          setDefaultDatetime();
          initTimezoneSelect();
          initAnalog();
          updateFormState();

          // Ticker horloge 1×/s (indépendant du réseau)
          setInterval(tickClock, 1000);
          tickClock();

          // Polling Arduino 2×/s
          pollStatus();
          setInterval(pollStatus, 2000);
        </script>
    </body>
</html>
)HTML";
}  // namespace

RestApiServer::RestApiServer(TimeCoordinator& coordinator, TimeSource* sources[], uint8_t sourceCount, Notification* notifier, uint16_t port)
    : coordinator_(coordinator), notifier_(notifier), server_(port) {
    sourceCount_ = min(sourceCount, TimeCoordinator::MAX_SOURCES);
    for (uint8_t i = 0; i < sourceCount_; ++i) {
        sources_[i] = sources[i];
    }
}

void RestApiServer::begin() {
    server_.begin();
    if (!notifier_) return;

    notifier_->info("REST API server started on port 80");
    notifier_->info("Current interface IP: " + ipToString(WiFi.localIP()));
}

void RestApiServer::update() {
    WiFiClient client = server_.available();
    if (!client) return;

    const unsigned long timeoutAt = millis() + 3000;
    while (!client.available() && millis() < timeoutAt) {
        delay(1);
    }

    if (!client.available()) {
        client.stop();
        return;
    }

    String requestLine = client.readStringUntil('\n');
    requestLine.trim();

    String method;
    String path;
    if (!parseRequestLine(requestLine, method, path)) {
        sendResponse(client, 200, "text/plain", "Arduino UNO R4 WiFi - Server active.");
        client.stop();
        return;
    }

    int contentLength = 0;
    while (true) {
        String headerLine = client.readStringUntil('\n');
        headerLine.trim();
        if (headerLine.startsWith("Content-Length:")) {
            contentLength = headerLine.substring(15).toInt();
        }
        if (headerLine.length() == 0) {
            break;
        }
    }

    if (method == "OPTIONS") {
        sendResponse(client, 204, "text/plain", "");
        client.stop();
        return;
    }

    if (method == "GET" && path == "/") {
        sendWebPage(client);
        client.stop();
        return;
    }

    if (method == "GET" && path == "/status") {
        sendStatus(client);
        client.stop();
        return;
    }

    if (method == "POST" && path == "/toggle-source") {
        String body = readBody(client, contentLength);
        String message;
        handleToggleSource(body, message);
        sendResponse(client, 200, "text/plain", message);
        client.stop();
        return;
    }

    if (method == "POST" && path == "/set-time") {
        String body = readBody(client, contentLength);
        String message;
        handleSetTime(body, message);
        sendResponse(client, 200, "text/plain", message);
        client.stop();
        return;
    }

    sendResponse(client, 200, "text/plain", "Arduino UNO R4 WiFi - Server active.");
    client.stop();
}

/**
 * @internal
 * @brief Map source name text to source index.
 * @details
 * Normalizes the incoming source name and resolves it to the corresponding internal source slot.
 *
 * @param sourceName Source identifier string (e.g. "dab", "ntp", "gps").
 * @param outIndex Output index written when mapping succeeds.
 * @return True if the source name is valid and available in configured source count.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
bool RestApiServer::sourceIndexFromName(const String& sourceName, uint8_t& outIndex) const {
    String normalized = sourceName;
    normalized.toLowerCase();

    if (normalized == "dab") {
        outIndex = 0;
        return (sourceCount_ > 0);
    }
    if (normalized == "ntp") {
        outIndex = 1;
        return (sourceCount_ > 1);
    }
    if (normalized == "gps") {
        outIndex = 2;
        return (sourceCount_ > 2);
    }

    return false;
}

/**
 * @internal
 * @brief Parse HTTP request line into method and path.
 * @details
 * Extracts the HTTP verb and target path from the first request line and strips query parameters.
 *
 * @param requestLine Raw first HTTP request line.
 * @param method Output HTTP method token.
 * @param path Output normalized path component.
 * @return True when parsing succeeds and both method/path are non-empty.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
bool RestApiServer::parseRequestLine(const String& requestLine, String& method, String& path) const {
    int firstSpace = requestLine.indexOf(' ');
    if (firstSpace < 0) return false;
    int secondSpace = requestLine.indexOf(' ', firstSpace + 1);
    if (secondSpace < 0) return false;

    method = requestLine.substring(0, firstSpace);
    path = requestLine.substring(firstSpace + 1, secondSpace);
    int queryStart = path.indexOf('?');
    if (queryStart >= 0) {
        path = path.substring(0, queryStart);
    }
    return (method.length() > 0 && path.length() > 0);
}

/**
 * @internal
 * @brief Read HTTP request body payload.
 * @details
 * Reads up to the announced content length with a short timeout and returns collected data.
 *
 * @param client Active WiFi client connection.
 * @param contentLength Expected payload size from request headers.
 * @return Body payload as string (possibly partial on timeout).
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
String RestApiServer::readBody(WiFiClient& client, int contentLength) const {
    if (contentLength <= 0) return "";

    String body;
    body.reserve(contentLength);
    const unsigned long timeoutAt = millis() + 2000;

    while (static_cast<int>(body.length()) < contentLength && millis() < timeoutAt) {
        if (client.available()) {
            body += static_cast<char>(client.read());
        }
    }

    return body;
}

/**
 * @internal
 * @brief Decode URL-encoded form text.
 * @details
 * Converts '+' to spaces and `%HH` hex escapes to their character representation.
 *
 * @param input URL-encoded input text.
 * @return Decoded string.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
String RestApiServer::urlDecode(const String& input) const {
    String decoded;
    decoded.reserve(input.length());

    for (int i = 0; i < input.length(); ++i) {
        char c = input[i];
        if (c == '+') {
            decoded += ' ';
        } else if (c == '%' && i + 2 < input.length()) {
            char hex[3] = {input[i + 1], input[i + 2], '\0'};
            decoded += static_cast<char>(strtol(hex, nullptr, 16));
            i += 2;
        } else {
            decoded += c;
        }
    }

    return decoded;
}

/**
 * @internal
 * @brief Extract one form parameter from request body.
 * @details
 * Finds `key=value` in an x-www-form-urlencoded payload and returns decoded value.
 *
 * @param body Request body containing form data.
 * @param key Parameter key to lookup.
 * @return Decoded parameter value, or empty string if key is absent.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
String RestApiServer::getParam(const String& body, const String& key) const {
    String search = key + "=";
    int start = body.indexOf(search);
    if (start < 0) return "";

    start += search.length();
    int end = body.indexOf('&', start);
    if (end < 0) end = body.length();

    return urlDecode(body.substring(start, end));
}

/**
 * @internal
 * @brief Send HTTP response to client.
 * @details
 * Writes status line, CORS headers, content type, and optional body to the active client.
 *
 * @param client Active WiFi client connection.
 * @param code HTTP status code.
 * @param contentType Response content type header value.
 * @param body Response payload.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void RestApiServer::sendResponse(WiFiClient& client, int code, const String& contentType, const String& body) const {
    String status = "200 OK";
    if (code == 204) {
        status = "204 No Content";
    }

    client.println("HTTP/1.1 " + status);
    client.println("Access-Control-Allow-Origin: *");
    client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");
    client.println("Access-Control-Allow-Headers: Content-Type");
    client.println("Content-Type: " + contentType);
    client.println("Connection: close");
    client.println();

    if (body.length() > 0) {
        client.println(body);
    }
}

/**
 * @internal
 * @brief Send current source/time status as JSON.
 * @details
 * Builds a compact JSON snapshot with source enabled states and current manual time status.
 *
 * @param client Active WiFi client connection.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void RestApiServer::sendStatus(WiFiClient& client) const {
    bool timeSet = false;
    String currentTime = currentTimeIsoString(timeSet);

    String json = "{";
    json += "\"dab\":" + String((sourceCount_ > 0 && sources_[0] && sources_[0]->isEnabled()) ? "true" : "false") + ",";
    json += "\"ntp\":" + String((sourceCount_ > 1 && sources_[1] && sources_[1]->isEnabled()) ? "true" : "false") + ",";
    json += "\"gps\":" + String((sourceCount_ > 2 && sources_[2] && sources_[2]->isEnabled()) ? "true" : "false") + ",";
    json += "\"timeSet\":" + String(timeSet ? "true" : "false") + ",";
    json += "\"time\":\"" + currentTime + "\"";
    json += "}";

    sendResponse(client, 200, "application/json", json);
}

/**
 * @internal
 * @brief Serve embedded HTML control page.
 * @details
 * Sends the precompiled web UI page used for browser-based control of the device.
 *
 * @param client Active WiFi client connection.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void RestApiServer::sendWebPage(WiFiClient& client) const {
    sendResponse(client, 200, "text/html", String(kWebPage));
}

/**
 * @internal
 * @brief Handle source enable/disable command.
 * @details
 * Parses source and desired state from request body, applies the new source state,
 * and triggers an immediate coordinator resynchronization.
 *
 * @param body URL-encoded request payload.
 * @param message Output response text.
 * @return True when handler completes processing.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
bool RestApiServer::handleToggleSource(const String& body, String& message) {
    String sourceName = getParam(body, "source");
    String value = getParam(body, "value");
    bool enabled = (value == "1");

    uint8_t index = 0;
    if (sourceIndexFromName(sourceName, index) && sources_[index]) {
        sources_[index]->setEnabled(enabled);
        coordinator_.forceSync();

        if (notifier_) {
            notifier_->info("REST toggle source " + sourceName + " => " + (enabled ? "ON" : "OFF"));
        }
    }

    message = "OK";
    return true;
}

/**
 * @internal
 * @brief Handle manual time update command.
 * @details
 * Parses date/time fields, clamps values to valid ranges, applies them to coordinator,
 * and returns the resulting formatted time string.
 *
 * @param body URL-encoded request payload.
 * @param message Output response text.
 * @return True when handler completes processing.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
bool RestApiServer::handleSetTime(const String& body, String& message) {
    int year = getParam(body, "year").toInt();
    int month = getParam(body, "month").toInt();
    int day = getParam(body, "day").toInt();
    int hour = getParam(body, "hour").toInt();
    int minute = getParam(body, "minute").toInt();
    int second = getParam(body, "second").toInt();

    if (year < 1970) year = 1970;
    if (year > 9999) year = 9999;
    if (month < 1) month = 1;
    if (month > 12) month = 12;
    if (day < 1) day = 1;
    int maxDay = TimeMath::daysInMonth(static_cast<uint16_t>(year), static_cast<uint8_t>(month));
    if (day > maxDay) day = maxDay;
    if (hour < 0) hour = 0;
    if (hour > 23) hour = 23;
    if (minute < 0) minute = 0;
    if (minute > 59) minute = 59;
    if (second < 0) second = 0;
    if (second > 59) second = 59;

    DateTimeFields dt{};
    dt.date.year = static_cast<uint16_t>(year);
    dt.date.month = static_cast<uint8_t>(month);
    dt.date.day = static_cast<uint8_t>(day);
    dt.time.hour = static_cast<uint8_t>(hour);
    dt.time.minute = static_cast<uint8_t>(minute);
    dt.time.second = static_cast<uint8_t>(second);

    coordinator_.setManualDateTime(dt);
    manualTimeSet_ = true;

    if (notifier_) {
        bool hasTime = false;
        String timeText = currentTimeIsoString(hasTime);
        notifier_->info("REST manual time set to " + timeText);
    }

    bool hasTime = false;
    message = currentTimeIsoString(hasTime);
    return true;
}

/**
 * @internal
 * @brief Build formatted current time string.
 * @details
 * Returns a fixed-format timestamp (`YYYY-MM-DD HH:MM:SS`) when manual time is set
 * and coordinator provides current date-time; otherwise returns a "not defined" marker.
 *
 * @param isSet Output flag indicating whether a valid time value is available.
 * @return Formatted time string or fallback text.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
String RestApiServer::currentTimeIsoString(bool& isSet) const {
    if (!manualTimeSet_) {
        isSet = false;
        return "Non definie";
    }

    DateTimeFields dt{};
    if (!coordinator_.getCurrentDateTime(dt)) {
        isSet = false;
        return "Non definie";
    }

    isSet = true;
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u %02u:%02u:%02u",
             dt.date.year,
             dt.date.month,
             dt.date.day,
             dt.time.hour,
             dt.time.minute,
             dt.time.second);
    return String(buffer);
}
