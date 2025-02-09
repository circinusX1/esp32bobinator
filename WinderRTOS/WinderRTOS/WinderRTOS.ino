

#include <Arduino.h>


#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebSrv.h>
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library for ST7735
#include <SPI.h>

#include "menu.h"



#define TFT_CS 5    // Chip select control pin
#define TFT_DC 2    // Data Command control pin
#define TFT_RST 15  // Reset pin (could connect to RST pin)
//#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18

//#define SPI_FREQUENCY 27000000
#define SPI_FREQUENCY 24000000


Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
AsyncWebServer server(80);

const char* ssid = "marius1";
const char* password = "zoomahia1";

const char* PARAM_MESSAGE = "message";

void notFound(AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
}

motor_t Motor[] = {
    { 14, 27, 0, 0, 0, 0, "GUID", 0, 0, false, nullptr },
    { 25, 26, 0, 0, 0, 0, "COIL", 0, 0, false, nullptr },

};

AccelStepper stepper = AccelStepper(1, Motor[0].speed, Motor[0].dir);
AccelStepper stepper1 = AccelStepper(1, Motor[1].speed, Motor[1].dir);



screen_t Screen;
QueueHandle_t queue = xQueueCreate(10, sizeof(int));
SemaphoreHandle_t xMutex = NULL;
SemaphoreHandle_t dMutex = NULL;
int MenuSel = -1;
int Bottomy = 0;
extern char Details[128];
extern coil_t Coil;

void taskMotor2(void*);
void taskMotor3(void*);

locker_t::locker_t() {
    xSemaphoreTake(xMutex, portMAX_DELAY);
}

locker_t::~locker_t() {
    xSemaphoreGive(xMutex);
}


disp_locker_t::disp_locker_t() {
    xSemaphoreTake(dMutex, portMAX_DELAY);
}

disp_locker_t::~disp_locker_t() {
    xSemaphoreGive(dMutex);
}

void fs_setup() {

#if 0
  esp_vfs_littlefs_conf_t conf = {
    .base_path = "/xxx",
    .partition_label = "littlefs",
    .format_if_mount_failed = true,
    .dont_mount = false,
  };
  esp_err_t ret = esp_vfs_littlefs_register(&conf);
  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      Serial.println("Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      Serial.println("Failed to find LittleFS partition");
    } else {
      Serial.print("Failed to initialize LittleFS ");
      Serial.println(esp_err_to_name(ret));
    }
    return;
  }
  size_t total; size_t used;
  ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
            Serial.printf("Failed to get LittleFS partition information (%s)\n", esp_err_to_name(ret));
    }
    else
    {
            Serial.printf("Partition size: total: %d, used: %d", total, used);
    }
#endif
}

void setup() {
    Serial.begin(115200);



    int BootReason = esp_reset_reason();
    if (BootReason == 1) {  // Reset due to power-on event.
        Serial.println("Reboot was because of Power-On!!");
    }

    if (BootReason == 6) {  // Reset due to task watchdog.
        Serial.println("Reboot was because of WDT!!");
    }

    uint32_t size_flash_chip = 0;
    esp_flash_get_size(NULL, &size_flash_chip);
    fs_setup();
    Serial.print("Flash size ");
    Serial.println(size_flash_chip);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "Hello, world");
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest* request) {
        String message;
        if (request->hasParam(PARAM_MESSAGE)) {
            message = request->getParam(PARAM_MESSAGE)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, GET: " + message);
    });

    // Send a POST request to <IP>/post with a form field message set to <message>
    server.on("/post", HTTP_POST, [](AsyncWebServerRequest* request) {
        String message;
        if (request->hasParam(PARAM_MESSAGE, true)) {
            message = request->getParam(PARAM_MESSAGE, true)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, POST: " + message);
    });

    server.onNotFound(notFound);
    server.begin();

    for (int i = 0; i < pins_count(); i++) {
        pinMode(Pins[i].pin, INPUT);
        Pins[i].state = digitalRead(Pins[i].pin);
        Serial.printf("Pin:%d state:%d\r\n", i, Pins[i].state);
    }


    ////////////////////////////////////////////////////////

    tft.initR(INITR_BLACKTAB);
    tft.fillScreen(ST77XX_BLACK);
    int w = tft.width();
    int h = tft.height();
    Serial.print("width =");
    Serial.println(w);
    Serial.print("height =");
    Serial.println(h);
    tft.setRotation(2);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1);

    xMutex = xSemaphoreCreateMutex();
    dMutex = xSemaphoreCreateMutex();

    Motor[CM].stepper = &stepper;
    Motor[GM].stepper = &stepper1;

    esp_task_wdt_init(30, false);



    xTaskCreate(
        taskMenu,    /* Task function. */
        "taskMenu1", /* String with name of task. */
        8192,        /* Stack size in bytes. */
        Motor,       /* Parameter passed as input of the task */
        1,           /* Priority of the task. */
        0);
}

void show_turns(int turns) {
    disp_locker_t d;
    xQueueSendToBack(queue, &turns, portMAX_DELAY);
}

void screen_it(int curent, int sel) {
    MenuSel = sel;
    //Serial.printf("curent=%d == se=%d\n", curent,sel);
    auto lprint = [&](const cel_t& cel) {
        int x = cel.x;
        if (cel.idx == sel) {
            tft.fillRect(x - 1, cel.y - 2, 128 - x - 1, 14, ST77XX_BLACK);
            tft.drawRect(x - 1, cel.y - 2, 128 - x - 1, 14, ST77XX_WHITE);
            tft.setTextColor(ST77XX_WHITE);
            x += 10;
        } else if (curent == cel.idx) {
            tft.drawRect(x - 1, cel.y - 2, 128 - x - 1, 14, ST77XX_YELLOW);
            tft.setTextColor(ST77XX_YELLOW);
        } else {
            tft.setTextColor(ST77XX_GREEN);
        }

        tft.setCursor(x, cel.y);
        tft.print(cel.out);
    };

    if (Screen.count.idx != curent)
        tft.fillScreen(0x0000);
    else
        tft.fillRect(Screen.count.x, Screen.count.y - 2, 128, 22, 0);
    lprint(Screen.start);
    lprint(Screen.spire);
    lprint(Screen.length);
    lprint(Screen.wire);
    lprint(Screen.thread);
    lprint(Screen.ctrlA);
    lprint(Screen.ctrlB);
    lprint(Screen.mot_speed);
    lprint(Screen.count);

    tft.setTextColor(0xFFFF);
    tft.setCursor(5, tft.height() - 20);
    tft.print(Details);
}

void loop() {
    esp_task_wdt_init(30, false);
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();

    taskMotor2(0);
}

static void m_prep(int motor, float speed, float accel) {
    Motor[motor].stepper->setMaxSpeed(abs(speed) + 10);
    Motor[motor].stepper->setAcceleration(ACCEL);
    Motor[motor].stepper->setSpeed(speed);
}

static void m_stop(int motor) {
    Motor[motor].stepper->setCurrentPosition(0);
    Motor[motor].stepper->stop();
}

static void m_go(int motor, float speed, float accel, float steps) {
    int c = 0;
    Serial.printf("[%d] going to %2.2f with speed:%2.2f\n", motor, steps, speed);
    Motor[motor].stepper->setCurrentPosition(0);
    Motor[motor].stepper->moveTo(steps);
    while (Motor[motor].stepper->run()) {
        if (++c % 1000) {
            esp_task_wdt_reset();
        }
    }
    Serial.printf("[%d] reached %2.2f with speed:%2.2f\n", steps, speed);
    m_stop(motor);
}

volatile float Goffset = 0;
void taskMotor2(void* pm) {
    int CHUNK = 5;
    bool gswitch = false;
    int dirty = 0;
    int gdirty = 0;
    float speed = 0, steps = 0;
    float gspeed = 0, gsteps = 0;
    Motor[CM].stepper->setMaxSpeed(Motor[CM].speed);
    Motor[GM].stepper->setMaxSpeed(Motor[GM].speed);
    Motor[CM].stepper->setAcceleration(Motor[CM].speed);
    Motor[GM].stepper->setAcceleration(Motor[GM].speed);

    while (1) {
        delay(100);
        esp_task_wdt_reset();
        scope__ {
            locker_t l;
            if (dirty != Motor[CM].dirty) {

                dirty = Motor[CM].dirty;
                gdirty = Motor[GM].dirty;
                speed = Motor[CM].speed;
                steps = Motor[CM].steps;

                gspeed = Motor[GM].speed;
                gsteps = Motor[GM].togo;
                Serial.println("-----------------------------------------------------");
                Serial.printf("THR: %s S:%2.2f V:%2.2f \n", Motor[CM].name, steps / ONE_SPIN, speed);
                Serial.printf("THR: Layer: %2.2f\n", Motor[CM].layer_steps / ONE_SPIN);
                Serial.println("-----------------------------------------------------");
            }
        }
        if (speed == 0) {
            continue;
        }
        Motor[CM].stepper->setMaxSpeed(speed);
        Motor[GM].stepper->setMaxSpeed(speed);

        Motor[CM].stepper->setAcceleration(ACCEL);
        Motor[GM].stepper->setAcceleration(ACCEL);

        Motor[CM].stepper->setCurrentPosition(0);
        Motor[CM].stepper->setSpeed(speed);
        Motor[GM].stepper->setCurrentPosition(0);
        Motor[GM].stepper->setSpeed(speed);

        float shot = ONE_SPIN * 5;
        float gshot = shot * (gspeed / speed);
        float cursteps = 0;
        float prevsteps = 0;
        float goffpos = 0;
        Serial.printf("going to run %d steps in shots=%2.2f / %2.2f \n", steps, shot, gshot);
        while (cursteps <= steps) {
            esp_task_wdt_reset();
            Motor[CM].stepper->runToNewPosition(shot);
            Motor[GM].stepper->runToNewPosition(gshot);
            goffpos += gshot;
            cursteps += (ONE_SPIN * CHUNK);
            Serial.printf("-----stepped 5, offpos = %2.2f\n", goffpos);
            if (cursteps - prevsteps > Motor[CM].layer_steps) {
                gshot = -gshot;
                speed = -speed;
                Serial.println("inverting guider");
                Motor[GM].stepper->setSpeed(speed);
                prevsteps = cursteps;
                Serial.printf("curstep = %2.2f of %2.2f \n", cursteps, steps);
            }
            Motor[0].count_spire += CHUNK;
            show_turns(Motor[0].count_spire);
            Motor[GM].stepper->setCurrentPosition(0);
            Motor[CM].stepper->setCurrentPosition(0);
            scope__ {
                locker_t l;
                if (Motor[CM].dirty != dirty) {
                    dirty = Motor[CM].dirty;
                    if (Motor[CM].pause) {
                        Serial.println("paused");
                        Motor[CM].stepper->stop();
                        dirty = Motor[CM].dirty;
                        vTaskDelay(3000);
                        Motor[CM].stepper->setSpeed(speed);
                        Motor[CM].pause = false;
                    } else if (Coil.start == false) {
                        Serial.println("broken");
                        cursteps = steps + 1;
                    }
                }
            }
        }
        Serial.printf("going home = %2.2f\n", goffpos);
        Motor[GM].stepper->setCurrentPosition(0);
        if (goffpos) {
            Motor[GM].stepper->setSpeed(-speed);
            Motor[GM].stepper->runToNewPosition(-goffpos);
        }
        Serial.println("motors stopped ");
        Motor[CM].stepper->stop();
        Motor[CM].speed = Motor[CM].steps = 0;
        Motor[GM].stepper->stop();
        Motor[GM].speed = Motor[CM].steps = 0;
        gspeed = speed = 0;
        gsteps = steps = 0;
        gdirty = Motor[GM].dirty;
        dirty = Motor[CM].dirty;
        esp_task_wdt_reset();
    }
}

