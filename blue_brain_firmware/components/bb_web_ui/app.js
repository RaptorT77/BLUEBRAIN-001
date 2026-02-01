function showTab(tabId, el) {
    document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
    document.querySelectorAll('.nav-links a').forEach(a => a.classList.remove('active'));

    document.getElementById(tabId).classList.add('active');
    if (el) el.classList.add('active');

    if (tabId === 'config') loadConfig();
}

// Polling Loop
console.log("BlueBrain UI Started");
setInterval(fetchMetrics, 2000);

async function fetchMetrics() {
    try {
        const response = await fetch('/api/v1/status');
        if (!response.ok) throw new Error("Network response was not ok");
        const data = await response.json();

        // Update UI
        document.getElementById('valRms').innerText = data.rms.toFixed(3);
        document.getElementById('valPeak').innerText = data.peak.toFixed(3);
        document.getElementById('valCrest').innerText = data.crest.toFixed(2);
        document.getElementById('valTemp').innerText = data.temp.toFixed(1) + " °C";

        // AI Result logic
        const aiText = document.getElementById('aiResult');
        const aiBar = document.getElementById('confidenceFill');
        const aiConf = document.getElementById('confidenceText');

        // Assuming data.ai_class is 0 (Sano), 1 (Desbalance), 2 (Rodamiento)
        let status = "Desconocido";
        let color = "#555";

        switch (parseInt(data.ai_class)) {
            case 0: status = "🟢 SANO"; color = "#00E676"; break;
            case 1: status = "⚠️ DESBALANCEO"; color = "#FFC107"; break;
            case 2: status = "🛑 FALLA RODAMIENTO"; color = "#FF5252"; break;
        }

        aiText.innerText = status;
        aiBar.style.width = (data.ai_conf * 100) + "%";
        aiBar.style.backgroundColor = color;
        aiConf.innerText = Math.round(data.ai_conf * 100) + "% Confianza";

        document.getElementById('connStatus').innerText = "Conectado";
        document.getElementById('connStatus').style.background = "#00E676";
    } catch (e) {
        document.getElementById('connStatus').innerText = "Desconectado";
        document.getElementById('connStatus').style.background = "#FF5252";
    }
}

async function loadConfig() {
    const res = await fetch('/api/v1/config');
    const cfg = await res.json();

    document.getElementById('cfg_wifi_ssid').value = cfg.wifi_ssid;
    document.getElementById('cfg_wifi_pass').value = "******"; // Don't show password or handle carefully
    document.getElementById('cfg_mqtt_uri').value = cfg.mqtt_uri;
    document.getElementById('cfg_mqtt_port').value = cfg.mqtt_port;
    document.getElementById('cfg_mqtt_user').value = cfg.mqtt_user;
    document.getElementById('cfg_mqtt_topic').value = cfg.mqtt_topic;

    document.getElementById('cfg_sample_rate').value = cfg.sample_rate;
    document.getElementById('cfg_n_samples').value = cfg.n_samples;
    document.getElementById('cfg_espnow_en').checked = cfg.espnow_en;
}

document.getElementById('configForm').addEventListener('submit', async (e) => {
    e.preventDefault();
    const data = {
        wifi_ssid: document.getElementById('cfg_wifi_ssid').value,
        wifi_pass: document.getElementById('cfg_wifi_pass').value,
        mqtt_uri: document.getElementById('cfg_mqtt_uri').value,
        mqtt_port: parseInt(document.getElementById('cfg_mqtt_port').value),
        mqtt_user: document.getElementById('cfg_mqtt_user').value,
        mqtt_topic: document.getElementById('cfg_mqtt_topic').value,
        sample_rate: parseInt(document.getElementById('cfg_sample_rate').value),
        n_samples: parseInt(document.getElementById('cfg_n_samples').value),
        espnow_en: document.getElementById('cfg_espnow_en').checked
    };

    // Simple password check: if still mask, don't send or send empty?
    if (data.wifi_pass === "******") delete data.wifi_pass;

    await fetch('/api/v1/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    });
    alert("Configuración Guardada. Reinicie el dispositivo para aplicar.");
});
