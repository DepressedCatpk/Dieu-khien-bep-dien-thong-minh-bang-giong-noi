# Bếp Điện Thông Minh Điều Khiển Bằng Giọng Nói (TinyML - ESP32)

Dự án nghiên cứu thiết kế, chế tạo mô hình thực nghiệm hệ thống bếp điện thông minh có khả năng nhận dạng câu lệnh giọng nói tiếng Việt và thực thi điều khiển hoàn toàn ngoại tuyến (offline) tại biên (Edge AI). Hệ thống ứng dụng thuật toán trích xuất đặc trưng âm học MFCC kết hợp mạng nơ-ron tích chập 2 chiều (2D-CNN) được tối ưu hóa bằng công nghệ TinyML để chạy thời gian thực trên vi điều khiển ESP32 giá thành rẻ.

---

## 1. Kiến trúc thư mục dự án

```text
├── docs/                      # Chứa báo cáo báo cáo tiểu luận (PDF/DOCX)
├── esp32_firmware/            # Mã nguồn nhúng chính nạp cho vi điều khiển
│   └── smart_stove/
│       ├── smart_stove.ino    # File khởi chạy chính và xử lý máy trạng thái FSM
│       ├── config.h           # Tệp cấu hình các chân GPIO và tham số mô hình AI
│       ├── audio_handler.cpp  # Xử lý đọc luồng âm thanh I2S từ mic qua DMA
│       ├── audio_handler.h
│       ├── peripherals.cpp    # Điều khiển các thiết bị LCD, Relay, LED RGB
│       └── peripherals.h
├── hardware_test/             # Mã nguồn kiểm tra kết nối phần cứng ngoại vi
│   ├── hardware_test.ino      # Kiểm tra độc lập LCD, đóng ngắt Relay và LED RGB
│   └── README.md
├── i2c_scanner/               # Mã nguồn quét thiết bị trên bus I2C
│   └── i2c_scanner.ino        # Tìm địa chỉ hex của màn hình LCD (mặc định 0x27)
├── mic_test/                  # Mã nguồn kiểm tra tín hiệu microphone
│   └── mic_test.ino           # Đọc tín hiệu I2S thô vẽ lên Serial Plotter
└── tools/                     # Các công cụ Python hỗ trợ tiền xử lý dữ liệu trên PC
    ├── audio_recorder.py      # Thu âm giọng nói từ mic PC để làm Dataset (.wav)
    ├── data_augmentation.py   # Tăng cường dữ liệu (trộn nhiễu trắng, dịch tần số)
    ├── record_from_serial.py  # Thu âm truyền luồng PCM thô qua Serial từ ESP32
    └── serial_audio_stream/   # Code ESP32 hỗ trợ stream dữ liệu mic lên cổng Serial
        └── serial_audio_stream.ino
```

---

## 2. Sơ đồ đấu nối chân phần cứng (Pinout)

Để hệ thống hoạt động chính xác, hãy kết nối vi điều khiển ESP32 DevKit V1 với các thiết bị ngoại vi theo sơ đồ dưới đây:

| Linh kiện ngoại vi | Chân linh kiện | Chân GPIO trên ESP32 | Ghi chú |
| :--- | :---: | :---: | :--- |
| **INMP441 (Microphone I2S)** | VDD | 3V3 | Nguồn cấp 3.3V từ ESP32 |
| | GND | GND | Nối đất |
| | L/R | GND | Chọn kênh Trái (Left Channel) |
| | SCK | GPIO 25 | I2S Serial Clock |
| | WS | GPIO 26 | I2S Word Select |
| | SD | GPIO 33 | I2S Serial Data Input |
| **LCD 2004 (Màn hình hiển thị)** | VCC | VIN (5V) | Cấp nguồn 5V ngoài hoặc chân VIN |
| | GND | GND | Nối đất chung |
| | SDA | GPIO 21 | Giao tiếp dữ liệu I2C |
| | SCL | GPIO 22 | Giao tiếp xung nhịp I2C |
| **LED RGB (Đèn chỉ báo màu)** | R | GPIO 13 | Nối qua điện trở hạn dòng 220 Ohm |
| | G | GPIO 12 | Nối qua điện trở hạn dòng 220 Ohm |
| | B | GPIO 14 | Nối qua điện trở hạn dòng 220 Ohm |
| | Cathode chung | GND | Chân âm nối chung |
| **Relay 5V 1 kênh (Bếp mô phỏng)**| IN | GPIO 27 | Tín hiệu kích hoạt mức cao (Active High) |
| | VCC / GND | 5V / GND | Cấp nguồn nuôi 5V từ cổng sạc phụ |

*Lưu ý:* Nên cấp nguồn 5V riêng biệt cho module Relay và LCD để tránh sụt áp gây reset ESP32 khi đóng ngắt cuộn dây của Rơ-le.

---

## 3. Hướng dẫn chạy các kịch bản thử nghiệm ngoại vi

Trước khi nạp code chính, sinh viên cần tiến hành chạy các code kiểm tra phần cứng độc lập trong các thư mục tương ứng:

### 3.1. Dò địa chỉ màn hình LCD (`i2c_scanner`)
1. Mở file `i2c_scanner/i2c_scanner.ino` bằng Arduino IDE.
2. Kết nối ESP32 với máy tính, chọn đúng cổng COM và nhấn **Upload**.
3. Mở **Serial Monitor** ở baudrate `115200`. Ghi nhận địa chỉ hex xuất hiện (thường là `0x27` hoặc `0x3F`). Địa chỉ này sẽ được điền vào hằng số `LCD_I2C_ADDR` trong file `config.h`.

### 3.2. Kiểm tra tín hiệu Microphone I2S (`mic_test`)
1. Mở file `mic_test/mic_test.ino` bằng Arduino IDE và nạp chương trình.
2. Mở **Serial Plotter** trên Arduino IDE ở baudrate `115200`.
3. Phát âm thanh (nói hoặc gõ nhẹ vào mic), biểu đồ sóng âm trên màn hình phải dao động nhịp nhàng theo cường độ âm thanh. Nếu biểu đồ là đường thẳng nằm ngang, hãy kiểm tra lại các chân kết nối SCK, WS, SD của INMP441.

### 3.3. Kiểm tra các ngoại vi chấp hành (`hardware_test`)
1. Mở file `hardware_test/hardware_test.ino` bằng Arduino IDE và nạp chương trình.
2. Mạch sẽ chạy chu kỳ tự động:
   - LCD 2004 hiển thị thông báo kiểm tra ở cả 4 dòng.
   - Relay đóng ngắt (nghe tiếng cạch nhẹ của tiếp điểm) tuần tự mỗi 2 giây.
   - LED RGB đổi màu luân phiên giữa Đỏ, Xanh lá, Xanh dương và Trắng.

---

## 4. Hướng dẫn cài đặt công cụ Python (`tools`)

Thư mục `tools/` chứa các kịch bản Python hỗ trợ quá trình thu thập và tiền xử lý dữ liệu giọng nói trên máy tính.

### Cài đặt thư viện phụ thuộc
Yêu cầu máy tính cài đặt Python 3.8 trở lên. Mở Terminal tại thư mục `tools/` và chạy lệnh cài đặt:
```bash
pip install pyaudio soundfile numpy scipy pyserial
```

### Sử dụng các kịch bản:
* **`audio_recorder.py`**: Chạy kịch bản này để thu âm các mẫu câu lệnh tiếng Việt thông qua microphone máy tính. Các tệp âm thanh sẽ được tự động lưu dưới định dạng đạt chuẩn của Edge Impulse (16kHz, Mono, 16-bit PCM).
  ```bash
  python audio_recorder.py
  ```
* **`data_augmentation.py`**: Kịch bản tăng cường dữ liệu tự động. Nhập thư mục chứa các file ghi âm gốc, kịch bản sẽ tạo thêm các mẫu biến thể bằng cách trộn thêm nhiễu trắng (noise) hoặc dịch chuyển tần số để tăng độ bền vững cho mô hình AI.
  ```bash
  python data_augmentation.py
  ```

---

## 5. Hướng dẫn nạp mã nguồn chính và vận hành hệ thống

Mã nguồn chính nằm trong thư mục `esp32_firmware/smart_stove/`.

### Bước 1: Huấn luyện và xuất thư viện TinyML trên Edge Impulse
1. Thu thập dữ liệu tiếng Việt (5 lớp nhãn: `nguyen khoi`, `bep lua`, `tat bep`, `hen gio`, và `noise`).
2. Tải toàn bộ tập mẫu lên **Edge Impulse**.
3. Cấu hình dự án (Impulse Design):
   - **Time series data:** Window size = `1000ms`, Window increase = `100ms`.
   - **Processing block:** Chọn **MFCC** (32 Mel filterbanks, 13 coefficients, frame length 20ms, overlap 10ms).
   - **Learning block:** Chọn **Classification (Keras)** (Mạng tích chập 2D-CNN).
4. Huấn luyện mô hình, tiến hành lượng tử hóa **INT8** (độ chính xác đạt khoảng 95.8%).
5. Vào thẻ **Deployment**, chọn **Arduino library**, tải tệp `.zip` về máy tính.

### Bước 2: Nhúng thư viện vào Arduino IDE
1. Trong Arduino IDE, chọn `Sketch -> Include Library -> Add .ZIP Library...` và chọn file ZIP vừa tải từ Edge Impulse.
2. Cài đặt thêm thư viện màn hình `LiquidCrystal_I2C` từ thư viện quản lý của IDE.

### Bước 3: Cấu hình và Nạp chương trình
1. Mở file `esp32_firmware/smart_stove/smart_stove.ino`.
2. Kiểm tra các cấu hình GPIO và địa chỉ LCD trong tệp `config.h` để đảm bảo khớp với sơ đồ đấu nối thực tế.
3. Chọn Board là **ESP32 Dev Module** (hoặc NodeMCU-32S) và chọn đúng cổng COM.
4. Nhấn **Upload** để biên dịch và nạp code xuống chip.

### Bước 4: Vận hành kiểm tra máy trạng thái (FSM)
Sau khi nạp thành công, hệ thống hoạt động theo máy trạng thái 5 bước như sau:
1. **SLEEP:** Bếp tắt hoàn toàn (Relay ngắt, LED RGB tắt, LCD hướng dẫn người dùng gọi *"nguyễn khôi"*).
2. **STANDBY:** Khi gọi đúng *"nguyên khôi"*, LED RGB nhấp nháy màu Trắng và Relay chớp tắt đồng bộ chu kỳ 500ms để chỉ báo sẵn sàng. Trạng thái chờ lệnh kéo dài 10 giây.
3. **HEATING (Nấu thông thường):** Gọi lệnh *"bếp lửa"* khi đang chờ, bếp sẽ bật công suất tối đa (Relay đóng liên tục, LED RGB chuyển Đỏ).
4. **TIMER_CONFIG (Cấu hình giờ):** Gọi lệnh *"hẹn giờ"* để tăng dần thời gian đếm ngược (mỗi lần tăng 5 phút). Bếp chốt cấu hình sau 3 giây không nhận thêm lệnh mới.
5. **TIMER (Đếm ngược):** Bếp đếm ngược thời gian thực trên LCD kèm thanh tiến trình co giãn. Hết giờ bếp tự động ngắt Relay và quay về SLEEP.
6. **TẮT BẾP:** Người dùng có thể gọi lệnh *"tắt bếp"* ở bất kỳ thời điểm nào để cưỡng bức hệ thống tắt an toàn ngay lập tức.

---

## 6. Hướng dẫn xử lý sự cố thường gặp (Troubleshooting)

* **Lỗi biên dịch "multiple definition of..." (Linker errors):**
  - *Nguyên nhân:* Thư viện SDK Edge Impulse chứa các định nghĩa hàm toàn cục bị include ở nhiều file `.cpp` khác nhau thông qua tệp `config.h`.
  - *Giải quyết:* Cấu trúc mã nguồn đã được tách biệt. Đảm bảo file thư viện Edge Impulse `<ten_thu_vien_inferencing.h>` chỉ được khai báo duy nhất tại file chính `smart_stove.ino`. Tất cả các tệp khác chỉ tham chiếu qua cấu hình trung gian và bộ đệm PCM.
* **LCD 2004 không sáng đèn nền hoặc hiển thị ô vuông đen:**
  - *Giải quyết:* Kiểm tra lại biến trở chỉnh độ tương phản (Contrast) ở mặt sau module I2C PCF8574 của LCD. Dùng tua-vít xoay nhẹ cho đến khi chữ hiện rõ. Đảm bảo nối đúng chân SDA/SCL vào GPIO 21/22 của ESP32.
* **Bếp không nhận dạng được lệnh giọng nói:**
  - *Giải quyết:* Mở Serial Monitor xem log. Nếu mô hình nhận diện ra từ khóa nhưng vi điều khiển không chuyển trạng thái, hãy kiểm tra nhãn trả về. Code đã được tối ưu để hỗ trợ song song cả nhãn có gạch dưới (`nguyen_khoi`) và nhãn có dấu cách (`nguyen khoi`).
  - *Độ nhạy kích hoạt:* Từ khóa đánh thức `"nguyen khoi"` đã được tối ưu hạ ngưỡng tin cậy xuống **`0.65`** để tăng độ nhạy, các lệnh thực thi khác giữ ở mức **`0.75`** để tránh nhận diện sai do tạp âm.
