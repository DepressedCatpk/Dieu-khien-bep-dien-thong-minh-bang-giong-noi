import os
import sys
import time
import sounddevice as sd
import numpy as np
import scipy.io.wavfile as wav

# ==========================================
# CẤU HÌNH GHI ÂM (RECORDING CONFIGURATION)
# ==========================================
SAMPLE_RATE = 16000  # Tần số lấy mẫu tiêu chuẩn cho TinyML (16kHz)
DURATION = 1.5       # Thời lượng mẫu (1.5 giây)
CHANNELS = 1         # Ghi âm đơn kênh (Mono)

# Danh sách các nhãn (Labels)
LABELS = ["nguyen_khoi", "bep_lua", "hen_gio", "tat_bep", "noise"]
DATASET_DIR = "dataset"

def check_dependencies():
    """Kiểm tra và hướng dẫn cài đặt thư viện cần thiết."""
    try:
        import sounddevice
        import scipy
        import numpy
    except ImportError:
        print("\n[!] Thư viện chưa đầy đủ. Hãy chạy lệnh sau trong Terminal để cài đặt:")
        print("    pip install sounddevice numpy scipy\n")
        sys.exit(1)

def init_folders():
    """Tạo các thư mục chứa dữ liệu nếu chưa tồn tại."""
    if not os.path.exists(DATASET_DIR):
        os.makedirs(DATASET_DIR)
        print(f"-> Đã tạo thư mục gốc: {DATASET_DIR}")
    
    for label in LABELS:
        folder_path = os.path.join(DATASET_DIR, label)
        if not os.path.exists(folder_path):
            os.makedirs(folder_path)
            print(f"-> Đã tạo thư mục nhãn: {folder_path}")

def record_sample(label):
    """Tiến hành thu âm một mẫu dài DURATION giây và lưu lại."""
    folder_path = os.path.join(DATASET_DIR, label)
    
    # Tạo tên tệp độc nhất dựa trên timestamp
    timestamp = int(time.time() * 1000)
    filename = f"{label}_{timestamp}.wav"
    filepath = os.path.join(folder_path, filename)
    
    print("\n-------------------------------------------")
    print(f"Chuan bi ghi am tu: '{label.upper()}'")
    print("Nhan [ENTER] de bat dau ghi am...")
    input()
    
    print(">>> 🔴 DANG GHI AM... (Noi tu khoa ngay!)")
    # Ghi âm từ mic mặc định
    recording = sd.rec(int(DURATION * SAMPLE_RATE), samplerate=SAMPLE_RATE, channels=CHANNELS, dtype='int16')
    sd.wait() # Chờ ghi âm xong
    print(">>> ⏹️ DA GHI AM XONG.")
    
    # Phát lại mẫu vừa ghi âm để kiểm tra chất lượng
    print("Phat lai am thanh de kiem tra... (Nhan Ctrl+C neu muon dung)")
    try:
        sd.play(recording, SAMPLE_RATE)
        sd.wait()
    except KeyboardInterrupt:
        pass
    
    # Hỏi người dùng xem mẫu ghi âm có đạt chất lượng không
    save_opt = input("Ban co muon LUULAI mau nay khong? (Y/n): ").strip().lower()
    if save_opt == '' or save_opt == 'y':
        # Lưu file WAV 16-bit PCM
        wav.write(filepath, SAMPLE_RATE, recording)
        print(f"[OK] Da luu tệp: {filepath}")
        return True
    else:
        print("[HUY] Da bo qua mau nay.")
        return False

def show_statistics():
    """Hiển thị số lượng mẫu hiện tại trong mỗi thư mục nhãn."""
    print("\n================ THONG KE DATASET ================")
    total = 0
    for label in LABELS:
        folder_path = os.path.join(DATASET_DIR, label)
        if os.path.exists(folder_path):
            count = len([f for f in os.listdir(folder_path) if f.endswith('.wav')])
            print(f" - Nhãn '{label}': {count} mẫu")
            total += count
        else:
            print(f" - Nhãn '{label}': 0 mẫu")
    print(f"Tong so mau hien tai: {total}")
    print("==================================================")

def main():
    check_dependencies()
    init_folders()
    
    while True:
        show_statistics()
        print("\nCHON NHAN BAN MUON THU AM:")
        for idx, label in enumerate(LABELS):
            print(f" [{idx + 1}] {label.upper()}")
        print(" [Q] Thoat chương trình")
        
        choice = input("\nNhap lua chon cua ban (1-5 hoac Q): ").strip().lower()
        if choice == 'q':
            print("Tam biet!")
            break
        
        try:
            choice_idx = int(choice) - 1
            if 0 <= choice_idx < len(LABELS):
                selected_label = LABELS[choice_idx]
                
                # Vòng lặp thu âm liên tục cho nhãn đã chọn cho đến khi người dùng muốn đổi nhãn
                while True:
                    success = record_sample(selected_label)
                    cont = input("\nTiep tuc thu am nhan nay? (Y/n): ").strip().lower()
                    if cont == 'n':
                        break
            else:
                print("[!] Lua chon khong hop le. Vui long chon tu 1 den 5.")
        except ValueError:
            print("[!] Dau vao khong hop le. Vui long nhap so tu 1 den 5 hoac Q.")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nChuong trinh bi ngat. Tam biet!")
