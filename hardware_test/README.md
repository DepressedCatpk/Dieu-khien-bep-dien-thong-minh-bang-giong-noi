# Hướng dẫn Kiểm tra Phần cứng & Đấu nối dây (Chính thức)

Thư mục này chứa chương trình [hardware_test.ino](file:///d:/Apps/Antigravity/ĐỒ ÁN MCB/hardware_test/hardware_test.ino) giúp bạn kiểm thử toàn bộ linh kiện trước khi tiến hành thu thập dữ liệu và tích hợp mô hình AI theo đúng sơ đồ chân bạn cung cấp.

---

## 1. Sơ đồ đấu nối dây chi tiết (Full Pinout)

Hãy thực hiện kết nối các linh kiện với bo mạch ESP32 DevKit V1 theo các khối chức năng dưới đây. Đảm bảo rút nguồn USB trước khi nối dây.

### 1.1 Khối Nguồn Chính (Cấp nguồn cho Breadboard)
- **Chân VIN (ESP32)** --> Cắm vào đường ray Dương (+) trên Breadboard --> Tạo đường nguồn **5V chung**.
- **Chân GND (ESP32)** --> Cắm vào đường ray Âm (-) trên Breadboard --> Tạo đường **mass (GND) chung**.

### 1.2 Màn hình LCD2004 + I2C (Nguồn 5V)
- **VCC** --> Đường ray Dương (+) 5V
- **GND** --> Đường ray Âm (-) GND
- **SDA** --> Chân **GPIO 21** (ESP32)
- **SCL** --> Chân **GPIO 22** (ESP32)
- *(Lưu ý: Nếu bị lỗi hiển thị đèn nền do mất Jumper, đảm bảo đã cắm 1 sợi dây Cái-Cái nối tắt 2 chân kim Backlight phía sau module I2C).*

### 1.3 Microphone I2S INMP441 (Nguồn 3.3V)
- **VDD** --> Chân **3V3 trực tiếp** trên ESP32 (⚠️ *Tuyệt đối không cắm vào ray 5V để tránh cháy Mic*)
- **GND** --> Đường ray Âm (-) GND
- **L/R** --> Đường ray Âm (-) GND (Chọn kênh Trái - Left Channel)
- **WS** --> Chân **GPIO 26** (ESP32)
- **SCK** --> Chân **GPIO 25** (ESP32)
- **SD** --> Chân **GPIO 33** (ESP32)

### 1.4 Module LED RGB (Cathode chung - Nguồn cấp từ GPIO)
- **(-) hoặc GND** --> Đường ray Âm (-) GND
- **R (Red)** --> Chân **GPIO 13** (ESP32)
- **G (Green)** --> Chân **GPIO 12** (ESP32)
- **B (Blue)** --> Chân **GPIO 14** (ESP32)

### 1.5 Khối Chấp Hành An Toàn (Relay 5V & Đèn LED 5V)
- **VCC (Relay)** --> Đường ray Dương (+) 5V
- **GND (Relay)** --> Đường ray Âm (-) GND
- **IN (Relay)** --> Chân **GPIO 27** (ESP32)
- **Mạch nối đèn LED 5V qua tiếp điểm:**
  1. Đường ray Dương (+) 5V --> Cổng **COM** của Relay.
  2. Cổng **NO** (Thường mở) của Relay --> Điện trở $220\Omega$ --> Cực **Dương** (Chân dài) của đèn LED 5V.
  3. Cực **Âm** (Chân ngắn) của đèn LED 5V --> Đường ray Âm (-) GND.

---

## 2. Chuẩn bị trên Arduino IDE

Để chạy code trên ESP32, bạn cần cấu hình Arduino IDE theo các bước sau:

1. **Cài đặt Board ESP32:**
   - Mở Arduino IDE -> `File` -> `Preferences`.
   - Tại ô **Additional Boards Manager URLs**, thêm đường dẫn sau:
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Vào `Tools` -> `Board` -> `Boards Manager...`. Tìm kiếm từ khóa `esp32` và bấm **Install** phiên bản mới nhất (khuyến nghị bản `2.x.x` để ổn định với các thư viện TinyML).

2. **Cài đặt thư viện LCD:**
   - Vào `Sketch` -> `Include Library` -> `Manage Libraries...`.
   - Tìm kiếm `LiquidCrystal I2C` của tác giả **Frank de Brabander**.
   - Bấm **Install**.

---

## 3. Tiến hành Kiểm thử với tệp hardware_test.ino

1. Mở tệp [hardware_test.ino](file:///d:/Apps/Antigravity/ĐỒ ÁN MCB/hardware_test/hardware_test.ino) bằng Arduino IDE.
2. Kết nối ESP32 với máy tính qua cáp Micro USB.
3. Chọn đúng cổng COM và chọn board `ESP32 Dev Module` tại mục `Tools` -> `Board` và `Tools` -> `Port`.
4. Bấm **Upload** (mũi tên hướng sang phải) để nạp code.
5. Sau khi nạp xong:
   - **Màn hình LCD1602:** Khởi động hiển thị hiệu ứng chào mừng căn giữa, sau đó tự động nhảy chu kỳ hiển thị trạng thái của Relay và màu của đèn LED RGB sau mỗi 3 giây.
   - **LED RGB & Relay:** Thay đổi tuần tự giữa màu Đỏ (RED) -> Xanh Lá (GREEN) -> Xanh Dương (BLUE) tương ứng với Relay ON/OFF.
   - **Mic I2S INMP441:** Mở `Tools` -> `Serial Plotter` hoặc `Serial Monitor` (115200 baud) để theo dõi biên độ âm thanh `AvgAmp` phản hồi khi bạn nói.
