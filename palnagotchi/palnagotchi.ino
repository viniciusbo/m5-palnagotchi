#include "ArduinoJson.h"
#include "M5Cardputer.h"
#include "M5Unified.h"
#include "mood.h"
#include "pwngrid.h"
#include "ui.h"

#define STATE_INIT 0
#define STATE_ADVT 1
#define STATE_HALT 255

uint8_t state;

void initM5() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.begin();
}

void setup() {
  initM5();
  initUi();
  initPwngrid();
  state = STATE_INIT;
}

uint8_t current_channel = 1;
uint32_t system_boot_time = millis();
uint32_t last_mood_switch = 10001;

const String broken_face = palnagotchi_moods[MOOD_BROKEN];

void wakeUp() {
  for (uint8_t i = 0; i < 3; i++) {
    setMood(i);
    showMood(getCurrentFace(), getCurrentMood());
    delay(1250);
  }

  drawTopCanvas();
  drawBottomCanvas();
}

void advertise() {
  uint32_t elapsed = millis() - last_mood_switch;
  if (elapsed > 10000) {
    setMood(random(2, 21));
    showMood(getCurrentFace(), getCurrentMood());
    last_mood_switch = millis();
  }

  uint32_t uptime = millis() / 1000;
  esp_err_t result =
      advertisePalnagotchi(current_channel++, uptime, getCurrentFace());
  if (current_channel == 15) {
    current_channel = 1;
  }

  if (result == ESP_ERR_WIFI_IF) {
    showMood(broken_face, "Error: invalid interface", true);
    state = STATE_HALT;
  } else if (result == ESP_ERR_INVALID_ARG) {
    showMood(broken_face, "Error: invalid argument", true);
    state = STATE_HALT;
  } else if (result != ESP_OK) {
    showMood(broken_face, "Error: unknown", true);
    state = STATE_HALT;
  }
}

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
    advertise();
  }
}
