
#include "menu.h"

#define BACK 0
#define DOWN 1
#define UP 2
#define ENTER 3
#define BOB_SPEED 800.0
#define SET_FILE "/settings"
#define count_of(xx) sizeof(xx) / sizeof(xx[0])

const int ledPin = 5;
extern int Bottomy;
extern int MenuSel;
extern motor_t Motor[];
extern screen_t Screen;
extern QueueHandle_t queue;
static int Mindex = 0;
char Details[128];
typedef void (*call_back)(int);

input_t Pins[] = {
  { 36, 0 },
  { 39, 0 },
  { 34, 0 },
  { 35, 0 }
};

coil_t Coil = { 0x2, 500, 10, 0.1, 1.0, false, 1500 };


struct menu_t {
  char* text;
  call_back functor;
};

ESPFlash<coil_t>* pSettings;  //("/settings.cfg");

void show_text(const char* t, float arg);
static void main_menu_nav(int b);
void set_spire(int b);
void set_length(int b);
void set_wirediam(int b);
void set_thread_step(int b);
void start_stop(int b);
void ctrl_spinA(int b);
void ctrl_spinB(int b);
void mot_speed(int b);
void  motor_run(int direction, int motor, int spins);

menu_t main_menu[] = {
  { "Start Stop : ", start_stop },
  { "Spire final: ", set_spire },
  { "Coil length: ", set_length },
  { "Wire Diam  : ", set_wirediam },
  { "Guider Step: ", set_thread_step },
  { "Coiler Ctrl: ", ctrl_spinA },
  { "Guider Ctrl: ", ctrl_spinB },
  { "M1 Speed  : ", mot_speed },
  { "Spire      : ", nullptr },
};

call_back current_cb = main_menu_nav;
static void save()
{
    int layer = (int)(Coil.len_mm/Coil.wire_mm);
    int spire = (int)Coil.spire;
    if(spire % layer)
    {
        Coil.spire += layer - spire % layer ;
    }
    if (!pSettings->set(Coil)) {
      Serial.println("failed ot save Coil");
      digitalWrite(ledPin, HIGH);
    }
}




void start_stop(int b) {
  locker_t l;
  Serial.printf("%s b=%d\n", __FUNCTION__, b);
  Screen.start.out = Screen.start.txt;
  if (b == ENTER) {
    Screen.sp_count = 0;
    Coil.start = true;
    Motor[CM].speed = Coil.mot_speed;
    Motor[CM].steps = ONE_SPIN * Coil.spire;
    
    Motor[CM].layer_steps = ONE_SPIN * (Coil.len_mm / Coil.wire_mm);
    int left0ver = (int)Coil.spire % (int)Motor[CM].layer_steps;

    Motor[CM].spire_final = Coil.spire - left0ver;

    Motor[GM].togo = ONE_SPIN * (Coil.len_mm / Coil.thread_mm);
    Motor[GM].steps = Motor[GM].togo + ONE_SPIN;
    Motor[GM].speed = (Coil.mot_speed * (Coil.wire_mm / Coil.thread_mm));

    Serial.printf("steps:2.2f, Gspeed=2.2f, blen:2.2f wire:2.2f layersteps=2.2f \n", Motor[CM].steps,Motor[GM].speed,
                  Coil.len_mm,
                  Coil.wire_mm,
                  Motor[CM].layer_steps);

    sprintf(Details, "Layer 0os: %2.2f", Motor[CM].layer_steps/ONE_SPIN);
    Motor[CM].dirty++;
    
    Screen.start.out += "oooo";
   

  } else if (b == BACK) {
    Coil.start = false;
    Motor[1].speed = 0;
    Motor[1].steps = 0;
    Motor[0].speed = 0;
    Motor[0].steps = 0;
    Motor[1].dirty++;
    Motor[0].dirty++;
    Screen.start.out += "STOPPED";
    Screen.count.txt = "0";
    current_cb = main_menu_nav;
  } else if (b == UP || b == DOWN) 
  {
    Motor[CM].dirty++;
    Motor[CM].pause=true;
  }
  Serial.printf("Motor-0 %s conf=%d %d\n", Motor[0].name, Motor[0].speed, Motor[0].steps);
  Serial.printf("Motor-1 %s conf=%d %d\n", Motor[1].name, Motor[1].speed, Motor[1].steps);
  screen_it(Screen.start.idx, Screen.start.idx);
}


void set_spire(int b) {
  
  float twolayers = 2.0 * (Coil.len_mm / Coil.wire_mm);
  Serial.printf("spire = %2.2f\n",twolayers);
  if (b == UP)
    Coil.spire += twolayers;
  else if (b == DOWN) {
    if (Coil.spire > twolayers)
      Coil.spire -= twolayers;
  } else if (b == BACK) {
    save();
    Serial.println("Coil saved");
    current_cb = main_menu_nav;
  }


  Screen.spire.out = Screen.spire.txt;
  Screen.spire.out += String(Coil.spire);
  screen_it(Screen.spire.idx, Screen.spire.idx);
}


void set_length(int b) {
  Serial.println(__FUNCTION__);
  if (b == UP)
    Coil.len_mm += 0.5;
  else if (b == DOWN) {
    if (Coil.len_mm > 1)
      Coil.len_mm -= 0.5;
  } else if (b == BACK) {
    current_cb = main_menu_nav;
    save();
  }

  Screen.length.out = Screen.length.txt;
  Screen.length.out += String(Coil.len_mm);
  screen_it(Screen.length.idx, Screen.length.idx);
}

void set_wirediam(int b) {
  Serial.println(__FUNCTION__);
  if (b == UP)
    Coil.wire_mm += .01;
  else if (b == DOWN) {
    if (Coil.wire_mm > 0.01)
      Coil.wire_mm -= .01;
  } else if (b == BACK) {
    save();

    current_cb = main_menu_nav;
  }
  Screen.wire.out = Screen.wire.txt;
  Screen.wire.out += String(Coil.wire_mm);
  screen_it(Screen.wire.idx, Screen.wire.idx);
}

void set_thread_step(int b) {
  Serial.println(__FUNCTION__);
  if (b == UP)
    Coil.thread_mm += 0.05;
  else if (b == DOWN) {
    if (Coil.thread_mm > 0.1)
      Coil.thread_mm -= 0.05;
  } else if (b == BACK) {
    save();
    current_cb = main_menu_nav;
  }

  Screen.thread.out = Screen.thread.txt;
  Screen.thread.out += String(Coil.thread_mm);
  screen_it(Screen.thread.idx, Screen.thread.idx);
}

void ctrl_spinA(int b) {
  Serial.println(__FUNCTION__);
  locker_t l;
  if (b == UP) {
    motor_run(1, CM,ONE_SPIN);
  } else if (b == DOWN) {
    motor_run(-1, CM,ONE_SPIN);
  } else if (b == BACK) {
    current_cb = main_menu_nav;
  }
  Screen.ctrlA.out = Screen.ctrlA.txt;
  Screen.ctrlA.out += "UP/DN";
  screen_it(Screen.ctrlA.idx, Screen.ctrlA.idx);
}

void ctrl_spinB(int b) {
  Serial.println(__FUNCTION__);
  locker_t l;
  if (b == UP) {
    motor_run(1, GM, ONE_SPIN);
  } else if (b == DOWN) {
    motor_run(-1, GM, ONE_SPIN);
  } else if (b == BACK) {
    current_cb = main_menu_nav;
  }
  Screen.ctrlB.out = Screen.ctrlB.txt;
  Screen.ctrlB.out += "UP/DN";
  screen_it(Screen.ctrlB.idx, Screen.ctrlB.idx);
}


void mot_speed(int b) {
  int c=0;  
  if (b == UP) {
    Coil.mot_speed += 200;
    c=1;
  } else if (b == DOWN) {
    if (Coil.mot_speed > 200){
      Coil.mot_speed -= 200;
      c=1;
    }
  } else if (b == BACK) {
    current_cb = main_menu_nav;
    save();
  }
  Screen.mot_speed.out = Screen.mot_speed.txt;
  Screen.mot_speed.out += String(Coil.mot_speed);
  screen_it(Screen.mot_speed.idx, Screen.mot_speed.idx);
  if(c)
  {
    motor_run(1, GM, ONE_SPIN);
    motor_run(-1, GM, ONE_SPIN);
    Serial.println("stopped");
  }
}

void  motor_run(int direction, int motor, int spins)
{
      Motor[motor].stepper->setMaxSpeed(Coil.mot_speed);
      Motor[motor].stepper->setAcceleration(ACCEL);
      
      Serial.printf("%d spining\n",motor);
      Motor[motor].stepper->setCurrentPosition(0);
      Motor[motor].stepper->setSpeed(Coil.mot_speed);
      Motor[motor].stepper->moveTo((4*ONE_SPIN) * direction);
      Motor[motor].stepper->runToPosition();
      Serial.printf(" %d stopping\n",motor);
      Motor[motor].stepper->stop();
      Motor[motor].stepper->setCurrentPosition(0);
}

void main_menu_nav(int b) {
  Serial.printf("%s b=%d\n", __FUNCTION__, b);
  if (b == UP) {
    Mindex--;
    if (Mindex < 0)
      Mindex = count_of(main_menu) - 1;
  } else if (b == DOWN) {
    Mindex++;
    if (Mindex == 8)
      Mindex = 0;
  } else if (b == ENTER) {
    current_cb = main_menu[Mindex].functor;
    current_cb(-1);
    return;
  }
  Serial.printf("%s but=%d Mindex=%d\n", __FUNCTION__, b, Mindex);
  if (current_cb == main_menu_nav) {
    screen_it(Mindex, -1);
  }
}

size_t pins_count() {
  return sizeof(Pins) / sizeof(Pins[0]);
}

static void click(int i) {
  Serial.print("click ");
  Serial.println(i);
  if (current_cb) {
    disp_locker_t l;
    current_cb(i);
  }
}

void taskMenu(void*) 
{
  int turns;

  esp_task_wdt_init(30,false);
  esp_task_wdt_add(NULL); 
  esp_task_wdt_reset();


  Serial.println("taskMenu");
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  if (!SPIFFS.begin()) {
    Serial.println("For   matting SPIFFS. Will take a while...");
    SPIFFS.format();
  }
  pSettings = new ESPFlash<coil_t>("/settings.cfg");

  coil_t loco = { 0, 0, 0, 0, 0 };
  loco = pSettings->get();
  if (loco.magic != Coil.magic) {
    Serial.println("Coil failed to load, saving");

    if (false == pSettings->set(Coil)) {
      Serial.println("Coil failed to save first time");
      digitalWrite(ledPin, HIGH);
    } else {
      Coil = pSettings->get();
      Serial.printf("Loaded: coil.spire = 2.2f\n", Coil.spire);
    }
  } else {
    Serial.println("Setting loaded Okay");
    Coil = loco;
  }

  int depart = 14;
  int mistep = 14;

  Screen.start.idx = 0;
  Screen.start.x = 6;
  Screen.start.y = depart;
  Screen.start.txt = main_menu[0].text;
  depart += mistep;

  Screen.spire.idx = 1;
  Screen.spire.x = 6;
  Screen.spire.y = depart;
  Screen.spire.txt = main_menu[1].text;
  depart += mistep;

  Screen.length.idx = 2;
  Screen.length.x = 6;
  Screen.length.y = depart;
  Screen.length.txt = main_menu[2].text;
  depart += mistep;

  Screen.wire.idx = 3;
  Screen.wire.x = 6;
  Screen.wire.y = depart;
  Screen.wire.txt = main_menu[3].text;
  depart += mistep;

  Screen.thread.idx = 4;
  Screen.thread.x = 6;
  Screen.thread.y = depart;
  Screen.thread.txt = main_menu[4].text;
  depart += mistep;


  Screen.ctrlA.idx = 5;
  Screen.ctrlA.x = 6;
  Screen.ctrlA.y = depart;
  Screen.ctrlA.txt = main_menu[5].text;
  depart += mistep;

  Screen.ctrlB.idx = 6;
  Screen.ctrlB.x = 6;
  Screen.ctrlB.y = depart;
  Screen.ctrlB.txt = main_menu[6].text;
  depart += mistep;

  Screen.mot_speed.idx = 7;
  Screen.mot_speed.x = 6;
  Screen.mot_speed.y = depart;
  Screen.mot_speed.txt = main_menu[7].text;

  depart += mistep;

  Screen.count.idx = 8;
  Screen.count.x = 8;
  Screen.count.y = depart;
  Screen.count.txt = main_menu[8].text;

  Bottomy = mistep + 5;

  Screen.count.out = Screen.count.txt;
  Screen.count.out += "0";

  Screen.ctrlA.out = Screen.ctrlA.txt;
  Screen.ctrlA.out += "OFF";

  Screen.ctrlB.out = Screen.ctrlB.txt;
  Screen.ctrlB.out += "OFF";

  Screen.length.out = Screen.length.txt;
  Screen.length.out += String(Coil.len_mm);

  Screen.spire.out = Screen.spire.txt;
  Screen.spire.out += String(Coil.spire);

  Screen.start.out = Screen.start.txt;
  Screen.start.out += "OFF";

  Screen.thread.out = Screen.thread.txt;
  Screen.thread.out += String(Coil.thread_mm);

  Screen.mot_speed.out = Screen.mot_speed.txt;
  Screen.mot_speed.out += String(Coil.mot_speed);

  Screen.wire.out = Screen.wire.txt;
  Screen.wire.out += String(Coil.wire_mm);
  esp_task_wdt_add(NULL); 
  esp_task_wdt_reset();
  
  delay(100);
  click(0);
  while (1) {
    esp_task_wdt_reset(); 
    for (int i = 0; i < sizeof(Pins) / sizeof(Pins[0]); i++) {
      int dr = digitalRead(Pins[i].pin);
      if (dr != Pins[i].state) {

        Pins[i].state = dr;
        if (dr == 1) {
          digitalWrite(ledPin, LOW);
          click(i);
        }
      }
    }
    delay(100);
    scope__ {
      disp_locker_t l;
      if (xQueueReceive(queue, &turns, 1)) {
        Screen.count.out = Screen.count.txt;
        Screen.count.out += turns;
        screen_it(Screen.count.idx, MenuSel);
      }
    }
  }
  
}
