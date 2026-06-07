#ifndef AUDIO_HANDLER_H
#define AUDIO_HANDLER_H

#include "config.h"

// Khởi tạo hệ thống thu âm I2S và Task chạy ngầm trên Core 0
void init_audio();

// Ghi dữ liệu vào Ring Buffer
void write_ring_buffer(int16_t *samples, int count);

// Đọc dữ liệu từ Ring Buffer
void read_ring_buffer(int16_t *output, int length);

// Callback dùng cho Edge Impulse đọc dữ liệu từ ring buffer
int get_audio_data(size_t offset, size_t length, float *out_ptr);

#endif
