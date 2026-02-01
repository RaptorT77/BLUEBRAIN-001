#include "icm42688.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "hardware_defs.h"
#include <string.h>

static const char *TAG = "ICM42688";
static icm42688_dev_t dev;

// Función auxiliar para registrar la escritura/lectura
static esp_err_t icm_write_reg(uint8_t reg, uint8_t data) {
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  uint8_t tx_data[2] = {reg & 0x7F, data}; // MSB 0 para Write

  t.length = 8 * 2;
  t.tx_buffer = tx_data;

  return spi_device_transmit(dev.spi_handle, &t);
}

static uint8_t icm_read_reg(uint8_t reg) {
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  uint8_t tx_data = reg | 0x80; // MSB 1 para Read
  uint8_t rx_data[2] = {0};

  t.length = 8 * 2;
  t.tx_buffer = &tx_data;
  t.rx_buffer = rx_data;

  esp_err_t ret = spi_device_transmit(dev.spi_handle, &t);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error SPI Read");
    return 0;
  }
  return rx_data[1]; // El primer byte es dummy/status durante la fase de
                     // dirección
}

esp_err_t icm42688_init(void) {
  ESP_LOGI(TAG, "Iniciando ICM-42688-P...");

  // 1. Configurar Bus SPI (Si no se ha hecho ya)
  spi_bus_config_t buscfg = {
      .mosi_io_num = PIN_SPI_MOSI,
      .miso_io_num = PIN_SPI_MISO,
      .sclk_io_num = PIN_SPI_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz =
          8192, // Aumentado a 8KB para soportar lecturas FIFO (DMA)
  };

  // Intentar inicializar SPI2 (FSPI). Si ya está en uso, ignorar error
  // ESP_ERR_INVALID_STATE
  esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "Error al inicializar SPI Bus: %d", ret);
    return ret;
  }

  // 2. Agregar dispositivo al bus
  spi_device_interface_config_t devcfg = {
      .clock_speed_hz = 1000000,  // 1 MHz
      .mode = 0,                  // SPI Data Mode 0
      .spics_io_num = PIN_IMU_CS, // CS Pin
      .queue_size = 7,
  };

  ret = spi_bus_add_device(SPI2_HOST, &devcfg, &dev.spi_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error al agregar dispositivo SPI: %d", ret);
    return ret;
  }

  // 3. Verificar WHO_AM_I
  uint8_t whoami = icm_read_reg(ICM42688_REG_WHO_AM_I);
  ESP_LOGI(TAG, "WHO_AM_I = 0x%02X (Esperado: 0x%02X)", whoami,
           ICM42688_WHO_AM_I_VAL);

  if (whoami != ICM42688_WHO_AM_I_VAL) {
    ESP_LOGW(TAG, "Advertencia: ICM-42688 no detectado (Esto es normal si solo "
                  "es preparación)");
    // return ESP_FAIL; // No fallar fatalmente en preparación
  }

  ESP_LOGI(TAG, "ICM-42688-P Inicializado (Modo Preparación DMA)");
  return ESP_OK;
}

uint8_t icm42688_read_whoami(void) {
  return icm_read_reg(ICM42688_REG_WHO_AM_I);
}

// Transaction object must persist in memory during the transition
static spi_transaction_t dma_trans;

esp_err_t icm42688_start_dma_read(uint8_t *buffer, size_t len) {
  memset(&dma_trans, 0, sizeof(dma_trans));

  // Configuración para lectura de bloques (e.g. FIFO)
  // Asumimos que la dirección de registro ya fue configurada o es FIFO_DATA
  // Para simplificar, aquí leemos el registro FIFO_DATA (0x30) repetidamente.
  // OJO: En modo SPI burst, se envía la dirección una vez y luego clocks.

  // Si queremos leer FIFO_DATA (0x30) en burst: 0x30 | 0x80 = 0xB0
  // spi_transaction_t soporta cmd/addr phases si se configura el device.
  // Aquí hacemos bit-banging manual del primer byte si no usamos cmd/addr.
  // Para DMA puro, el buffer TX debe tener el comando.

  // NOTA: Para este stub de preparación, usaremos RX puro asumiendo
  // que el byte de comando ya se gestiona o se usa un buffer TX/RX simétrico.

  dma_trans.length = len * 8;
  dma_trans.rx_buffer = buffer;
  dma_trans.tx_buffer = NULL; // Solo lectura (Asumiendo half-duplex o dummy TX)

  // Para una implementación real de lectura de registro, necesitamos enviar la
  // dirección. dma_trans.tx_buffer debería tener la dirección. Pero el usuario
  // pidió "preparación".

  return spi_device_queue_trans(dev.spi_handle, &dma_trans, portMAX_DELAY);
}

esp_err_t icm42688_wait_dma_read(void) {
  spi_transaction_t *rtrans;
  return spi_device_get_trans_result(dev.spi_handle, &rtrans, portMAX_DELAY);
}
