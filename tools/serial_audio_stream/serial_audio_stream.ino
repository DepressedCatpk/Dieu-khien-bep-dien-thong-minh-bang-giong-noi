/**
 * ĐỒ ÁN MẠNG CẢM BIẾN - THU ÂM CHẤT LƯỢNG CAO QUA SERIAL CHO TINYML
 * 
 * Sketch này cấu hình bộ xử lý I2S của ESP32 để đọc dữ liệu âm thanh số từ INMP441
 * với tần số 16000Hz, Mono, 16-bit PCM chất lượng cao (đọc 32-bit tránh rè rồi chuyển sang 16-bit)
 * và truyền trực tiếp qua cổng Serial với tốc độ cao (115200) lên máy tính.
 * 
 * Sơ đồ đấu nối Mic:
 * - VDD --> 3V3 của ESP32 (Không cắm 5V!)
 * - GND --> GND
 * - L/R --> GND (Chọn kênh Trái)
 * - WS  --> GPIO 26
 * - SCK --> GPIO 25
 * - SD  --> GPIO 33
 */

#include <driver/i2s.h>

#define I2S_WS   26
#define I2S_SCK  25
#define I2S_SD   33
#define I2S_PORT I2S_NUM_0

// Tần số lấy mẫu tiêu chuẩn cho AI Speech
#define SAMPLE_RATE 16000

void init_i2s() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,   // Đọc 32-bit slot để tương thích hoàn toàn với INMP441
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,    // Mono (Kênh Trái)
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
  // Bắt đầu truyền Serial ở tốc độ 921600 baud để đảm bảo truyền thời gian thực không bị mất gói
  Serial.begin(921600);
  
  // Khởi tạo I2S đọc mic
  init_i2s();
}

void loop() {
  int32_t raw_sample = 0;
  size_t bytes_read = 0;
  
  // Đọc liên tục 1 mẫu dữ liệu thô (32-bit) từ bộ đệm DMA của I2S
  esp_err_t err = i2s_read(I2S_PORT, &raw_sample, sizeof(raw_sample), &bytes_read, portMAX_DELAY);
  
  if (err == ESP_OK && bytes_read > 0) {
    // Tiền xử lý: Dịch phải 14 bit để lấy 16-bit PCM chất lượng cao
    // và ép kiểu về int16_t (16-bit có dấu)
    int16_t sample16 = (int16_t)(raw_sample >> 14);
    
    // Truyền trực tiếp 2 bytes (16-bit) thô lên cổng Serial máy tính
    Serial.write((uint8_t*)&sample16, sizeof(sample16));
  }
}
