#include "M5Cardputer.h"
#include "M5Unified.h"
#include "ui.h"

#define STATE_INIT 0
#define STATE_ADVT 1
#define STATE_HALT 255

uint8_t state;

void initM5() {
  auto cfg = M5.config();
  M5.begin();
  M5.Display.begin();
  M5Cardputer.begin(cfg);
  M5Cardputer.Keyboard.begin();
}

void setup() {
  initM5();
  initPwngrid();
  initUi();
  state = STATE_INIT;
}

uint8_t current_channel = 1;
uint32_t system_boot_time = millis();
uint32_t last_mood_switch = 10001;

void wakeUp() {
  for (uint8_t i = 0; i < 3; i++) {
    setMood(i);
    updateUi();
    delay(1250);
  }
}

void advertise(uint8_t channel) {
  uint32_t elapsed = millis() - last_mood_switch;
  if (elapsed > 50000) {
    setMood(random(2, 21));
    last_mood_switch = millis();
  }

  esp_err_t result = advertisePalnagotchi(channel, getCurrentMoodFace());

  if (result == ESP_ERR_WIFI_IF) {
    setMood(MOOD_BROKEN, "", "Error: invalid interface", true);
    state = STATE_HALT;
  } else if (result == ESP_ERR_INVALID_ARG) {
    setMood(MOOD_BROKEN, "", "Error: invalid argument", true);
    state = STATE_HALT;
  } else if (result != ESP_OK) {
    setMood(MOOD_BROKEN, "", "Error: unknown", true);
    state = STATE_HALT;
  }
}

bool peer_menu_open = false;

void loop() {
  M5.update();
  M5Cardputer.update();

  if (state == STATE_HALT) {
    return;
  }

  if (state == STATE_INIT) {
    // wakeUp();
    state = STATE_ADVT;
  }

  if (state == STATE_ADVT) {
    checkGoneFriends();
    advertise(current_channel++);
    if (current_channel == 15) {
      current_channel = 1;
    }

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      peer_menu_open = !peer_menu_open;
    }
    Serial.println(peer_menu_open);
  }

  updateUi(true, peer_menu_open);
}
