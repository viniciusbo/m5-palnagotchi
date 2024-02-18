#include "ui.h"

M5Canvas canvas_top(&M5.Display);
M5Canvas canvas_main(&M5.Display);
M5Canvas canvas_bot(&M5.Display);
M5Canvas canvas_peers_menu(&canvas_main);

int32_t display_w;
int32_t display_h;
int32_t canvas_h;
int32_t canvas_center_x;
int32_t canvas_top_h;
int32_t canvas_bot_h;
int32_t canvas_peers_menu_h;
int32_t canvas_peers_menu_w;

void initUi() {
  M5.Display.setRotation(1);
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

  canvas_top.createSprite(display_w, canvas_top_h);
  canvas_bot.createSprite(display_w, canvas_bot_h);
  canvas_main.createSprite(display_w, canvas_h);
}

bool menu_open = false;

void updateUi(bool show_toolbars, bool show_menu) {
  drawTopCanvas();

  uint8_t mood_id = getCurrentMoodId();
  String mood_face = getCurrentMoodFace();
  String mood_phrase = getCurrentMoodPhrase();
  bool mood_broken = isCurrentMoodBroken();

  drawTopCanvas();
  drawBottomCanvas(getRunTotalPeers(), getTotalPeers(), getLastFriendName(),
                   getClosestRssi());
  drawMood(mood_face, mood_phrase, mood_broken);
  drawPeersMenu();

  M5.Display.startWrite();
  if (show_toolbars) {
    canvas_top.pushSprite(0, 0);
    canvas_bot.pushSprite(0, canvas_top_h + canvas_h);
  }

  if (show_menu) {
    canvas_peers_menu.createSprite(floor(display_w * .7), canvas_h);
    canvas_peers_menu.pushSprite(display_w * .1, 0);
  } else {
    // canvas_peers_menu.fillSprite(BLACK);
    canvas_peers_menu.deleteSprite();
  }
  canvas_main.pushSprite(0, canvas_top_h);
  M5.Display.endWrite();
}

void drawTopCanvas() {
  canvas_top.fillSprite(BLACK);
  canvas_top.setTextSize(1);
  canvas_top.setTextColor(GREEN);
  canvas_top.setColor(GREEN);
  canvas_top.setTextDatum(top_left);
  canvas_top.drawString("CH *", 0, 3);
  canvas_top.setTextDatum(top_right);
  unsigned long ellapsed = millis() / 1000;
  int8_t h = ellapsed / 3600;
  int sr = ellapsed % 3600;
  int8_t m = sr / 60;
  int8_t s = sr % 60;
  char right_str[50] = "UP 00:00:00";
  sprintf(right_str, "UP %02d:%02d:%02d", h, m, s);
  // M5 getbattery level might not return data sometimes
  // TODO: check for return value
  // char right_str[50] = "UPS 0%  UP 00:00:00";
  // sprintf(right_str, "UPS %i%% UP %02d:%02d:%02d",
  // M5.Power.getBatteryLevel(),
  //         h, m, s);
  canvas_top.drawString(right_str, display_w, 3);
  canvas_top.drawLine(0, canvas_top_h - 1, display_w, canvas_top_h - 1);
}

void drawBottomCanvas(uint8_t friends_run, uint8_t friends_tot,
                      String last_friend_name, signed int rssi) {
  canvas_bot.fillSprite(BLACK);
  canvas_bot.setTextSize(1);
  canvas_bot.setTextColor(GREEN);
  canvas_bot.setColor(GREEN);
  canvas_bot.setTextDatum(top_left);

  // https://github.com/evilsocket/pwnagotchi/blob/2122af4e264495d32ee415c074da8efd905901f0/pwnagotchi/ui/view.py#L191
  String rssi_bars = "";
  if (rssi != -1000) {
    if (rssi >= -67) {
      rssi_bars = "||||";
    } else if (rssi >= -70) {
      rssi_bars = "|||";
    } else if (rssi >= -80) {
      rssi_bars = "||";
    } else {
      rssi_bars = "|";
    }
  }

  char stats[50] = "FRND 0 (0)";
  if (friends_run > 0) {
    sprintf(stats, "FRND %d (%d) [%s] %s", friends_run, friends_tot,
            last_friend_name, rssi_bars);
  }

  canvas_bot.drawString(stats, 0, 5);
  canvas_bot.setTextDatum(top_right);
  canvas_bot.drawString("NOT AI", display_w, 5);
  canvas_bot.drawLine(0, 0, display_w, 0);
}

void drawMood(String face, String phrase, bool broken) {
  if (broken == true) {
    canvas_main.setTextColor(RED);
  } else {
    canvas_main.setTextColor(GREEN);
  }

  canvas_main.setTextSize(4);
  canvas_main.setTextDatum(middle_center);
  canvas_main.clear(BLACK);
  canvas_main.drawString(face, canvas_center_x, canvas_h / 2);
  canvas_main.setTextDatum(bottom_center);
  canvas_main.setTextSize(1);
  canvas_main.drawString(phrase, canvas_center_x, canvas_h - 23);
}

void drawPeersMenu() {
  // canvas_peers_menu.fillRectAlpha(0, 0, canvas_peers_menu_w,
  //                                 canvas_peers_menu_h, 125, MAGENTA);
  canvas_peers_menu.fillSprite(BLACK);
  canvas_peers_menu.setTextSize(1);
  canvas_peers_menu.setTextColor(GREEN);
  canvas_peers_menu.setColor(GREEN);
  canvas_peers_menu.setTextDatum(top_left);
  canvas_peers_menu.println("> Back");
  canvas_peers_menu.println("> No peers found. Seriosly?");
  canvas_peers_menu.pushSprite(0, canvas_top_h);
}

// #define ARROW_UP ';'
// #define ARROW_DOWN '.'

// bool check_prev_press() {
//   if (M5.Keyboard.isKeyPressed(ARROW_UP)) {
//     return true;
//   }
//
//   return false;
// }
//
// bool check_next_press() {
//   if (M5.Keyboard.isKeyPressed(ARROW_DOWN)) {
//     return true;
//   }
//
//   return false;
// }
