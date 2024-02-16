#include "ArduinoJson.h"
#include "M5Cardputer.h"
#include "M5Unified.h"
// #include "esp_system.h"
#include "pwngrid.h"
// #include "esp_event.h"
// #include "esp_event_loop.h"

// #define ARROW_UP ';'
// #define ARROW_DOWN '.'

// bool check_prev_press() {
//   if (M5Cardputer.Keyboard.isKeyPressed(ARROW_UP)) {
//     return true;
//   }
//
//   return false;
// }
//
// bool check_next_press() {
//   if (M5Cardputer.Keyboard.isKeyPressed(ARROW_DOWN)) {
//     return true;
//   }
//
//   return false;
// }

void init_m5() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextDatum(middle_center);
  M5Cardputer.Display.setTextFont(&fonts::FreeSans9pt7b);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.drawString("Palnagotchi", M5Cardputer.Display.width() / 2,
                                 M5Cardputer.Display.height() / 2);
}

#define STATE_INIT 0
#define STATE_ADVT 1
#define STATE_HALT 255

uint8_t state;

void setup() {
  state = STATE_INIT;

  init_m5();

  wifi_init_config_t WIFI_INIT_CONFIG = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&WIFI_INIT_CONFIG);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_start();
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&pwnSnifferCallback);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_ps(WIFI_PS_NONE);

  delay(2000);
}

uint8_t current_channel = 1;

void loop() {
  M5Cardputer.update();
  // M5Cardputer.Display.setTextPadding(15);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextFont(&fonts::Font0);

  if (state == STATE_HALT) {
    return;
  }

  if (state == STATE_INIT) {
    M5Cardputer.Display.fillScreen(BLACK);
    // M5Cardputer.Display.setCursor(15, 15);
    M5Cardputer.Display.println("");
    M5Cardputer.Display.println(" Initializing...");
    state = STATE_ADVT;
  }

  if (state == STATE_ADVT) {
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.println(" Advertisement started! (x_x)");
    state++;
  }

  esp_err_t result = advertisePalnagotchi(current_channel++);
  if (current_channel == 15) {
    current_channel = 1;
  }

  if (result == ESP_ERR_WIFI_IF) {
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println(" Error: invalid interface");
    state = STATE_HALT;
  } else if (result == ESP_ERR_INVALID_ARG) {
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println(" Error: invalid argument");
    state = STATE_HALT;
  } else if (result != ESP_OK) {
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println(" Error: unknown");
    state = STATE_HALT;
  }
}
