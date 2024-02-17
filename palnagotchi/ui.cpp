#include "ui.h"

#include "pwnagotchi.h"

M5Canvas canvas_top(&M5Cardputer.Display);
M5Canvas canvas(&M5Cardputer.Display);
M5Canvas canvas_bot(&M5Cardputer.Display);

int32_t display_w;
int32_t display_h;
int32_t canvas_h;
int32_t canvas_center_x;
int32_t canvas_top_h;
int32_t canvas_bot_h;

void initUi() {
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextFont(&fonts::Font0);
  // M5Cardputer.Display.setTextFont(&fonts::FreeMono12pt7b);
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  M5Cardputer.Display.setTextColor(GREEN);

  display_w = M5Cardputer.Display.width();
  display_h = M5Cardputer.Display.height();
  canvas_h = display_h * .8;
  canvas_center_x = display_w / 2;
  canvas_top_h = display_h * 0.1;
  canvas_bot_h = display_h * 0.1;

  canvas.createSprite(display_w, canvas_h);
  canvas_top.createSprite(display_w, canvas_top_h);
  canvas_bot.createSprite(display_w, canvas_bot_h);

  canvas.setTextColor(GREEN);

  canvas_top.setTextSize(1);
  canvas_top.setTextColor(GREEN);
  canvas_top.setColor(GREEN);
  canvas_top.setTextDatum(top_left);

  canvas_bot.setTextSize(1);
  canvas_bot.setTextColor(GREEN);
  canvas_bot.setColor(GREEN);
  canvas_bot.setTextDatum(top_left);
}

void wakeUp() {
  canvas.clear(BLACK);
  showMood(0);
  delay(1250);
  showMood(1);
  delay(1250);
  showMood(2);
  delay(1250);
}

void showMood(uint8_t mood, String phrase) {
  if (mood == MOOD_BROKEN) {
    canvas.setTextColor(RED);
  } else {
    canvas.setTextColor(GREEN);
  }

  canvas.setTextSize(4);
  canvas.setTextDatum(middle_center);
  canvas.clear(BLACK);
  canvas.drawString(palnagotchi_moods[mood], canvas_center_x, canvas_h / 2);
  canvas.setTextDatum(bottom_center);
  canvas.setTextSize(1);
  canvas.drawString((phrase != "") ? phrase : palnagotchi_moods_desc[mood],
                    canvas_center_x, canvas_h - 23);

  canvas_top.clear(BLACK);
  canvas_top.drawString("CH * / UP 00:00:00", 0, 3);
  canvas_top.drawLine(0, canvas_top_h - 1, display_w, canvas_top_h - 1);

  canvas_bot.clear(BLACK);
  canvas_bot.drawString("FRND 0 (0) [nikoV] / NOT AI", 0, 5);
  canvas_bot.drawLine(0, 0, display_w, 0);

  M5Cardputer.Display.startWrite();
  canvas.pushSprite(0, canvas_top_h);
  canvas_top.pushSprite(0, 0);
  canvas_bot.pushSprite(0, canvas_top_h + canvas_h);
  M5Cardputer.Display.endWrite();
}

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
