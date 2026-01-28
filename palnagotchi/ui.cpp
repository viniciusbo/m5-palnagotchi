#include "ui.h"

M5Canvas canvas_top(&M5.Display);
M5Canvas canvas_main(&M5.Display);
M5Canvas canvas_bot(&M5.Display);

int32_t display_w;
int32_t display_h;
int32_t canvas_h;
int32_t canvas_center_x;
int32_t canvas_top_h;
int32_t canvas_bot_h;
int32_t canvas_peers_menu_h;
int32_t canvas_peers_menu_w;

float lastBatteryLevel = -1;
unsigned long lastBatteryUpdate = 0;

struct menu {
  char name[25];
  int command;
};

menu main_menu[] = {{"Nearby Pwnagotchis", 2}, {"Settings", 4}, {"About", 8}};

menu settings_menu[] = {
    {"Change name", 40},
    {"Display brightness", 41},
    {"Sound", 42},
    {"Factory reset", 43},
};

int main_menu_len = sizeof(main_menu) / sizeof(menu);
int settings_menu_len = sizeof(settings_menu) / sizeof(menu);

bool menu_open = false;
uint8_t menu_current_cmd = 0;
uint8_t menu_current_opt = 0;
uint8_t current_brightness = 128;

void initUi() {
  if (M5.Display.width() < M5.Display.height()) {
    M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  }

  M5.Display.setTextFont(&fonts::Font0);
  M5.Display.setTextSize(1);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(GREEN);
  M5.Display.setColor(GREEN);

  display_w = M5.Display.width();
  display_h = M5.Display.height();
  canvas_h = display_h * .8;
  canvas_center_x = display_w / 2;
  canvas_top_h = display_h * .1;
  canvas_bot_h = display_h * .1;
  canvas_peers_menu_h = display_h * .8;
  canvas_peers_menu_w = display_w * .8;

  // M5Core can't render main canvas at full height.
  // This program is not very resource intensive, at least it shouldn't be.
  // This is the maximum height I was able to render right now.
  if (M5.getBoard() == m5::board_t::board_M5Stack) canvas_h = 170;

  canvas_top.createSprite(display_w, canvas_top_h);
  canvas_bot.createSprite(display_w, canvas_bot_h);
  canvas_main.createSprite(display_w, canvas_h);

  // Load brightness from EEPROM
  uint8_t saved_brightness = EEPROM.read(28);
  if (saved_brightness > 0) {
    current_brightness = saved_brightness;
  }
  M5.Display.setBrightness(current_brightness);
}

bool keyboard_changed = false;

bool toggleMenuBtnPressed() {
  #if defined(ARDUINO_M5STACK_CARDPUTER)
    return M5Cardputer.BtnA.isPressed() ||
          (keyboard_changed && (M5Cardputer.Keyboard.isKeyPressed('m') ||
                                M5Cardputer.Keyboard.isKeyPressed('`')));
  #else
    return M5.BtnA.wasHold();
  #endif
}

bool isOkPressed() {
  #if defined(ARDUINO_M5STACK_CARDPUTER)
    return M5Cardputer.BtnA.isPressed() ||
      (keyboard_changed && M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER));
  #elif defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
    return M5.BtnA.wasClicked();
  #else
    return M5.BtnA.wasDecideClickCount() && M5.BtnA.getClickCount() == 1;
  #endif
}

bool isNextPressed() {
  #if defined(ARDUINO_M5STACK_CARDPUTER)
    return keyboard_changed && (M5Cardputer.Keyboard.isKeyPressed('.') ||
                                  M5Cardputer.Keyboard.isKeyPressed('/') ||
                                  M5Cardputer.Keyboard.isKeyPressed(KEY_TAB));
  #elif defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
    return M5.BtnPWR.wasClicked();
  #elif defined(ARDUINO_M5STACK_DIAL)
    return M5Dial.Encoder.readAndReset() > 0;
  #elif defined(ARDUINO_M5STACK_DINMETER)
    return DinMeter.Encoder.readAndReset() > 0;
  #else
    return M5.BtnA.wasDecideClickCount() && M5.BtnA.getClickCount() == 2;
  #endif
}

bool isPrevPressed() {
  #if defined(ARDUINO_M5STACK_CARDPUTER)
    return keyboard_changed && (M5Cardputer.Keyboard.isKeyPressed(',') ||
                                M5Cardputer.Keyboard.isKeyPressed(';'));
  #elif defined(ARDUINO_M5STACK_STICKC) || defined(ARDUINO_M5STACK_STICKC_PLUS) || defined(ARDUINO_M5STACK_STICKC_PLUS2)
    return M5.BtnB.wasClicked();
  #elif defined(ARDUINO_M5STACK_DIAL)
    return M5Dial.Encoder.readAndReset() < 0;
  #elif defined(ARDUINO_M5STACK_DINMETER)
    return DinMeter.Encoder.readAndReset() < 0;
  #else
    return M5.BtnA.wasDecideClickCount() && M5.BtnA.getClickCount() == 3;
  #endif
}

void updateUi(bool show_toolbars) {
  #ifdef ARDUINO_M5STACK_CARDPUTER
    keyboard_changed = M5Cardputer.Keyboard.isChange();
  #else
    keyboard_changed = false;
  #endif

  if (toggleMenuBtnPressed()) {
    if (menu_open == true && menu_current_cmd != 0) {
      menu_current_cmd = 0;
      menu_current_opt = 0;
    } else {
      menu_open = !menu_open;
    }
  }

  uint8_t mood_id = getCurrentMoodId();
  String mood_face = getCurrentMoodFace();
  String mood_phrase = getCurrentMoodPhrase();
  bool mood_broken = isCurrentMoodBroken();
  uint8_t total_peers = 0;
  EEPROM.get(0, total_peers);

  M5.Display.startWrite();
  drawTopCanvas();
  
  String bottom_text = "";
  if (menu_open && menu_current_cmd == 8) {
    bottom_text = "Version: " + String(VERSION);
  }

  drawBottomCanvas(getPwngridRunTotalPeers(), total_peers,
                   getPwngridLastFriendName(), getPwngridClosestRssi(),
                   bottom_text);

  if (menu_open) {
    drawMenu();
  } else {
    drawMood(mood_face, mood_phrase, mood_broken);
  }

  if (show_toolbars) {
    canvas_top.pushSprite(0, 0);
    canvas_bot.pushSprite(0, canvas_top_h + canvas_h);
  }
  canvas_main.pushSprite(0, canvas_top_h);
  M5.Display.endWrite();
}

void drawTopCanvas() {
  unsigned long now = millis();

  if (now - lastBatteryUpdate > 2000) {
    lastBatteryUpdate = now;

    float rawLevel = M5.Power.getBatteryLevel();
    float alpha = 0.2;
    if (rawLevel >= 0 && rawLevel <= 100) {
      if (lastBatteryLevel < 0) {
        lastBatteryLevel = rawLevel;
      } else {
        lastBatteryLevel = alpha * rawLevel + (1 - alpha) * lastBatteryLevel;
      }
    }
  }

  canvas_top.fillSprite(BLACK);
  canvas_top.setTextSize(1);
  canvas_top.setTextColor(GREEN);
  canvas_top.setColor(GREEN);
  canvas_top.setTextDatum(top_left);

  // Hide CH on M5Dial since it can't be seen anyway
  #if !defined(ARDUINO_M5STACK_DIAL)
    canvas_top.drawString("CH *", 0, 5);
  #endif 

  unsigned long ellapsed = millis() / 1000;
  int8_t h = ellapsed / 3600;
  int sr = ellapsed % 3600;
  int8_t m = sr / 60;
  int8_t s = sr % 60;
  char right_str[50] = "";

  if (display_w > 128 && M5.Power.getBatteryVoltage() > 0) {
    sprintf(right_str, "UPS %i%% UP %02d:%02d:%02d", (int)lastBatteryLevel, h, m,
          s);
    canvas_top.setTextDatum(top_right);
    canvas_top.drawString(right_str, display_w, 5);
  } else {
    sprintf(right_str, "UP %02d:%02d:%02d", h, m, s);

    #ifdef ARDUINO_M5STACK_DIAL
      // Center text on M5Dial
      canvas_top.setTextDatum(top_center);
      canvas_top.drawString(right_str, canvas_center_x, 5);
    #else
      canvas_top.setTextDatum(top_right);
      canvas_top.drawString(right_str, display_w, 5);
    #endif
  }
  
  canvas_top.drawLine(0, canvas_top_h - 1, display_w, canvas_top_h - 1);
}

String getRssiBars(signed int rssi) {
  if (rssi != -1000) {
    if (rssi >= -67) {
      return "||||";
    } else if (rssi >= -70) {
      return "|||";
    } else if (rssi >= -80) {
      return "||";
    } else {
      return "|";
    }
  }

  return "";
}

void drawBottomCanvas(uint8_t friends_run, uint8_t friends_tot,
                      String last_friend_name, signed int rssi,
                      String override_text) {
  canvas_bot.fillSprite(BLACK);
  canvas_bot.setTextSize(1);
  canvas_bot.setTextColor(GREEN);
  canvas_bot.setColor(GREEN);
  
  if (override_text != "") {
    canvas_bot.setTextDatum(top_center);
    canvas_bot.drawString(override_text, canvas_center_x, 5);
    canvas_bot.drawLine(0, 0, display_w, 0);
    return;
  }

  canvas_bot.setTextDatum(top_left);

  #ifdef ARDUINO_M5STACK_DIAL
    canvas_bot.setTextDatum(top_center);
  #endif

  String rssi_bars = getRssiBars(rssi);
  char stats[64];
  if (friends_run > 0) {
    sprintf(stats, "FRND %d (%d) [%s] %s", friends_run, friends_tot,
            last_friend_name.c_str(), rssi_bars.c_str());
  } else {
    sprintf(stats, "FRND 0 (0)");
  }

  uint8_t x_pos = 0;
  #ifdef ARDUINO_M5STACK_DIAL
    x_pos = canvas_center_x;
  #endif

  canvas_bot.drawString(stats, x_pos, 5);
  canvas_bot.setTextDatum(top_right);
  if (display_w > 128) {
    canvas_bot.drawString("NOT AI", display_w, 5);
  }
  canvas_bot.drawLine(0, 0, display_w, 0);
}

void drawMood(String face, String phrase, bool broken) {
  canvas_main.setTextColor(broken ? RED : GREEN);
  canvas_main.setTextSize(4);
  canvas_main.setTextDatum(middle_center);
  canvas_main.fillSprite(BLACK);
  canvas_main.drawString(face, canvas_center_x, canvas_h / 2);
  canvas_main.setTextDatum(bottom_center);
  canvas_main.setTextSize(1);
  canvas_main.drawString(phrase, canvas_center_x, canvas_h - 23);
}

#define ROW_SIZE 40
#define PADDING 10

void drawMainMenu() {
  canvas_main.fillSprite(BLACK);
  if (display_w > 128) {
    canvas_main.setTextSize(2);
  } else {
    canvas_main.setTextSize(1.5);
  }
  canvas_main.setTextColor(GREEN);
  canvas_main.setTextDatum(top_left);

  char display_str[50] = "";
  for (uint8_t i = 0; i < main_menu_len; i++) {
    sprintf(display_str, "%s %s", (menu_current_opt == i) ? ">" : " ",
            main_menu[i].name);
    int y = PADDING + (i * ROW_SIZE / 2);
    canvas_main.drawString(display_str, 0, y);
  }
}

void drawNearbyMenu() {
  canvas_main.clear(BLACK);
  canvas_main.setTextSize(2);
  canvas_main.setTextColor(GREEN);
  canvas_main.setTextDatum(top_left);

  pwngrid_peer *pwngrid_peers = getPwngridPeers();
  uint8_t len = getPwngridRunTotalPeers();

  if (len == 0) {
    canvas_main.setTextColor(TFT_DARKGRAY);
    canvas_main.setCursor(0, PADDING);
    canvas_main.println("No nearby Pwnagotchis. Seriously?");
  }

  char display_str[50] = "";
  for (uint8_t i = 0; i < len; i++) {
    sprintf(display_str, "%s %s [%s]", (menu_current_opt == i) ? ">" : " ",
            pwngrid_peers[i].name, getRssiBars(pwngrid_peers[i].rssi).c_str());
    int y = PADDING + (i * ROW_SIZE / 2);
    canvas_main.drawString(display_str, 0, y);
  }
}

void drawSettingsMenu() {
  canvas_main.fillSprite(BLACK);
  canvas_main.setTextSize(2);
  canvas_main.setTextColor(GREEN);
  canvas_main.setTextDatum(top_left);

  char display_str[50] = "";
  for (uint8_t i = 0; i < settings_menu_len; i++) {
    sprintf(display_str, "%s %s", (menu_current_opt == i) ? ">" : " ",
            settings_menu[i].name);
    int y = PADDING + (i * ROW_SIZE / 2);
    canvas_main.drawString(display_str, 0, y);
  }
}

void drawAboutMenu() {
  canvas_main.clear(BLACK);
  canvas_main.qrcode("https://github.com/viniciusbo/m5-palnagotchi",
                     (display_w / 2) - (display_h * 0.3), PADDING,
                     display_h * 0.65);
}

void drawChangeNameMenu(String name) {
  canvas_main.fillSprite(BLACK);
  canvas_main.setTextSize(2);
  canvas_main.setTextColor(GREEN);
  canvas_main.setTextDatum(top_center);
  canvas_main.drawString("New Name:", canvas_center_x, PADDING);
  canvas_main.setTextSize(3);
  canvas_main.drawString(name, canvas_center_x, canvas_h / 2 - 10);

  canvas_main.setTextSize(1);
  canvas_main.setTextDatum(bottom_center);
  canvas_main.drawString("NEXT: Randomize  A: Save", canvas_center_x,
                         canvas_h - PADDING);
}

void drawBrightnessMenu(uint8_t val) {
  canvas_main.fillSprite(BLACK);
  canvas_main.setTextSize(2);
  canvas_main.setTextColor(GREEN);
  canvas_main.setTextDatum(top_center);
  canvas_main.drawString("Brightness", canvas_center_x, PADDING);

  int bar_w = display_w * 0.8;
  int bar_h = 20;
  int bar_x = (display_w - bar_w) / 2;
  int bar_y = canvas_h / 2 - 10;

  canvas_main.drawRect(bar_x, bar_y, bar_w, bar_h, GREEN);
  
  // Map 0-255 to 0-bar_w
  int fill_w = map(val, 0, 255, 0, bar_w - 4);
  canvas_main.fillRect(bar_x + 2, bar_y + 2, fill_w, bar_h - 4, GREEN);

  canvas_main.setTextSize(1);
  canvas_main.setTextDatum(bottom_center);
  canvas_main.drawString("NEXT: +  PREV: -  A: Save", canvas_center_x,
                         canvas_h - PADDING);
}

void drawMenu() {
  if (menu_current_cmd == 0 || menu_current_cmd == 2 || menu_current_cmd == 4) {
    if (isNextPressed()) {
      menu_current_opt++;
      if (menu_current_cmd == 0 && menu_current_opt >= main_menu_len)
        menu_current_opt = 0;
      else if (menu_current_cmd == 4 && menu_current_opt >= settings_menu_len)
        menu_current_opt = 0;
      else if (menu_current_cmd == 2) {
        uint8_t total_peers = getPwngridRunTotalPeers();
        if (total_peers == 0 || menu_current_opt >= total_peers)
          menu_current_opt = 0;
      }
    }

    if (isPrevPressed()) {
      if (menu_current_opt > 0)
        menu_current_opt--;
      else if (menu_current_cmd == 0)
        menu_current_opt = main_menu_len - 1;
      else if (menu_current_cmd == 4)
        menu_current_opt = settings_menu_len - 1;
      else if (menu_current_cmd == 2) {
        uint8_t total_peers = getPwngridRunTotalPeers();
        if (total_peers > 0)
          menu_current_opt = total_peers - 1;
        else
          menu_current_opt = 0;
      }
    }
  }

  switch (menu_current_cmd) {
    case 0:
      if (isOkPressed()) {
        menu_current_cmd = main_menu[menu_current_opt].command;
        menu_current_opt = 0;
      }
      drawMainMenu();
      break;
    case 2:
      drawNearbyMenu();
      break;
    case 4:
      if (isOkPressed()) {
        menu_current_cmd = settings_menu[menu_current_opt].command;
        menu_current_opt = 0;
      }
      drawSettingsMenu();
      break;
    case 8:
      drawAboutMenu();
      break;
    case 40:
      {
        static String temp_pwn_name = "";
        if (temp_pwn_name == "") temp_pwn_name = getPwnName();

        if (isNextPressed()) {
          const char *names[] = {"PwnStar", "Hackey", "Glitch", "Cipher", "Zero", "PwnZor", "Ghost", "Neo"};
          temp_pwn_name = names[random(0, 8)];
        } else if (isOkPressed()) {
          setPwnName(temp_pwn_name);
          temp_pwn_name = ""; // Reset for next time
          menu_current_cmd = 4; // Return to Settings
          menu_current_opt = 0;
        }
        drawChangeNameMenu(temp_pwn_name);
      }
      break;
    case 41: // Display brightness
      if (isNextPressed()) {
        if (current_brightness > 239) current_brightness = 255;
        else current_brightness += 16;
        M5.Display.setBrightness(current_brightness);
      } else if (isPrevPressed()) {
        if (current_brightness < 21) current_brightness = 5;
        else current_brightness -= 16;
        M5.Display.setBrightness(current_brightness);
      } else if (isOkPressed()) {
        EEPROM.write(28, current_brightness);
        EEPROM.commit();
        menu_current_cmd = 4;
        menu_current_opt = 1; // Highlight "Brightness" option
      }
      drawBrightnessMenu(current_brightness);
      break;
    case 42: // Sound
      if (isOkPressed()) {
        menu_current_cmd = 4;
        menu_current_opt = 0;
      }
      canvas_main.fillSprite(BLACK);
      canvas_main.setTextSize(2);
      canvas_main.setTextDatum(middle_center);
      canvas_main.drawString("Not Implemented", canvas_center_x, canvas_h / 2);
      canvas_main.setTextSize(1);
      canvas_main.setTextDatum(bottom_center);
      canvas_main.drawString("Click A to Return", canvas_center_x, canvas_h - PADDING);
      break;
    case 43: // Factory reset
      if (isOkPressed()) {
        for (int i = 0; i < 256; i++) {
          EEPROM.write(i, 255);
        }
        EEPROM.commit();
        ESP.restart();
      } else if (isPrevPressed()) {
        menu_current_cmd = 4;
        menu_current_opt = 0;
      }
      canvas_main.fillSprite(BLACK);
      canvas_main.setTextSize(2);
      canvas_main.setTextColor(RED);
      canvas_main.setTextDatum(top_center);
      canvas_main.drawString("FACTORY RESET?", canvas_center_x, PADDING);
      canvas_main.setTextSize(1);
      canvas_main.setTextColor(GREEN);
      canvas_main.setTextDatum(middle_center);
      canvas_main.drawString("This will wipe all data", canvas_center_x, canvas_h / 2);
      canvas_main.setTextDatum(bottom_center);
      canvas_main.drawString("BACK: Cancel  A: Confirm", canvas_center_x, canvas_h - PADDING);
      break;
    default:
      drawMainMenu();
      break;
  }
}
