import os
import sys
import time

# Tốc độ baudrate khớp với code ESP32 của bạn
BAUD_RATE = 921600

def check_dependencies():
    try:
        import serial
    except ImportError:
        print("\n[!] Chưa cài đặt thư viện 'pyserial'.")
        print("    Vui lòng chạy lệnh: pip install pyserial\n")
        sys.exit(1)

def main():
    check_dependencies()
    import serial
    import serial.tools.list_ports
    
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("[!] Không tìm thấy cổng COM nào.")
        sys.exit(1)
        
    print("Danh sách các cổng COM:")
    for idx, port in enumerate(ports):
        print(f" [{idx + 1}] {port.device} - {port.description}")
        
    choice = input("Chọn cổng kết nối ESP32 (1-{}): ".format(len(ports))).strip()
    try:
        val = int(choice)
        port_device = ports[val - 1].device
    except:
        print("[!] Lựa chọn không hợp lệ.")
        sys.exit(1)
        
    output_filename = input("Nhập tên file lưu dữ liệu (ví dụ: ghi_am.raw): ").strip()
    if not output_filename:
        output_filename = "ghi_am.raw"
        
    try:
        ser = serial.Serial(port_device, baudrate=BAUD_RATE, timeout=1.0)
        time.sleep(1)
        ser.reset_input_buffer()
        print(f"\n[OK] Đã kết nối với {port_device} ở baudrate {BAUD_RATE}.")
        print(f"-> Chuẩn bị ghi âm vào file: {output_filename}")
        print("Nhấn [ENTER] để bắt đầu ghi âm...")
        input()
        
        with open(output_filename, "wb") as f:
            print("🔴 ĐANG GHI ÂM... Nhấn [Ctrl+C] để DỪNG ghi âm.")
            while True:
                # Đọc dữ liệu từ Serial và ghi thẳng vào file thô
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    f.write(data)
                time.sleep(0.01)
                
    except KeyboardInterrupt:
        print("\n⏹️ ĐÃ DỪNG GHI ÂM.")
        print(f"[OK] File thô đã được lưu tại: {os.path.abspath(output_filename)}")
        ser.close()
    except Exception as e:
        print(f"[!] Lỗi: {e}")

if __name__ == "__main__":
    main()
