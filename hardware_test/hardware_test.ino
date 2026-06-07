/**
 * ĐỒ ÁN MẠNG CẢM BIẾN - ĐIỀU KHIỂN BẾP ĐIỆN THÔNG MINH BẰNG GIỌNG NÓI
 * 
 * Hardware Test Sketch (Cấu hình LCD 2004 và đã bỏ Nút nhấn khẩn cấp):
 * 1. LCD2004 + I2C (SDA=21, SCL=22, Nguồn 5V, địa chỉ 0x3F)
 * 2. RGB LED (Common Cathode: GND, R=13, G=12, B=14)
 * 3. Relay 5V + Đèn LED (IN=27, Nguồn 5V)
 * 4. Mic INMP441 I2S (VDD=3.3V, GND=GND, L/R=GND, WS=26, SCK=25, SD=33)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <driver/i2s.h>

// ==========================================
// CẤU HÌNH CÁC CHÂN KẾT NỐI CHÍNH THỨC (PINOUT)
// ==========================================

// 1. Chân điều khiển Relay
#define PIN_RELAY 27

// 2. Chân điều khiển RGB LED (Common Cathode - Cực âm chung)
#define PIN_LED_R 13
#define PIN_LED_G 12
#define PIN_LED_B 14
#define RGB_COMMON_ANODE false 

// 3. Chân kết nối Mic INMP441 (I2S Pinout)
#define I2S_WS 26
#define I2S_SCK 25
#define I2S_SD 33
#define I2S_PORT I2S_NUM_0

// 4. Địa chỉ I2C của LCD2004 (20 cột, 4 hàng)
#define LCD_I2C_ADDR 0x27
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 20, 4);

// ==========================================
// KHỞI TẠO BỘ ĐỌC MIC I2S (I2S INITIALIZATION)
// ==========================================
void init_i2s() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
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

// ==========================================
// ĐIỀU KHIỂN LED RGB (RGB LED CONTROL)
// ==========================================
void setRGBColor(bool r, bool g, bool b) {
  if (RGB_COMMON_ANODE) {
    digitalWrite(PIN_LED_R, !r);
    digitalWrite(PIN_LED_G, !g);
    digitalWrite(PIN_LED_B, !b);
  } else {
    digitalWrite(PIN_LED_R, r);
    digitalWrite(PIN_LED_G, g);
    digitalWrite(PIN_LED_B, b);
  }
}

// ==========================================
// SETUP & LOOP
// ==========================================
unsigned long last_toggle_time = 0;
bool relay_state = false;
int color_state = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("===========================================");
  Serial.println("ESP32 Smart Stove - LCD 2004 Hardware Test");
  Serial.println("===========================================");

  // 1. Khởi tạo chân GPIO
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  
  digitalWrite(PIN_RELAY, LOW); 
  setRGBColor(false, false, false);

  // 2. Khởi tạo LCD2004
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print(" BEP DIEN THONG MINH");
  lcd.setCursor(0, 1);
  lcd.print("====================");
  lcd.setCursor(0, 2);
  lcd.print(" ESP32 Pin Testing  ");
  lcd.setCursor(0, 3);
  lcd.print("   Please wait...   ");
  delay(2000);

  // 3. Khởi tạo Mic I2S
  Serial.println("Dang khoi tao mic I2S INMP441...");
  init_i2s();
  Serial.println("Mic khoi tao thanh cong!");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" BEP DIEN THONG MINH");
  lcd.setCursor(0, 1);
  lcd.print("====================");
  lcd.setCursor(0, 2);
  lcd.print("Hardware Test Active");
  lcd.setCursor(0, 3);
  lcd.print("Speak into Mic...   ");
}

void loop() {
  // --- ĐỌC DỮ LIỆU TỪ MIC INMP441 ---
  int32_t raw_samples[64];
  size_t bytes_read = 0;
  esp_err_t err = i2s_read(I2S_PORT, &raw_samples, sizeof(raw_samples), &bytes_read, portMAX_DELAY);
  
  float average_amplitude = 0;
  if (err == ESP_OK && bytes_read > 0) {
    int sample_count = bytes_read / sizeof(int32_t);
    float sum = 0;
    for (int i = 0; i < sample_count; i++) {
      int32_t val = raw_samples[i] >> 14; 
      sum += abs(val);
    }
    average_amplitude = sum / sample_count;
  }

  // --- ĐIỀU KHIỂN LCD, RGB LED VÀ RELAY CHU KỲ (MỖI 3 GIÂY) ---
  unsigned long current_time = millis();
  if (current_time - last_toggle_time >= 3000) {
    last_toggle_time = current_time;
    
    relay_state = !relay_state;
    digitalWrite(PIN_RELAY, relay_state ? HIGH : LOW);
    
    color_state = (color_state + 1) % 3;
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" BEP DIEN THONG MINH");
    lcd.setCursor(0, 1);
    lcd.print("====================");
    
    lcd.setCursor(0, 2);
    lcd.print("Relay: ");
    lcd.print(relay_state ? "ON  (LED ON) " : "OFF (LED OFF)");
    
    lcd.setCursor(0, 3);
    if (color_state == 0) {
      setRGBColor(true, false, false);
      lcd.print("RGB: RED            ");
    } else if (color_state == 1) {
      setRGBColor(false, true, false);
      lcd.print("RGB: GREEN          ");
    } else {
      setRGBColor(false, false, true);
      lcd.print("RGB: BLUE           ");
    }
    
    Serial.print("--- Relay = ");
    Serial.print(relay_state ? "BAT" : "TAT");
    Serial.print(" | LED RGB = ");
    Serial.print(color_state == 0 ? "DO" : (color_state == 1 ? "XANH LA" : "XANH DUONG"));
    Serial.print(" | Mic AvgAmp = ");
    Serial.println(average_amplitude);
  }
}
