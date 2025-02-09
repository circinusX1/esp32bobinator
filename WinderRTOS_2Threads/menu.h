

#pragma once

#include <Arduino.h>
#include <ESPFlash.h>
#include <AccelStepper.h>

#include <esp_task_wdt.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <esp_chip_info.h>
#include <spi_flash_mmap.h>
#endif

#define ONE_SPIN 200.0
#define MAX_SPEED 2500.0
#define ACCEL 5000.0
#define LIMITER 19
#define SPEED 1000.0
#define CM 1
#define GM 0
#define scope__ for (int r = 0; r == 0; r = 1)

struct input_t {
    int pin;
    int state;
};

class AccelStepper;
struct motor_t {
    int dirpin;
    int speedpin;
    volatile float speed;
    volatile float speed_go;
    volatile float steps;
    volatile float spire_final;
    volatile float layer_steps;
    volatile float count_spire;
    String name;
    volatile int dirty;
    volatile float togo;
    volatile bool pause;
    AccelStepper* stepper;
};

extern input_t Pins[];

void taskMenu(void*);
size_t pins_count();

class locker_t {
  public:
    locker_t();
    ~locker_t();
};


class disp_locker_t {
  public:
    disp_locker_t();
    ~disp_locker_t();
};

struct cel_t {
    int idx;
    int x;
    int y;
    String txt;
    String out;
};

struct screen_t {
    cel_t count;
    cel_t spire;
    cel_t wire;
    cel_t length;
    cel_t thread;
    cel_t steps;
    cel_t start;
    cel_t mot_speed;
    cel_t round;
    int sp_count;
};

struct coil_t {
    uint8_t magic;
    float spire;
    float len_mm;
    float wire_mm;
    float thread_mm;
    bool start;
    size_t mot_speed;
    float guider_steps;
};

void show_turns(int turns,bool locked);
void screen_it(int curent = 0, int sel = 0);
