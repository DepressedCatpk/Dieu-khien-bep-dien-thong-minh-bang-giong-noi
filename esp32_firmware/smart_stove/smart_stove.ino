/**
 * ĐỒ ÁN MẠNG CẢM BIẾN - ĐIỀU KHIỂN BẾP ĐIỆN THÔNG MINH BẰNG GIỌNG NÓI
 * 
 * Main File: smart_stove.ino
 * File điều phối trung tâm (App Coordinator) của hệ thống.
 * 
 * Cấu trúc module dự án:
 * - config.h: Cấu hình chân kết nối, hằng số và khai báo các trạng thái.
 * - audio_handler.h/.cpp: Xử lý I2S đọc mic INMP441 chạy ngầm trên Core 0, quản lý Ring Buffer.
 * - peripherals.h/.cpp: Điều khiển LCD1602 I2C, LED RGB, Relay và chuyển đổi các trạng thái.
 */

#include "config.h"

#if EDGE_IMPULSE_LIBRARY_INSTALLED
  // Undefine the dummy macros from config.h so the Edge Impulse SDK can define them
  #undef EI_CLASSIFIER_RAW_SAMPLE_COUNT
  #undef EI_CLASSIFIER_FREQUENCY
  #include <_i_u_khi_n_b_p_i_n_th_ng_minh_b_ng_gi_ng_n_i_inferencing.h>
#endif
#include "audio_handler.h"
#include "peripherals.h"

// =========================================================================
// KHỞI TẠO BIẾN TOÀN CỤC (ĐÃ KHAI BÁO EXTERN TRONG CONFIG.H)
// =========================================================================
SystemState currentState = STATE_SLEEP;
unsigned long stateTimer = 0;
int countdownSeconds = 0;
int initialCountdownSeconds = 0; // Định nghĩa biến lưu thời gian hẹn giờ ban đầu
unsigned long lastTimerTick = 0;
int timerMinutes = 0;

// =========================================================================
// SETUP & LOOP CHÍNH
// =========================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("===========================================");
  Serial.println("Smart Stove System - Multi-module Booting...");
  Serial.println("===========================================");

  // 1. Khởi tạo các ngoại vi (GPIO, LCD1602)
  init_peripherals();
  
  // 2. Khởi tạo hệ thống thu âm đa nhân (I2S chạy trên Core 0)
  init_audio();
  
  // 3. Hiển thị thông báo chào mừng khởi động trên LCD
  lcd.setCursor(0, 0);
  lcd.print(" BEP DIEN THONG MINH");
  lcd.setCursor(0, 1);
  lcd.print("====================");
  lcd.setCursor(0, 2);
  lcd.print("   System Booting   ");
  lcd.setCursor(0, 3);
  lcd.print("   Please wait...   ");
  delay(2000);

  // Đưa hệ thống vào trạng thái ngủ mặc định
  changeState(STATE_SLEEP);
}

void loop() {
  unsigned long timeNow = millis();

  // --- 1. NHẤP NHÁY ĐÈN BẾP (RELAY) VÀ LED RGB KHI ĐANG STANDBY ---
  if (currentState == STATE_STANDBY) {
    static unsigned long lastFlashStove = 0;
    static bool stoveFlashState = false;
    if (timeNow - lastFlashStove >= 500) { 
      lastFlashStove = timeNow;
      stoveFlashState = !stoveFlashState;
      
      // Nhấp nháy đèn bếp (Relay)
      digitalWrite(PIN_RELAY, stoveFlashState ? HIGH : LOW);
      
      // Nhấp nháy LED RGB (Màu trắng)
      if (stoveFlashState) {
        setRGB(true, true, true); // Sáng trắng
      } else {
        setRGB(false, false, false); // Tắt LED
      }
    }
  }

  // =========================================================================
  // 2. XỬ LÝ NHẬN DIỆN TINYML VỚI LỊCH PHÂN TÍCH (MỖI 500MS)
  // =========================================================================
  static unsigned long lastInferenceTime = 0;
  static unsigned long lastCommandTime = 0; // Chống nhận diện lặp lại từ khóa (Speech Debounce)
  
  if (timeNow - lastInferenceTime >= 500) { 
    lastInferenceTime = timeNow;
    
    const char* detected_label = "noise";
    float highest_score = 0.0;
    
#if EDGE_IMPULSE_LIBRARY_INSTALLED
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &get_audio_data;
    
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, false);
    
    if (r == EI_IMPULSE_OK) {
      for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (result.classification[ix].value > highest_score) {
          highest_score = result.classification[ix].value;
          detected_label = result.classification[ix].label;
        }
      }
    }
#else
    // Giả lập nhận diện từ cổng Serial:
    // 'k' -> nguyen_khoi, 'l' -> bep_lua, 'h' -> hen_gio, 't' -> tat_bep
    if (Serial.available() > 0) {
      char c = Serial.read();
      highest_score = 0.95;
      if (c == 'k') detected_label = "nguyen_khoi";
      else if (c == 'l') detected_label = "bep_lua";
      else if (c == 'h') detected_label = "hen_gio";
      else if (c == 't') detected_label = "tat_bep";
      
      while(Serial.available() > 0) Serial.read();
    }
#endif

    // Ngưỡng nhận diện động: 0.65 cho từ khóa đánh thức "nguyen_khoi" để nhạy hơn,
    // các lệnh điều khiển khác giữ nguyên 0.75 để tránh nhận nhầm nguy hiểm.
    float threshold = 0.75;
    // BUG FIX: Kiểm tra cả nhãn có dấu gạch dưới lẫn dấu cách (tùy phiên bản Edge Impulse)
    if (strcmp(detected_label, "nguyen_khoi") == 0 || strcmp(detected_label, "nguyen khoi") == 0) {
      threshold = 0.65;
    }

    // Khóa chống dội giọng nói: Chỉ nhận diện lệnh mới cách nhau ít nhất 1.5 giây
    if (highest_score > threshold && strcmp(detected_label, "noise") != 0 && (timeNow - lastCommandTime >= 1500)) {
      lastCommandTime = timeNow;
      
      Serial.print(">>> Nhan dien: ");
      Serial.print(detected_label);
      Serial.print(" | Acc: ");
      Serial.println(highest_score);
      
      // 1. Lệnh: Tắt bếp (Luôn luôn được ưu tiên xử lý ở mọi trạng thái để đảm bảo an toàn)
      if (strcmp(detected_label, "tat_bep") == 0 || strcmp(detected_label, "tat bep") == 0) {
        changeState(STATE_SLEEP);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" BEP DIEN THONG MINH");
        lcd.setCursor(0, 1);
        lcd.print("====================");
        lcd.setCursor(0, 2);
        lcd.print("   Bep Da Tat!      ");
        lcd.setCursor(0, 3);
        lcd.print("   Cam on!          ");
        delay(2000);
        changeState(STATE_SLEEP);
      }
      // 2. Các lệnh khác chỉ xử lý nếu bếp KHÔNG ở trạng thái đang đếm ngược hẹn giờ (STATE_TIMER)
      else if (currentState != STATE_TIMER) {
        // Lệnh: Đánh thức "nguyên khôi"
        if (strcmp(detected_label, "nguyen_khoi") == 0 || strcmp(detected_label, "nguyen khoi") == 0) {
          changeState(STATE_STANDBY);
          // BUG FIX: changeState(STATE_STANDBY) có delay(1500ms) bên trong.
          // Sau khi hàm trả về, ~1500ms đã trôi qua khiến debounce (1500ms) vừa hết hạn,
          // dẫn đến "nguyen khoi" bị nhận diện lại NGAY LẬP TỨC lần 2.
          // Lần gọi thứ 2 có prevState=STANDBY → nhánh else → digitalWrite(RELAY, LOW) → BẾP TẮT!
          // => Reset lastCommandTime NGAY SAU khi changeState trả về để bắt đầu lại debounce.
          lastCommandTime = millis();
        }
        // Lệnh: "bếp lửa" (bật nấu) - Chỉ nhận khi đang chờ STANDBY
        else if (strcmp(detected_label, "bep_lua") == 0 || strcmp(detected_label, "bep lua") == 0) {
          if (currentState == STATE_STANDBY) {
            changeState(STATE_HEATING);
          } else {
            Serial.println("[Chu y] Bep can o trang thai STANDBY de nhan lenh bep lua.");
          }
        }
        // Lệnh: "hẹn giờ" (tăng thời gian) - Nhận khi STANDBY hoặc HEATING hoặc TIMER_CONFIG
        else if (strcmp(detected_label, "hen_gio") == 0 || strcmp(detected_label, "hen gio") == 0) {
          if (currentState == STATE_STANDBY || currentState == STATE_HEATING || currentState == STATE_TIMER_CONFIG) {
            if (currentState != STATE_TIMER_CONFIG) {
              timerMinutes = 5;
              changeState(STATE_TIMER_CONFIG);
            } else {
              timerMinutes += 5;
              if (timerMinutes > 30) timerMinutes = 5; 
              changeState(STATE_TIMER_CONFIG);
            }
          } else {
            Serial.println("[Chu y] Bep can o trang thai STANDBY hoac HEATING de hen gio.");
          }
        }
      } else {
        Serial.println("[Chu y] Bep dang trong che do hen gio co chot. Bo qua cac lenh khac ngoai 'tat bep'.");
      }
    }
  }

  // =========================================================================
  // 3. XỬ LÝ TIMEOUT VÀ ĐẾM NGƯỢC CỦA CÁC TRẠNG THÁI
  // =========================================================================
  
  // A. Xử lý trong cài đặt giờ (STATE_TIMER_CONFIG) - Nhấp nháy LED theo số phút và chốt lệnh sau 3 giây
  if (currentState == STATE_TIMER_CONFIG) {
    if (timerMinutes == 5) {
      setRGB(true, true, false); // Vàng
    } else if (timerMinutes == 10) {
      setRGB(true, true, false); 
    } else if (timerMinutes == 15) {
      setRGB(true, false, false); // Đỏ
    } else {
      setRGB(true, false, true);  // Tím
    }

    if (millis() - stateTimer > 3000) {
      changeState(STATE_TIMER);
    }
  }

  // B. Timeout cho chế độ STANDBY - tự động ngủ sau 10 giây nếu không nhận lệnh
  if (currentState == STATE_STANDBY) {
    if (millis() - stateTimer > 10000) { 
      Serial.println(">>> Het 10s cho lenh! Tu dong ve che do SLEEP.");
      changeState(STATE_SLEEP);
    }
  }

  // C. Xử lý đếm ngược chế độ hẹn giờ (STATE_TIMER)
  if (currentState == STATE_TIMER) {
    if (millis() - lastTimerTick >= 1000) {
      lastTimerTick = millis();
      countdownSeconds--;
      
      // Cập nhật hiển thị thời gian và thanh tiến trình co lại thời gian thực
      updateTimerDisplay();
      
      Serial.print("-> Dang hen gio, con lai: ");
      Serial.print(countdownSeconds);
      Serial.println(" giay.");
      
      if (countdownSeconds <= 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" BEP DIEN THONG MINH");
        lcd.setCursor(0, 1);
        lcd.print("====================");
        lcd.setCursor(0, 2);
        lcd.print("    HET GIO NAU!    ");
        lcd.setCursor(0, 3);
        lcd.print("  TU DONG TAT BEP   ");
        digitalWrite(PIN_RELAY, LOW);
        setRGB(false, false, false);
        delay(3000);
        changeState(STATE_SLEEP); // Tắt hẳn bếp để phòng ngừa an toàn cháy nổ
      }
    }
  }
}
