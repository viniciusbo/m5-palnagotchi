#include "ArduinoJson.h"
#include "M5Cardputer.h"
#include "M5Unified.h"
#include "pwngrid.h"
#include "ui.h"

void init_m5() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.begin();
}

#define STATE_INIT 0
#define STATE_ADVT 1
#define STATE_HALT 255

uint8_t state;

void setup() {
  init_m5();
  initUi();

  wifi_init_config_t WIFI_INIT_CONFIG = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&WIFI_INIT_CONFIG);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_start();
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&pwnSnifferCallback);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_ps(WIFI_PS_NONE);

  state = STATE_INIT;
}

uint8_t current_channel = 1;
unsigned long last_mood_switch = 10001;

void loop() {
  M5Cardputer.update();

  if (state == STATE_HALT) {
    return;
  }

  if (state == STATE_INIT) {
    wakeUp();
    state = STATE_ADVT;
  }

  if (state == STATE_ADVT) {
    unsigned long elapsed = millis() - last_mood_switch;
    if (elapsed > 10000) {
      uint8_t mood = random(2, 21);
      showMood(mood);
      last_mood_switch = millis();
    }
  }

  esp_err_t result = advertisePalnagotchi(current_channel++);
  if (current_channel == 15) {
    current_channel = 1;
  }

  if (result == ESP_ERR_WIFI_IF) {
    showMood(MOOD_BROKEN, "Error: invalid interface");
    state = STATE_HALT;
  } else if (result == ESP_ERR_INVALID_ARG) {
    showMood(MOOD_BROKEN, "Error: invalid argument");
    state = STATE_HALT;
  } else if (result != ESP_OK) {
    showMood(MOOD_BROKEN, "Error: unknown");
    state = STATE_HALT;
  }
}
