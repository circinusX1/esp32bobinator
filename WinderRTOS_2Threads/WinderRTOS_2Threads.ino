

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
#define COILER Motor[CM]
#define GUIDER Motor[GM]

//#define SPI_FREQUENCY 27000000
#define SPI_FREQUENCY 24000000


Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
AsyncWebServer server(80);

const char* ssid = "marius1";
const char* password = "zoomahia1";
const char* PARAM_MESSAGE = "message";
static volatile float Goffset = 0;
static volatile float CoilCurpos;
static volatile float CoilTogo;

void notFound(AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
}

/*
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
*/

motor_t Motor[] = {
    { 14, 27, 0, 0, 0, 0, 0, 0, "GUID", 0, 0, false, nullptr },
    { 25, 26, 0, 0, 0, 0, 0, 0, "COIL", 0, 0, false, nullptr },

};

AccelStepper stepper = AccelStepper(1, Motor[0].speedpin, Motor[0].dirpin);
AccelStepper stepper1 = AccelStepper(1, Motor[1].speedpin, Motor[1].dirpin);



screen_t Screen;
QueueHandle_t queue = xQueueCreate(10, sizeof(int));
SemaphoreHandle_t xMutex = NULL;
SemaphoreHandle_t dMutex = NULL;
int MenuSel = -1;
int Bottomy = 0;
extern char Details[128];
extern coil_t Coil;

void taskMotor(void*);
void taskGuider(void*);

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

    COILER.stepper = &stepper;
    GUIDER.stepper = &stepper1;

    esp_task_wdt_init(30, false);

    if (LIMITER) {
        pinMode(LIMITER, INPUT_PULLUP);
        Serial.printf("Limiter=%d\n", digitalRead(LIMITER));
    }

    xTaskCreate(
        taskMenu,    /* Task function. */
        "taskMenu1", /* String with name of task. */
        8192,        /* Stack size in bytes. */
        Motor,       /* Parameter passed as input of the task */
        1,           /* Priority of the task. */
        0);

    xTaskCreate(
        taskGuider,   /* Task function. */
        "taskGuider", /* String with name of task. */
        8192,         /* Stack size in bytes. */
        Motor,        /* Parameter passed as input of the task */
        1,            /* Priority of the task. */
        0);
}

void show_turns(int turns, bool l) {
    if (!l) {
        disp_locker_t d;
        xQueueSendToBack(queue, &turns, portMAX_DELAY);
    } else {
        xQueueSendToBack(queue, &turns, portMAX_DELAY);
    }
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
            tft.fillRect(x - 1, cel.y - 2, 128 - x - 1, 14, ST77XX_BLUE);
            tft.drawRect(x - 1, cel.y - 2, 128 - x - 1, 14, ST77XX_BLUE);
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
    lprint(Screen.steps);
    lprint(Screen.thread);
    lprint(Screen.mot_speed);
    lprint(Screen.round);
    lprint(Screen.count);

    tft.setTextColor(0xFFFF);
    tft.setCursor(5, tft.height() - 20);
    tft.print(Details);
}

void loop() {
    taskMotor(0);
}

static void m_prep(int motor) {
    locker_t l;
    Motor[motor].stepper->setMaxSpeed(Motor[motor].speed);
    Motor[motor].stepper->setAcceleration(ACCEL);
    vTaskDelay(32);
    Motor[motor].stepper->setSpeed(COILER.speed);
    vTaskDelay(32);
}

static void m_stop(int motor) {
    locker_t l;
    Motor[motor].dirty++;
    Motor[motor].stepper->setCurrentPosition(0);
    vTaskDelay(32);
    Motor[motor].stepper->stop();
    vTaskDelay(32);
}

static void m_go(int motor, float steps, float speed) {
    int c = 0;
    Serial.printf("MOTOR[%d] runto: %2.2f, speed:%2.2f\n", motor, steps, speed);
    Motor[motor].stepper->setCurrentPosition(0);
    vTaskDelay(128);
    Motor[motor].stepper->setMaxSpeed(speed);
    vTaskDelay(32);
    Motor[motor].stepper->setAcceleration(ACCEL);
    vTaskDelay(32);
    Motor[motor].stepper->setSpeed(speed);
    vTaskDelay(32);
    Motor[motor].stepper->moveTo(steps);
    vTaskDelay(32);
    while (Motor[motor].stepper->run())
    {
        if (++c % 4000000 == 0) {
            esp_task_wdt_reset();
        }
    }
    Serial.printf("Motor[%d] gotto %2.2f speed:%2.2f\n", motor, steps, speed);
    m_stop(motor);
}

volatile static size_t Rolling = false;


void gm_gohome()
{
    if (digitalRead(LIMITER) == HIGH)
    {
        Serial.printf("Bringing home %2.2f\n", Goffset);
        int max=320;
        while (digitalRead(LIMITER) == HIGH && max-->0)
        {
            m_go(GM, -5, -SPEED / 10);
            vTaskDelay(32);
            esp_task_wdt_reset();
        }
        Goffset = 0;
        m_stop(GM);
    }

}


void taskGuider(void*) {
    int dirty = 0;
    float speed = 0;
    float steps = 0;
    size_t rolling = false;

    vTaskDelay(1000);
    Serial.println("Starting task guider");
    esp_task_wdt_init(30, false);
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();
    m_prep(GM);
    m_stop(GM);

    int lim = digitalRead(LIMITER);
    dirty = GUIDER.dirty;
    while (1) {
        vTaskDelay(128);
        esp_task_wdt_reset();
        scope__ {
            locker_t l;
            if (GUIDER.dirty != dirty) {
                dirty = GUIDER.dirty;
                speed = GUIDER.speed_go;
                steps = GUIDER.steps;
                Serial.printf(" ->> Guider: turns %2.2f \n", steps / ONE_SPIN);
                Serial.printf("   ->> Guider: speed %2.2f \n", speed);
                rolling = Rolling;
            }
        }
        if (digitalRead(LIMITER) != lim) {
            Serial.printf("Limiter=%d\n", lim = digitalRead(LIMITER));
        }
        if (steps == 0 || rolling == false) {
            vTaskDelay(8);
            continue;
        }
        Serial.printf("        ->>> Guiding wire now at %2.2f speed\n", speed);
        m_go(GM, steps, speed);
        Goffset += steps;

        scope__ {
            locker_t l;
            rolling = Rolling;
        }
        if (rolling) {
            Serial.println("Guiding wire reversing by its own");
            steps = -steps;
            speed = -speed;
            dirty = GUIDER.dirty;
            vTaskDelay(64);
            continue;
        }
        m_stop(GM);
        rolling = false;
        steps = 0;
        Serial.println("Guiding wire stopped");
    }
}

void taskMotor(void* pm) {
    int dirty = 0;
    float speed = 0;
    float spire_steps = 0;
    float shot = 0;
    int spire = 0;
    vTaskDelay(1000);

    Serial.println("Starting task motor");
    esp_task_wdt_init(30, false);
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();
    m_prep(CM);
    m_stop(CM);
    m_stop(GM);

    gm_gohome();

    m_prep(CM);
    m_stop(CM);
    m_stop(GM);
    dirty = COILER.dirty;

    while (1) {
        vTaskDelay(512);
        esp_task_wdt_reset();
        spire_steps = 0;
        scope__ {
            locker_t l;
            if (dirty != COILER.dirty) {
                Rolling = true;
                GUIDER.speed_go = GUIDER.speed;
                COILER.speed_go = COILER.speed;
                dirty = COILER.dirty;
                speed = COILER.speed_go;
                CoilTogo = spire_steps = COILER.steps;
                CoilCurpos = 0;
                spire = 0;
                shot = COILER.layer_steps;
                COILER.count_spire = spire;
                show_turns(COILER.count_spire, false);
                Serial.println("---------------------------------------------");
                Serial.printf("Coiler: spire %2.2f \n", spire_steps / ONE_SPIN);
                Serial.printf("Coiler: layer %2.2f \n", shot / ONE_SPIN);
                Serial.printf("Coiler: speed %2.2f steps %2.2f \n", speed, spire_steps);
                Serial.printf("Guider: speed %2.2f \n", GUIDER.speed_go);
                GUIDER.dirty++;
            }
        }
        if (spire_steps <= 0) {
            spire_steps = 0;
            vTaskDelay(256);
            continue;
        }

        show_turns(spire, false);
        Serial.printf("Bobinating pos=%2.2f of %2.2f\n", CoilCurpos, CoilTogo);
        Serial.printf("Bobinating pos=%2.2f of %2.2f\n", CoilTogo, CoilCurpos);

        show_turns(Motor[CM].stepper->currentPosition()/ONE_SPIN,false);

        m_go(CM, spire_steps/4, speed);
        show_turns(Motor[CM].stepper->currentPosition()/ONE_SPIN,false);

        m_go(CM, spire_steps/4, speed);
        show_turns(Motor[CM].stepper->currentPosition()/ONE_SPIN,false);

        m_go(CM, spire_steps/4, speed);
        show_turns(Motor[CM].stepper->currentPosition()/ONE_SPIN,false);

        m_go(CM, spire_steps/4, speed);
        show_turns(Motor[CM].stepper->currentPosition()/ONE_SPIN,false);

        vTaskDelay(1024);
        scope__ {
            locker_t lock;
            Rolling = false;
            vTaskDelay(16);
        }
        m_stop(CM);
        m_stop(GM);
        vTaskDelay(128);
        GUIDER.dirty++;
        esp_task_wdt_reset();
        vTaskDelay(128);
        gm_gohome();

        dirty = COILER.dirty;
        COILER.count_spire = spire;
        show_turns(COILER.count_spire, false);
        spire_steps = 0;
    }
}
