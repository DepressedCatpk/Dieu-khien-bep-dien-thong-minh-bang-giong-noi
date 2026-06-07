import os
import sys
import time
import wave

# ==========================================
# CẤU HÌNH GHI ÂM (RECORDING CONFIGURATION)
# ==========================================
SAMPLE_RATE = 16000  # Tần số lấy mẫu tiêu chuẩn (16kHz)
DURATION = 1.0       # Thời lượng mẫu (1.0 giây)
CHANNELS = 1         # Mono
BAUD_RATE = 921600   # Baudrate cao tương thích với ESP32 để truyền dữ liệu âm thanh thời gian thực

# Ánh xạ nhãn với thư mục đích thực tế của bạn (sử dụng dấu nháy đơn kép như trong cấu trúc thư mục)
LABELS = {
    "1": ("bep lua", "d:/Apps/Antigravity/label/''bep lua''"),
    "2": ("hen gio", "d:/Apps/Antigravity/label/''hen gio''"),
    "3": ("nguyen khoi", "d:/Apps/Antigravity/label/''nguyen khoi''"),
    "4": ("noise", "d:/Apps/Antigravity/label/''noise''"),
    "5": ("tat bep", "d:/Apps/Antigravity/label/''tat bep''")
}

def check_dependencies():
    """Kiểm tra và cài đặt pyserial nếu chưa có."""
    try:
        import serial
        import serial.tools.list_ports
    except ImportError:
        print("\n[!] Thư viện 'pyserial' chưa được cài đặt.")
        print("    Vui lòng mở Terminal và chạy lệnh sau:")
        print("    pip install pyserial\n")
        sys.exit(1)

def get_serial_port():
    """Liệt kê các cổng COM và yêu cầu người dùng chọn."""
    import serial.tools.list_ports
    ports = list(serial.tools.list_ports.comports())
    
    if not ports:
        print("\n[!] Không tìm thấy thiết bị Serial/ESP32 nào cắm vào máy tính.")
        print("    Hãy chắc chắn rằng ESP32 đã được kết nối và đã cài đặt driver CH340 / CP210x.")
        sys.exit(1)
        
    print("\nDanh sách các cổng Serial phát hiện được:")
    for idx, port in enumerate(ports):
        print(f" [{idx + 1}] {port.device} - {port.description}")
        
    while True:
        choice = input("\nChọn cổng kết nối với ESP32 (1-{}): ".format(len(ports))).strip()
        try:
            val = int(choice)
            if 1 <= val <= len(ports):
                return ports[val - 1].device
        except ValueError:
            pass
        print("[!] Lựa chọn không hợp lệ, vui lòng chọn lại.")

def main():
    check_dependencies()
    import serial
    
    # 1. Kết nối Serial với ESP32
    port = get_serial_port()
    print(f"-> Đang kết nối tới cổng {port} với baudrate {BAUD_RATE}...")
    try:
        ser = serial.Serial(port, baudrate=BAUD_RATE, timeout=3.0)
        time.sleep(1)  # Chờ kết nối ổn định
        print("[OK] Kết nối thành công!")
    except Exception as e:
        print(f"[!] Lỗi kết nối cổng Serial: {e}")
        sys.exit(1)
        
    while True:
        # 2. Chọn nhãn cần thu âm
        print("\n================ CHỌN TỪ KHÓA ĐỂ THU ÂM ================")
        for key, value in LABELS.items():
            print(f" [{key}] {value[0].upper()} (Thư mục: {value[1]})")
        print(" [Q] Thoát chương trình")
        print("========================================================")
        
        choice = input("\nNhập lựa chọn của bạn (1-5 hoặc Q): ").strip().lower()
        if choice == 'q':
            print("Cảm ơn bạn đã sử dụng chương trình!")
            ser.close()
            break
            
        if choice not in LABELS:
            print("[!] Lựa chọn không hợp lệ.")
            continue
            
        label_name, folder_path = LABELS[choice]
        
        # Tạo thư mục nếu chưa tồn tại
        if not os.path.exists(folder_path):
            try:
                os.makedirs(folder_path)
            except Exception as e:
                print(f"[!] Không thể tạo thư mục {folder_path}: {e}")
                continue
        
        # 3. Vòng lặp thu âm liên tục cho nhãn đã chọn
        while True:
            timestamp = int(time.time() * 1000)
            filename = f"{label_name.replace(' ', '_')}_{timestamp}.wav"
            filepath = os.path.join(folder_path, filename)
            
            print("\n-----------------------------------------------------")
            print(f"Từ khóa thu âm: '{label_name.upper()}'")
            print(f"File sẽ lưu: {filepath}")
            input("Nhấn [ENTER] để bắt đầu thu âm 1.0 giây...")
            
            print(">>> 🔴 ĐANG THU ÂM... (Nói ngay lập tức!)")
            
            # Xóa sạch dữ liệu cũ trong bộ đệm Serial trước khi thu
            ser.reset_input_buffer()
            
            # Tính số bytes cần đọc: 16000 mẫu/giây * 1.0 giây * 2 bytes/mẫu = 32000 bytes
            total_bytes_to_read = int(SAMPLE_RATE * DURATION * 2)
            
            # Đọc trực tiếp từ cổng Serial
            raw_pcm = ser.read(total_bytes_to_read)
            
            print(">>> ⏹️ ĐÃ THU ÂM XONG.")
            
            if len(raw_pcm) < total_bytes_to_read:
                print(f"[!] Cảnh báo: Chỉ đọc được {len(raw_pcm)}/ {total_bytes_to_read} bytes.")
                print("    Có thể kết nối Serial bị nghẽn hoặc ESP32 chưa nạp code serial_audio_stream.")
                retry = input("Bạn có muốn thu âm lại mẫu này không? (Y/n): ").strip().lower()
                if retry != 'n':
                    continue
                else:
                    break
            
            # 4. Đóng gói thành file WAV
            try:
                with wave.open(filepath, 'wb') as wav_file:
                    wav_file.setnchannels(CHANNELS)
                    wav_file.setsampwidth(2)  # 16-bit PCM (2 bytes)
                    wav_file.setframerate(SAMPLE_RATE)
                    wav_file.writeframes(raw_pcm)
                print(f"[OK] Đã lưu file thành công!")
            except Exception as e:
                print(f"[!] Lỗi khi ghi file WAV: {e}")
                
            cont = input("\nTiếp tục thu âm nhãn này? (Y/n): ").strip().lower()
            if cont == 'n':
                break

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nChương trình bị ngắt bởi người dùng. Tạm biệt!")
