# Hướng dẫn chạy và tích hợp mô hình TinyML trên ESP32

Mã nguồn [smart_stove.ino](file:///d:/Apps/Antigravity/ĐỒ ÁN MCB/esp32_firmware/smart_stove/smart_stove.ino) là chương trình điều khiển chính cho hệ thống bếp điện thông minh. Chương trình áp dụng mô hình kiến trúc đa nhân (dual-core) của ESP32:
- **Core 0:** Chạy tác vụ ngầm đọc tín hiệu I2S liên tục từ mic INMP441 và lưu vào vòng đệm (Ring Buffer).
- **Core 1:** Chạy tác vụ chính phân tích TinyML và điều khiển máy trạng thái (LCD, RGB LED, Relay).

---

## 1. Cách Kiểm thử Máy trạng thái & Tính năng Hẹn giờ Multi-click (Qua Serial)

Để thử nghiệm tính năng hẹn giờ tăng dần bằng giọng nói (Multi-click) trước khi nạp mô hình AI:

1. Đảm bảo `#define EDGE_IMPULSE_LIBRARY_INSTALLED 0` ở dòng 18 của tệp `smart_stove.ino`.
2. Mở **Serial Monitor** (115200 baud).
3. Sử dụng các phím gõ để giả lập:
   - **Đánh thức bếp:** Gõ phím `k` (giả lập nói *"nguyên khôi"*). Đèn bếp sáng trưng 1.5 giây để chào mừng, sau đó bắt đầu nhấp nháy liên tục (trạng thái **STANDBY**).
   - **Hẹn giờ lần 1:** Trong lúc bếp đang nhấp nháy chờ lệnh, gõ phím `h` (giả lập nói *"hẹn giờ"*). LCD sẽ chuyển sang màn hình thiết lập và hiển thị: `Hen gio: 5 phut`. LED RGB sáng màu **Vàng**.
   - **Cộng dồn thời gian (Lần 2):** Gõ tiếp phím `h` trước 3 giây trôi qua. LCD cập nhật: `Hen gio: 10 phut`.
   - **Cộng dồn thời gian (Lần 3):** Gõ tiếp phím `h` trước 3 giây trôi qua. LCD cập nhật: `Hen gio: 15 phut`. LED RGB sáng màu **Đỏ**.
   - **Chốt lệnh:** Sau khi bạn ngừng gõ chữ `h` quá 3 giây, hệ thống sẽ chốt thời gian 15 phút này và tự động chuyển sang trạng thái đếm ngược (`STATE_TIMER`). Đèn bếp (Relay) chuyển sang **sáng liên tục** và LED RGB chuyển sang màu **Tím**.
   - **Tắt bếp khẩn cấp:** Gõ phím `t` (giả lập *"tắt bếp"*) bất cứ lúc nào để tắt hoàn toàn bếp quay về `STATE_SLEEP`.

---

## 2. Lưu ý kỹ thuật về Hẹn giờ & Khử nhiễu giọng nói

### 2.1 Chế độ Demo nhanh (DEMO_FAST_TIMER)
Tại dòng 35 của tệp `smart_stove.ino`, có cấu hình `#define DEMO_FAST_TIMER true`.
- Khi để `true`: Mỗi 1 phút hẹn giờ trên màn hình sẽ tương ứng với **1 giây** thực tế (ví dụ: Hẹn giờ 15 phút sẽ đếm ngược tắt bếp trong 15 giây). Điều này giúp bạn trình diễn cho giảng viên xem trực tiếp mà không phải đợi hàng chục phút.
- Khi để `false`: Thời gian đếm ngược sẽ chạy đúng 60 giây thực tế cho mỗi phút cài đặt.

### 2.2 Khóa chống trùng tiếng (Speech Debounce)
Trong Edge Impulse, do mô hình phân tích liên tục mỗi 500ms (sliding window), khi bạn nói từ *"hẹn giờ"*, mô hình có thể phát hiện từ khóa này 2-3 lần liên tiếp. 
Để giải quyết lỗi thực tế này, trong code đã tích hợp một bộ khóa thời gian tại dòng 334:
`if (highest_score > 0.75 && ... && (timeNow - lastCommandTime >= 1500))`
Hệ thống sẽ **chỉ nhận 1 lệnh cách nhau tối thiểu 1.5 giây**. Khi bạn giả lập bằng Serial, hãy gõ các phím cách nhau khoảng 1.5 giây để khớp với bộ lọc này.

---

## 3. Quy trình Tích hợp Mô hình Edge Impulse thực tế

Khi bạn đã thu tập đủ dữ liệu và huấn luyện xong mô hình CNN trên Edge Impulse:

1. Vào mục **Deployment** trên Edge Impulse -> Chọn **Arduino library** -> Bấm **Build**.
2. Giải nén hoặc để nguyên file `.zip` và nạp vào Arduino IDE thông qua: **Sketch** -> **Include Library** -> **Add .ZIP Library...**.
3. Mở tệp `smart_stove.ino`. Chỉnh sửa dòng số 18:
   ```cpp
   #define EDGE_IMPULSE_LIBRARY_INSTALLED 1
   ```
4. Chỉnh sửa dòng số 22 để đổi tên file header tương ứng với tên thư viện của bạn:
   ```cpp
   #include <Ten_Thu_Vien_Cua_Ban_inferencing.h>
   ```
5. Nạp code mới lên ESP32. Lúc này bạn có thể cất bàn phím đi và ra lệnh trực tiếp bằng giọng nói thực tế!
