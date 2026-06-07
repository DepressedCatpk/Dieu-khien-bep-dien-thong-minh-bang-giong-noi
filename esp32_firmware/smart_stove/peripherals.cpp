#include "peripherals.h"

// Dinh nghia doi tuong LCD 2004 (20 cot, 4 hang)
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 20, 4);

void init_peripherals() {
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  
  digitalWrite(PIN_RELAY, LOW);
  setRGB(false, false, false);

  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void setRGB(bool r, bool g, bool b) {
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

void changeState(SystemState newState) {
  SystemState prevState = currentState; // Lưu trạng thái trước đó
  currentState = newState;
  stateTimer = millis();
  
  lcd.clear();
  switch (currentState) {
    case STATE_SLEEP:
      digitalWrite(PIN_RELAY, LOW); 
      setRGB(false, false, false);   
      lcd.setCursor(0, 0);
      lcd.print(" BEP DIEN THONG MINH");
      lcd.setCursor(0, 1);
      lcd.print("====================");
      lcd.setCursor(0, 2);
      lcd.print(" Noi: 'NGUYEN KHOI' ");
      lcd.setCursor(0, 3);
      lcd.print("    de khoi dong    ");
      Serial.println(">>> Chuyen sang trang thai: SLEEP (Cho tu khoa danh thuc)");
      break;
      
    case STATE_STANDBY:
      // --- HIỆU ỨNG KHỞI ĐỘNG (Chỉ chạy khi đánh thức từ SLEEP) ---
      if (prevState == STATE_SLEEP) {
        digitalWrite(PIN_RELAY, HIGH); 
        setRGB(true, true, true);      // Sáng Trắng
        lcd.setCursor(0, 0);
        lcd.print(" BEP DIEN THONG MINH");
        lcd.setCursor(0, 1);
        lcd.print("====================");
        lcd.setCursor(0, 2);
        lcd.print("   [ WAKING UP ]    ");
        lcd.setCursor(0, 3);
        lcd.print(" Xin chao chu nhan! ");
        delay(1500);                   
      } else {
        // Nếu chuyển từ trạng thái khác sang (như hết giờ nấu), tắt bếp nấu ngay lập tức
        digitalWrite(PIN_RELAY, LOW);
      }
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" BEP DIEN THONG MINH");
      lcd.setCursor(0, 1);
      lcd.print("====================");
      lcd.setCursor(0, 2);
      lcd.print("   [ STANDING BY ]  ");
      lcd.setCursor(0, 3);
      lcd.print("  Dang cho lenh...  ");
      setRGB(true, true, true);      // RGB sáng Trắng khi ở trạng thái chờ
      stateTimer = millis();         
      Serial.println(">>> Chuyen sang trang thai: STANDBY (Cho lenh 10s, bep nhap nhay)");
      break;
      
    case STATE_TIMER_CONFIG:
      digitalWrite(PIN_RELAY, HIGH); 
      lcd.setCursor(0, 0);
      lcd.print(" BEP DIEN THONG MINH");
      lcd.setCursor(0, 1);
      lcd.print("====================");
      lcd.setCursor(0, 2);
      lcd.print("   THIET LAP GIO    ");
      
      // Vẽ thanh tiến trình 10 khối: HG:15 m [■■■■■     ] (20 ký tự)
      lcd.setCursor(0, 3);
      lcd.print("HG:");
      lcd.print(timerMinutes);
      if (timerMinutes < 10) lcd.print(" ");
      lcd.print("m [");
      {
        int numBlocks = (timerMinutes * 10) / 30; // 30 phút tối đa tương ứng với 10 khối
        for (int i = 0; i < 10; i++) {
          if (i < numBlocks) lcd.write(0xFF);
          else lcd.print(" ");
        }
      }
      lcd.print("]");
      
      Serial.print(">>> Chuyen sang TIMER_CONFIG. Thoi gian hien tai: ");
      Serial.print(timerMinutes);
      Serial.println(" phut.");
      break;

    case STATE_HEATING:
      digitalWrite(PIN_RELAY, HIGH); 
      setRGB(true, false, false);    // Sáng Đỏ
      lcd.setCursor(0, 0);
      lcd.print(" BEP DIEN THONG MINH");
      lcd.setCursor(0, 1);
      lcd.print("====================");
      lcd.setCursor(0, 2);
      lcd.print("    BEP DANG NAU    ");
      lcd.setCursor(0, 3);
      lcd.print("  Cong suat: 100%   ");
      Serial.println(">>> Chuyen sang trang thai: HEATING (Relay ON)");
      break;
      
    case STATE_TIMER:
      digitalWrite(PIN_RELAY, HIGH); 
      setRGB(true, false, false);    // Sáng Đỏ
      
      if (DEMO_FAST_TIMER) {
        countdownSeconds = timerMinutes; 
      } else {
        countdownSeconds = timerMinutes * 60; 
      }
      initialCountdownSeconds = countdownSeconds; 
      
      lastTimerTick = millis();
      lcd.setCursor(0, 0);
      lcd.print(" BEP DIEN THONG MINH");
      lcd.setCursor(0, 1);
      lcd.print("====================");
      lcd.setCursor(0, 2);
      lcd.print("    BEP DANG NAU    ");
      
      updateTimerDisplay(); // Vẽ thời gian và tiến trình đếm ngược ban đầu ở hàng 4
      
      Serial.print(">>> CHOT HEN GIO! Bat dau dem nguoc: ");
      Serial.print(countdownSeconds);
      Serial.println(" giay.");
      break;
  }
}

// Cập nhật hiển thị thời gian hẹn giờ dạng MM:SS kèm thanh tiến trình co giãn (hàng 4)
void updateTimerDisplay() {
  int mins = countdownSeconds / 60;
  int secs = countdownSeconds % 60;
  
  lcd.setCursor(0, 3);
  
  // 1. In thoi gian dang MM:SS (7 ky tu bao gom khoang trong dem)
  lcd.print(" ");
  if (mins < 10) lcd.print("0");
  lcd.print(mins);
  lcd.print(":");
  if (secs < 10) lcd.print("0");
  lcd.print(secs);
  lcd.print(" ");
  
  // 2. In thanh tien trinh co gian 10 khoi [------     ] (12 ky tu)
  lcd.print("[");
  int numBlocks = 0;
  if (initialCountdownSeconds > 0) {
    numBlocks = (countdownSeconds * 10) / initialCountdownSeconds;
  }
  for (int i = 0; i < 10; i++) {
    if (i < numBlocks) {
      lcd.write(0xFF); 
    } else {
      lcd.print(" ");
    }
  }
  lcd.print("]");
}
