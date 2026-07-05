// ============================================================================
// Comunicacion con el servidor C++ y dibujo/animacion de las estructuras.
// Escenario 1.1: construir y mostrar estructuras.
// Escenario 1.2: animar el backward search paso a paso.
// ============================================================================

const buildBtn    = document.getElementById("buildBtn");
const textInput   = document.getElementById("textInput");
const statusEl    = document.getElementById("status");
const outputEl    = document.getElementById("output");
const patternInput= document.getElementById("patternInput");
const searchBtn   = document.getElementById("searchBtn");
const replayBtn   = document.getElementById("replayBtn");
const stepInfoEl  = document.getElementById("stepInfo");

let ultimaTraza = null;   // guarda la ultima traza para poder repetirla

// --- Eventos ---
buildBtn.addEventListener("click", construir);
textInput.addEventListener("keydown", (e) => { if (e.key === "Enter") construir(); });
searchBtn.addEventListener("click", buscar);
patternInput.addEventListener("keydown", (e) => { if (e.key === "Enter") buscar(); });
replayBtn.addEventListener("click", () => { if (ultimaTraza) animarTraza(ultimaTraza); });

// ============================================================================
// CONSTRUIR (escenario 1.1)
// ============================================================================
async function construir() {
    const texto = textInput.value.trim().toUpperCase();
    if (!texto) { mostrarStatus("Escribe una cadena primero.", true); return; }
    if (!/^[ACGT]+$/.test(texto)) {
        mostrarStatus("Solo se permiten los caracteres A, C, G, T.", true); return;
    }

    mostrarStatus("Construyendo...", false);
    try {
        const res = await fetch("/build", { method: "POST", body: texto });
        if (!res.ok) { mostrarStatus("Error del servidor: " + res.status, true); return; }
        const estado = await res.json();
        dibujarTodo(estado);
        mostrarStatus("Listo. Ahora puedes buscar un patrón.", false);
        outputEl.classList.remove("hidden");
        outputEl.classList.remove("fade-in");
        void outputEl.offsetWidth;
        outputEl.classList.add("fade-in");
        limpiarResaltado();
        stepInfoEl.classList.add("hidden");
        replayBtn.classList.add("hidden");
        ultimaTraza = null;
    } catch (err) {
        mostrarStatus("No se pudo conectar con el servidor.", true);
        console.error(err);
    }
}

// ============================================================================
// BUSCAR (escenario 1.2)
// ============================================================================
async function buscar() {
    const patron = patternInput.value.trim().toUpperCase();
    if (!patron) { mostrarStatus("Escribe un patrón para buscar.", true); return; }
    if (!/^[ACGT]+$/.test(patron)) {
        mostrarStatus("El patrón solo puede tener A, C, G, T.", true); return;
    }

    mostrarStatus("Buscando...", false);
    try {
        const res = await fetch("/search", { method: "POST", body: patron });
        if (!res.ok) { mostrarStatus("Error del servidor: " + res.status, true); return; }
        const traza = await res.json();
        ultimaTraza = traza;
        await animarTraza(traza);
        replayBtn.classList.remove("hidden");
    } catch (err) {
        mostrarStatus("No se pudo conectar con el servidor.", true);
        console.error(err);
    }
}

// Reproduce la traza paso a paso con pausas
async function animarTraza(traza) {
    limpiarResaltado();
    stepInfoEl.classList.remove("hidden");

    // Rango inicial: todo el SA
    stepInfoEl.innerHTML = `<div class="step-title">Inicio</div>` +
        `<div>Rango inicial: [lo=0, hi=${traza.n}) — todos los sufijos</div>`;
    resaltarRango(0, traza.n, null);
    await dormir(1350);

    for (const step of traza.steps) {
        if (step.outOfAlphabet) {
            stepInfoEl.innerHTML =
                `<div class="step-title">Paso ${step.step}: '${step.char}'</div>` +
                `<div class="step-fail">El carácter '${step.char}' no está en el alfabeto. El patrón no existe.</div>`;
            limpiarResaltado();
            break;
        }

        // Mostrar el calculo del paso
        stepInfoEl.innerHTML =
            `<div class="step-title">Paso ${step.step}: procesando '${step.char}' (sufijo acumulado: "${step.matched}")</div>` +
            `<div class="calc">lo = C[${step.char}] + rank(${step.lo_before}, ${step.char}) = ${step.C} + ${step.rank_lo} = <b>${step.lo_after}</b></div>` +
            `<div class="calc">hi = C[${step.char}] + rank(${step.hi_before}, ${step.char}) = ${step.C} + ${step.rank_hi} = <b>${step.hi_after}</b></div>` +
            (step.empty
                ? `<div class="step-fail">Rango vacío → el patrón no aparece.</div>`
                : `<div class="step-ok">Rango [${step.lo_after}, ${step.hi_after}) → ${step.occ} sufijo(s) empiezan con "${step.matched}"</div>`);

        // Resaltar las filas del nuevo rango
        if (step.empty) {
            limpiarResaltado();
        } else {
            resaltarRango(step.lo_after, step.hi_after, step.char);
        }
        await dormir(2000);
    }

    // Resultado final
    const r = traza.result;
    if (r.found) {
        stepInfoEl.innerHTML +=
            `<div class="step-result">✓ Encontrado: ${r.count} ocurrencia(s) en posición(es) ${r.positions.join(", ")}</div>`;
        mostrarStatus(`"${traza.pattern}" → ${r.count} ocurrencia(s).`, false);
    } else {
        stepInfoEl.innerHTML +=
            `<div class="step-result fail">✗ El patrón "${traza.pattern}" no aparece en la cadena.</div>`;
        mostrarStatus(`"${traza.pattern}" → 0 ocurrencias.`, false);
    }
}

// ============================================================================
// Resaltado de filas de la tabla de sufijos
// ============================================================================
function resaltarRango(lo, hi, char) {
    const filas = document.querySelectorAll("#suffixTable tr[data-row]");
    filas.forEach((tr) => {
        const row = parseInt(tr.dataset.row, 10);
        const debeEstar = (row >= lo && row < hi);
        const esta = tr.classList.contains("row-active");
        if (debeEstar && !esta) tr.classList.add("row-active");
        else if (!debeEstar && esta) tr.classList.remove("row-active");
    });
}

function limpiarResaltado() {
    document.querySelectorAll("#suffixTable tr.row-active")
        .forEach((tr) => tr.classList.remove("row-active"));
}

function dormir(ms) { return new Promise((r) => setTimeout(r, ms)); }

// ============================================================================
// DIBUJO DE ESTRUCTURAS (escenario 1.1)
// ============================================================================
function dibujarTodo(s) {
    dibujarResumen(s);
    dibujarTablaSufijos(s);
    dibujarTablaC(s);
    dibujarWaveletTree(s);
}

function dibujarResumen(s) {
    const el = document.getElementById("summary");
    el.innerHTML =
        `n = <span>${s.n}</span> caracteres (incluye $)<br>` +
        `Alfabeto: <span>${s.alphabet}</span> (σ = <span>${s.alphabet.length}</span>)<br>` +
        `BWT: <span>${s.bwt}</span>`;
}

function dibujarTablaSufijos(s) {
    let html = "<table><tr><th>i</th><th>SA[i]</th><th>F</th><th>L (BWT)</th></tr>";
    for (const row of s.suffixes) {
        html += `<tr data-row="${row.row}">` +
            `<td>${row.row}</td>` +
            `<td>${row.sa}</td>` +
            `<td class="mono">${row.F}</td>` +
            `<td class="mono">${row.L}</td>` +
            `</tr>`;
    }
    html += "</table>";
    document.getElementById("suffixTable").innerHTML = html;
}

function dibujarTablaC(s) {
    let html = "<table><tr>";
    for (const entry of s.C) html += `<th>${entry.char}</th>`;
    html += "</tr><tr>";
    for (const entry of s.C) html += `<td>${entry.value}</td>`;
    html += "</tr></table>";
    document.getElementById("tableC").innerHTML = html;
}

function dibujarWaveletTree(s) {
    const wt = s.wavelet;
    const alpha = wt.alphabet;
    let html = "";
    for (const node of wt.nodes) {
        const chars = alpha.slice(node.alpha_lo, node.alpha_hi);
        let bitsHtml = "";
        for (const b of node.bits) bitsHtml += `<span class="wt-bit-${b}">${b}</span>`;
        const hijos = [];
        if (node.left  !== -1) hijos.push(`izq→nodo ${node.left}`);
        if (node.right !== -1) hijos.push(`der→nodo ${node.right}`);
        const hijosTxt = hijos.length ? hijos.join(" · ") : "(hojas)";
        html += `<div class="wt-node">` +
            `<div class="wt-range">Nodo ${node.id} · {${chars.split("").join(",")}}</div>` +
            `<div class="wt-bits">${bitsHtml}</div>` +
            `<div class="wt-children">${hijosTxt}</div>` +
            `</div>`;
    }
    document.getElementById("waveletTree").innerHTML = html;
}

// --- util ---
function mostrarStatus(msg, esError) {
    statusEl.textContent = msg;
    statusEl.classList.toggle("error", esError);
}