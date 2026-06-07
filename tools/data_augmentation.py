import os
import sys
import random
import numpy as np
import scipy.io.wavfile as wav

# ==========================================
# CẤU HÌNH TĂNG CƯỜNG (AUGMENTATION CONFIG)
# ==========================================
INPUT_DIR = "dataset"
OUTPUT_DIR = "dataset_augmented"
SAMPLE_RATE = 16000

# Các tham số tăng cường
SHIFT_RANGE_MS = 150       # Dịch chuyển tối đa 150ms sang trái hoặc phải
VOL_MIN = 0.6              # Giảm âm lượng tối đa còn 60%
VOL_MAX = 1.4              # Tăng âm lượng tối đa lên 140%
NOISE_LEVEL_MIN = 0.05     # Tỷ lệ tiếng ồn tối thiểu (5%)
NOISE_LEVEL_MAX = 0.20     # Tỷ lệ tiếng ồn tối đa (20%)

# Số lượng bản sao tăng cường tạo ra cho mỗi tệp gốc
COPIES_PER_FILE = 4

def check_dependencies():
    try:
        import scipy
        import numpy
    except ImportError:
        print("\n[!] Thư viện numpy hoặc scipy chưa được cài đặt. Vui lòng chạy:")
        print("    pip install numpy scipy\n")
        sys.exit(1)

def get_noise_files():
    """Lấy danh sách các tệp âm thanh tiếng ồn từ thư mục dataset/noise."""
    noise_dir = os.path.join(INPUT_DIR, "noise")
    if not os.path.exists(noise_dir):
        return []
    
    noise_files = [os.path.join(noise_dir, f) for f in os.listdir(noise_dir) if f.endswith('.wav')]
    return noise_files

def load_wav(filepath):
    """Đọc tệp WAV và chuẩn hóa dữ liệu về float32 dạng mono."""
    sr, data = wav.read(filepath)
    if sr != SAMPLE_RATE:
        # Nhắc nhở nếu tần số không khớp
        raise ValueError(f"Tần số lấy mẫu của {filepath} là {sr}Hz, yêu cầu {SAMPLE_RATE}Hz")
    
    # Ép kiểu dữ liệu về float32 để tính toán không bị tràn
    if data.dtype == np.int16:
        data = data.astype(np.float32) / 32768.0
    elif data.dtype == np.int32:
        data = data.astype(np.float32) / 2147483648.0
    elif data.dtype == np.uint8:
        data = (data.astype(np.float32) - 128.0) / 128.0
        
    return data

def save_wav(data, filepath):
    """Lưu dữ liệu âm thanh dạng float32 thành tệp WAV 16-bit PCM."""
    # Giới hạn biên độ trong khoảng [-1.0, 1.0] để tránh méo tiếng
    clipped = np.clip(data, -1.0, 1.0)
    int_data = (clipped * 32767.0).astype(np.int16)
    wav.write(filepath, SAMPLE_RATE, int_data)

# --- CÁC HÀM XỬ LÝ ÂM THANH ---

def time_shift(data):
    """Dịch chuyển âm thanh ngẫu nhiên sang trái hoặc phải."""
    shift_samples = int((random.randint(-SHIFT_RANGE_MS, SHIFT_RANGE_MS) / 1000.0) * SAMPLE_RATE)
    shifted = np.zeros_like(data)
    
    if shift_samples > 0:
        # Dịch sang phải (chèn khoảng lặng vào đầu)
        shifted[shift_samples:] = data[:-shift_samples]
    elif shift_samples < 0:
        # Dịch sang trái (chèn khoảng lặng vào cuối)
        shifted[:shift_samples] = data[-shift_samples:]
    else:
        shifted = data.copy()
        
    return shifted

def scale_volume(data):
    """Thay đổi âm lượng ngẫu nhiên."""
    scale = random.uniform(VOL_MIN, VOL_MAX)
    return data * scale

def inject_noise(data, noise_files):
    """Trộn âm thanh tiếng ồn nền vào âm thanh gốc."""
    augmented = data.copy()
    
    # Nếu có tệp tiếng ồn từ thư mục noise, trộn ngẫu nhiên
    if noise_files:
        try:
            noise_path = random.choice(noise_files)
            noise_data = load_wav(noise_path)
            
            # Đảm bảo tiếng ồn có độ dài ít nhất bằng dữ liệu gốc
            if len(noise_data) < len(data):
                # Lặp lại tiếng ồn nếu quá ngắn
                noise_data = np.tile(noise_data, int(np.ceil(len(data) / len(noise_data))))
                
            # Chọn đoạn ngẫu nhiên trong tệp tiếng ồn
            start_idx = random.randint(0, len(noise_data) - len(data))
            noise_segment = noise_data[start_idx : start_idx + len(data)]
            
            # Trộn với tỷ lệ ngẫu nhiên
            factor = random.uniform(NOISE_LEVEL_MIN, NOISE_LEVEL_MAX)
            augmented = data + noise_segment * factor
            return augmented
        except Exception as e:
            # Nếu có lỗi khi đọc tệp noise, chuyển sang tự tạo nhiễu trắng
            pass
            
    # Tự sinh nhiễu trắng (White Noise) làm dự phòng
    noise = np.random.randn(len(data))
    factor = random.uniform(NOISE_LEVEL_MIN, NOISE_LEVEL_MAX) * 0.2 # Giảm nhỏ âm lượng nhiễu trắng sinh thêm
    augmented = data + noise * factor
    return augmented

def augment_file(filepath, out_dir, noise_files, idx):
    """Tạo ra phiên bản tăng cường từ 1 tệp âm thanh gốc."""
    try:
        data = load_wav(filepath)
        filename_raw = os.path.basename(filepath).replace(".wav", "")
        
        # 1. Ghi tệp gốc sang thư mục đích
        save_wav(data, os.path.join(out_dir, f"{filename_raw}_orig.wav"))
        
        # 2. Tạo các bản sao tăng cường
        for i in range(COPIES_PER_FILE):
            aug_data = data.copy()
            
            # Áp dụng ngẫu nhiên các phương pháp biến đổi
            # Ít nhất áp dụng dịch thời gian hoặc âm lượng
            aug_data = time_shift(aug_data)
            aug_data = scale_volume(aug_data)
            
            # Tỷ lệ 70% trộn thêm tiếng ồn
            if random.random() < 0.70:
                aug_data = inject_noise(aug_data, noise_files)
                
            # Lưu tệp tăng cường
            out_filename = f"{filename_raw}_aug_{i}.wav"
            save_wav(aug_data, os.path.join(out_dir, out_filename))
            
    except Exception as e:
        print(f"[LOI] Khong the xử lý {filepath}: {e}")

def main():
    check_dependencies()
    
    if not os.path.exists(INPUT_DIR):
        print(f"[!] Thu muc nguon '{INPUT_DIR}' khong ton tai. Vui long chay 'audio_recorder.py' truoc de tao du lieu.")
        return
        
    # Lấy danh sách tiếng ồn nền
    noise_files = get_noise_files()
    print(f"-> Tim thay {len(noise_files)} tep tieng on nen de tron (nhan 'noise').")
    
    # Quét qua các thư mục con trong dataset
    subdirs = [d for d in os.listdir(INPUT_DIR) if os.path.isdir(os.path.join(INPUT_DIR, d))]
    
    if not subdirs:
        print("[!] Khong tim thay nhãn dữ liệu nao trong thu muc 'dataset'.")
        return
        
    print("\nBat dau tang cuong du lieu (Data Augmentation)...")
    
    total_processed = 0
    total_generated = 0
    
    for label in subdirs:
        in_label_dir = os.path.join(INPUT_DIR, label)
        out_label_dir = os.path.join(OUTPUT_DIR, label)
        
        if not os.path.exists(out_label_dir):
            os.makedirs(out_label_dir)
            
        wav_files = [f for f in os.listdir(in_label_dir) if f.endswith('.wav')]
        
        if not wav_files:
            print(f" - Nhan '{label}': Khong co tep .wav nao.")
            continue
            
        print(f" - Dang xu ly nhan '{label}': {len(wav_files)} tep goc...")
        
        for file in wav_files:
            filepath = os.path.join(in_label_dir, file)
            augment_file(filepath, out_label_dir, noise_files, total_processed)
            total_processed += 1
            total_generated += (1 + COPIES_PER_FILE) # Gốc + các bản sao
            
    print("\n================ HOAN THANH TANG CUONG ================")
    print(f" Da xử lý xong: {total_processed} tep goc.")
    print(f" Tong so tep da tao ra (trong '{OUTPUT_DIR}'): {total_generated} tep.")
    print(" Ban da co the nen thu muc nay lai hoac tai tung thu muc len Edge Impulse!")
    print("=========================================================")

if __name__ == "__main__":
    main()
