/**
 * ĐỒ ÁN MẠNG CẢM BIẾN - KIỂM TRA ĐỘC LẬP MIC I2S INMP441
 * 
 * Sketch này được thiết kế tối giản để kiểm tra riêng tín hiệu Microphone I2S INMP441.
 * Không sử dụng LCD, Relay hay RGB LED nhằm loại bỏ mọi tác nhân gây lỗi khác.
 * 
 * Sơ đồ đấu nối Mic:
 * - VDD --> Chân 3V3 trực tiếp trên ESP32 (Không cắm 5V!)
 * - GND --> GND
 * - L/R --> GND (Kênh Trái)
 * - WS  --> GPIO 26
 * - SCK --> GPIO 25
 * - SD  --> GPIO 33
 * 
 * Hướng dẫn kiểm tra:
 * 1. Nạp code này vào ESP32.
 * 2. Mở Tools -> Serial Plotter (Ctrl+Shift+L) trong Arduino IDE.
 * 3. Thiết lập tốc độ truyền baud là 115200.
 * 4. Bạn sẽ thấy đồ thị dạng sóng âm chạy liên tục. Thử nói, thổi nhẹ hoặc gõ nhẹ vào mic
 *    để kiểm tra xem sóng âm có dao động lớn lên không.
 */

#include <driver/i2s.h>

// Định cấu hình chân kết nối I2S đúng theo sơ đồ của bạn
#define I2S_WS   26
#define I2S_SCK  25
#define I2S_SD   33
#define I2S_PORT I2S_NUM_0

// Cấu hình tần số lấy mẫu (16kHz chuẩn cho TinyML)
#define SAMPLE_RATE 16000

void init_i2s() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,   // INMP441 xuất dữ liệu 24-bit đặt trong khung 32-bit
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,    // Lấy kênh trái (do L/R nối đất)
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,                              // Kích thước buffer DMA nhỏ giúp phản hồi thời gian thực
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE, // Không dùng chân TX phát dữ liệu
    .data_in_num = I2S_SD
  };

  // Cài đặt driver và gán chân cho ESP32
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== KIEM TRA DOC LAP MIC INMP441 ===");
  
  // Khởi tạo I2S
  init_i2s();
  Serial.println("Mic I2S khoi dong thanh cong! Dang doc du lieu...");
}

void loop() {
  int32_t raw_samples[64];
  size_t bytes_read = 0;
  
  // Đọc dữ liệu thô từ I2S DMA buffer
  esp_err_t err = i2s_read(I2S_PORT, &raw_samples, sizeof(raw_samples), &bytes_read, portMAX_DELAY);
  
  if (err == ESP_OK && bytes_read > 0) {
    int sample_count = bytes_read / sizeof(int32_t);
    
    for (int i = 0; i < sample_count; i++) {
      // Đầu ra INMP441 có độ phân giải 24-bit được căn lề trái trong khung 32-bit.
      // Ta dịch phải 14 bit để đưa về giá trị 18-bit có dấu chuẩn để hiển thị rõ ràng trên đồ thị.
      int32_t processed_sample = raw_samples[i] >> 14;
      
      // In ra theo định dạng Serial Plotter nhận diện
      // Cố định giới hạn trên/dưới (-3000, 3000) để biểu đồ không bị co giãn tự động quá mức
      Serial.print("LimitMin:-3000,LimitMax:3000,Waveform:");
      Serial.println(processed_sample);
    }
  }
}
