# 🌊 Flujo de Datos Blue Brain: De Sensor a Terminal

Este documento detalla el viaje de un dato desde que es vibración física hasta que aparece como texto en tu terminal.

---

## 1. 📡 Fase de Adquisición (El "Bunker" - Core 1)

**Objetivo:** Capturar una "foto" instantánea de la vibración durante 1 segundo exacto.

*   **Función Clave:** `bb_sensors_read_accel_burst()`
*   **Variable Búfer:** `uint8_t *raw_data` (6 KB en Heap).
*   **Sensor:** MPU6050 (Dirección I2C `0x68`).

**Proceso Paso a Paso:**
1.  **Inicio:** `Task_Vibration_Analysis` despierta.
2.  **Bucle de Captura:** Se ejecuta 1024 veces (`SAMPLE_COUNT`).
    *   Lee 6 bytes del MPU6050 (Registros `0x3B` a `0x40`: Accel X, Y, Z).
    *   **Tiempo entre muestras:** El driver espera ~1000 microsegundos entre lecturas.
    *   **Frecuencia Resultante (Fs):** ~1000 Hz (1000 muestras/segundo).
3.  **Resultado:** Un bloque de memoria cruda con 1024 lecturas de aceleración en 3 ejes.

---

## 2. 🧠 Fase de Procesamiento DSP (El "Cerebro" - Core 1)

**Objetivo:** Convertir voltajes crudos en información útil (G-Force y Frecuencia).

*   **Función Clave:** `bb_dsp_ai_process_vibration()`
*   **Entrada:** `raw_data` (Bytes).
*   **Salida:** Estructura `bb_telemetry_t`.

### A. Conversión y Normalización
Los datos crudos (enteros de 16-bit) se convierten a gravedad (G).
*   **Variable:** `accel_x` (float).
*   **Fórmula:** `Raw / 2048.0` (Para rango +/- 16G).
    *   *Ejemplo:* Si el sensor lee `2048`, eso es `1.0 G` (Gravedad terrestre).

### B. Análisis Temporal (Time Domain)
Antes de la FFT, calculamos estadísticas básicas sobre la señal en el tiempo:
1.  **RMS (Root Mean Square):** `sqrt(sum(x^2)/N)`. Representa la **energía total** de la vibración. Importante para ver "cuánto vibra" en general.
2.  **Peak (Pico):** El valor absoluto máximo alcanzado.
3.  **Crest Factor (CF):** `Peak / RMS`.
    *   Identifica golpes o impactos. Si CF > 3, hay golpeteo (bearing faults). Si CF ~ 1.41, es vibración sinusoidal pura (desbalance).

### C. Análisis Espectral (Frequency Domain - FFT)
Aquí ocurre la magia matemática para ver "a qué velocidad" vibra.
1.  **Ventana de Hann:** Se multiplica la señal por una curva de campana para suavizar los bordes y evitar errores en la FFT ("Spectral Leakage").
2.  **FFT (Radix-2):** Transforma 1024 muestras de tiempo en 512 puntos de frecuencia.
3.  **Magnitud:** Convierte los números complejos de la FFT en valores reales (Amplitud).

### D. Extracción de Características (AI Ligera)
1.  **Frecuencia Dominante (`dom_freq`):** Buscamos cuál de los 512 "bins" de frecuencia tiene la magnitud más alta.
    *   *Resolución:* `1000 Hz / 1024 muestras = ~0.97 Hz` por paso.
2.  **Bandas de Energía:** Sumamos la energía en zonas específicas:
    *   **Band LO (<100Hz):** Problemas estructurales, desbalance, soltura mecánica.
    *   **Band HI (>100Hz):** Defectos en rodamientos, engranajes, lubricación.

---

## 3. 🖥️ Interpretación de Salida (Terminal / MQTT)

Cuando ves esto en la terminal:

```text
I (150703) BLUE_BRAIN_MAIN: RMS: 1.042 G | Peak: 1.055 G | CF: 1.01
I (150709) BLUE_BRAIN_MAIN: Temp: 17.31 C
I (150714) bb_connect: Telemetría publicada: {"rms":1.042,"peak":1.055, ... "dom_freq":1.0}
```

**Esto es lo que significa:**

*   **RMS: 1.042 G:**
    *   La máquina está "quieta" pero siente la gravedad de la tierra (1.0G).
    *   **Vibración real:** `1.042 - 1.000 = 0.042 G` de ruido/vibración. Es muy bajo (saludable).
*   **Peak: 1.055 G:**
    *   El tirón máximo fue de 1.055 G. Casi igual al promedio.
*   **CF: 1.01:**
    *   Muy cercano a 1.0. Significa que la señal es muy plana/constante. No hay golpes ni "martillazos" internos.
*   **Temp: 17.31 C:** Temperatura del gabinete/motor.
*   **dom_freq: 1.0 Hz:**
    *   Probablemente ruido DC (0-1 Hz) porque el sensor está estático. Si el motor girara a 1800 RPM, verías `30.0 Hz`.

---

## 🔄 Resumen del Ciclo

| Etapa | Quién | Tiempo | Variable Clave |
| :--- | :--- | :--- | :--- |
| **Captura** | `bb_sensors` | 1024 ms | `raw_data[]` |
| **Cálculo** | `bb_dsp_ai` | ~50 ms | `fft_input[]` |
| **Reporte** | `main` | <10 ms | `bb_telemetry_t` |
| **Envío** | `bb_connect` | Async | JSON MQTT |
| **Espera** | `vTaskDelay` | 5000 ms | - |

El sistema pasa **1 segundo escuchando** atentamente y **5 segundos descansando/enviando**, repitiendo el ciclo infinitamente.
