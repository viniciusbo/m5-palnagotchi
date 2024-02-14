#include <M5Cardputer.h>
#include <M5Unified.h>
#include <WiFi.h>

#define ARROW_UP ';'
#define ARROW_DOWN '.'

#define PID_1 = "Detect pwnagotchi"
#define PID_2 = "Make friendship"

int mcursor = 0;
int menu_size = 2;
String const menu[] = { "Detect pwnagotchi", "Make friendship" };

uint8_t pwngrid_signature_addr[] = { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad };
uint8_t pwngrid_broadcast_addr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

bool check_prev_press() {
  if (M5Cardputer.Keyboard.isKeyPressed(ARROW_UP)) {
    return true;
  }

  return false;
}

bool check_next_press() {
  if (M5Cardputer.Keyboard.isKeyPressed(ARROW_DOWN)) {
    return true;
  }

  return false;
}

bool check_select_press() {
  if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) || M5Cardputer.BtnA.wasClicked()) {
    return true;
  }

  return false;
}

void draw_menu() {
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setCursor(0, 10, 1);

  for (int i = 0; i < menu_size; i++) {
    if (mcursor == i) {
      M5Cardputer.Display.print("> ");
    } else {
      M5Cardputer.Display.print("  ");
    }

    M5Cardputer.Display.println(menu[i]);
  }
}

void run(int pid) {
  switch (pid) {
    case 1:
      
  }
}

void menu_loop() {
  if (check_next_press()) {
    mcursor = (mcursor + 1) % menu_size;
    draw_menu();
    delay(250);
  }

  if (check_prev_press()) {
    mcursor = mcursor - 1;
    
    if (mcursor < 0) {
      mcursor = menu_size - 1;
    }
    
    draw_menu();
    delay(250);
  }

  if (check_select_press()) {
    run(mcursor);
  }
}

int current_channel = 1;
int channel_size = 12;

void channel_hop() {
  int channel = (current_channel + 1) % channel_size;
  if (channel == 0) {
    channel = 1;
  }

  int hidden = 1;
  WiFi.softAP("{name:\"Palnagotchi\",identity:\"32e9f315e92d974342c93d0fd952a914bfb4e6838953536ea6f63d54db6b9610\",session_id:\"c7:87:3e:b0:c1:f1\",grid_version:\"1.10.3\",epoch:999999,face=\"(x_x)\"}", "", channel, hidden);
  delay(100);
}

void setup() {
  // M5init
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextDatum(middle_center);
  M5Cardputer.Display.setTextFont(&fonts::Orbitron_Light_32);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setRotation(1);
  
  // Wifi init
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAPmacAddress(pwngrid_signature_addr);

  // App init
  draw_menu();
}

void loop() {
  M5Cardputer.update();
  menu_loop();
  channel_hop();
}
