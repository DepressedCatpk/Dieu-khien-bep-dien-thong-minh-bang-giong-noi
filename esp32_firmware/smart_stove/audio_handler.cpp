#include "audio_handler.h"
#include <driver/i2s.h>

// Quản lý Ring Buffer (Bộ đệm vòng)
#define RING_BUFFER_SIZE EI_CLASSIFIER_RAW_SAMPLE_COUNT
static int16_t ring_buffer[RING_BUFFER_SIZE];
static volatile int ring_buffer_head = 0;
static portMUX_TYPE ringBufferMutex = portMUX_INITIALIZER_UNLOCKED;

void write_ring_buffer(int16_t *samples, int count) {
  portENTER_CRITICAL(&ringBufferMutex);
  for (int i = 0; i < count; i++) {
    ring_buffer[ring_buffer_head] = samples[i];
    ring_buffer_head = (ring_buffer_head + 1) % RING_BUFFER_SIZE;
  }
  portEXIT_CRITICAL(&ringBufferMutex);
}

void read_ring_buffer(int16_t *output, int length) {
  portENTER_CRITICAL(&ringBufferMutex);
  int read_index = (ring_buffer_head - length + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
  for (int i = 0; i < length; i++) {
    output[i] = ring_buffer[read_index];
    read_index = (read_index + 1) % RING_BUFFER_SIZE;
  }
  portEXIT_CRITICAL(&ringBufferMutex);
}

int get_audio_data(size_t offset, size_t length, float *out_ptr) {
  int16_t *temp_buf = (int16_t *)malloc(length * sizeof(int16_t));
  if (temp_buf == NULL) return -1;
  read_ring_buffer(temp_buf, length);
  for (size_t i = 0; i < length; i++) {
    out_ptr[i] = (float)temp_buf[i];
  }
  free(temp_buf);
  return 0;
}

// Task chạy ngầm thu âm liên tục (Audio Capture Task)
void audioCaptureTask(void *pvParameters) {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // INMP441 dùng 32-bit slot
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 128,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);

  int32_t i2s_raw_buffer[128];
  int16_t processed_samples[128];
  size_t bytes_read;

  while (true) {
    // Đọc mẫu âm thanh từ DMA
    i2s_read(I2S_PORT, &i2s_raw_buffer, sizeof(i2s_raw_buffer), &bytes_read, portMAX_DELAY);
    int samples_count = bytes_read / sizeof(int32_t);
    
    // Tiền xử lý: đưa về dạng 16-bit PCM có dấu
    for (int i = 0; i < samples_count; i++) {
      processed_samples[i] = (int16_t)(i2s_raw_buffer[i] >> 14);
    }
    
    write_ring_buffer(processed_samples, samples_count);
    
    // Dừng 1 tick hệ điều hành để tránh watchdog kích hoạt
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void init_audio() {
  xTaskCreatePinnedToCore(
    audioCaptureTask,
    "AudioCapture",
    4096,
    NULL,
    10,
    NULL,
    0 // Chạy trên Core 0
  );
}
