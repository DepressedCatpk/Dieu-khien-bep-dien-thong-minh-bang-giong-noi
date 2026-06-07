#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =========================================================================
// CẤU HÌNH LIÊN KẾT THƯ VIỆN EDGE IMPULSE
// =========================================================================
// SỬ DỤNG: 
// 1. Huấn luyện mô hình trên Edge Impulse xong, chọn "Arduino library" -> "Build".
// 2. Nhập file ZIP vào Arduino IDE qua Sketch -> Include Library -> Add .ZIP Library.
// 3. Thay đổi giá trị dưới đây thành 1 và đổi tên include cho đúng tên thư mục thư viện của bạn.
#define EDGE_IMPULSE_LIBRARY_INSTALLED 1

// --- DINH NGHIA MACRO MAC DINH CHO TIEN XU LY AUDIO ---
#define EI_CLASSIFIER_RAW_SAMPLE_COUNT 24000 
#define EI_CLASSIFIER_FREQUENCY 16000

#if !EDGE_IMPULSE_LIBRARY_INSTALLED
  // --- DINH NGHIA DUMMY KHI CHUA CO THU VIEN ---
  typedef struct {
      float value;
      const char* label;
  } ei_classifier_output_t;
  typedef struct {
      ei_classifier_output_t classification[5];
  } ei_impulse_result_t;
  typedef enum {
      EI_IMPULSE_OK = 0
  } EI_IMPULSE_ERROR;
#endif

// =========================================================================
// CẤU HÌNH HẸN GIỜ DEMO (DEMO MODE CONFIGURATION)
// =========================================================================
// true: 1 phút hiển thị = 1 giây thực tế (để demo nhanh).
// false: 1 phút hiển thị = 60 giây thực tế (chạy thực tế).
#define DEMO_FAST_TIMER true

// =========================================================================
// ĐỊNH NGHĨA CHÂN NGOẠI VI CHÍNH THỨC (PINOUT)
// =========================================================================
#define PIN_RELAY    27
#define PIN_LED_R    13
#define PIN_LED_G    12
#define PIN_LED_B    14

#define RGB_COMMON_ANODE false // Đặt false cho LED Cathode chung

#define I2S_WS       26
#define I2S_SCK      25
#define I2S_SD       33
#define I2S_PORT     I2S_NUM_0

#define LCD_I2C_ADDR 0x27

// =========================================================================
// ĐỊNH NGHĨA MÁY TRẠNG THÁI (STATE MACHINE)
// =========================================================================
enum SystemState {
  STATE_SLEEP,        // Bếp tắt hoàn toàn (ngủ), chờ từ khóa đánh thức "nguyên khôi"
  STATE_STANDBY,      // Trạng thái chờ sau khi thức dậy (Relay nhấp nháy, RGB sáng trắng, chờ lệnh 10s)
  STATE_TIMER_CONFIG, // Đang đếm số lần nói "hẹn giờ" để tăng thời gian (3s chốt lệnh)
  STATE_HEATING,      // Bếp đang nấu (Relay ON liên tục), LED màu đỏ, LCD báo nấu
  STATE_TIMER         // Đang đếm ngược hẹn giờ tắt bếp, hiển thị trên LCD
};

// Khai báo extern cho các biến toàn cục dùng chung ở các module
extern SystemState currentState;
extern unsigned long stateTimer;
extern int countdownSeconds;
extern int initialCountdownSeconds; // Biến lưu thời gian hẹn giờ ban đầu
extern unsigned long lastTimerTick;
extern int timerMinutes;

#endif
