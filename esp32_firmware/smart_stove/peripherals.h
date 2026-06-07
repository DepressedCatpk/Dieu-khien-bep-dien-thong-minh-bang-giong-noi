#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include "config.h"
#include <LiquidCrystal_I2C.h>

// Khai báo extern đối tượng lcd
extern LiquidCrystal_I2C lcd;

// Khởi tạo các linh kiện ngoại vi
void init_peripherals();

// Đổi màu LED RGB
void setRGB(bool r, bool g, bool b);

// Chuyển đổi trạng thái hệ thống và cập nhật màn hình LCD
void changeState(SystemState newState);

// Cập nhật hiển thị thời gian hẹn giờ dạng MM:SS kèm thanh tiến trình
void updateTimerDisplay();

#endif
