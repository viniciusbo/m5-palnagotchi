#ifdef ARDUINO_M5STACK_CARDPUTER
  #include "M5Cardputer.h"
#endif

#ifdef ARDUINO_M5STACK_DIAL
  #include "M5Dial.h"
#endif

#ifdef ARDUINO_M5STACK_DINMETER
  #include "M5DinMeter.h"
#endif

#include "EEPROM.h"
#include "mood.h"
#include "pwngrid.h"

#define VERSION "v1.3"

void initUi();
void wakeUp();
void drawMood(String face, String phrase, bool broken = false);
void drawTopCanvas();
void drawBottomCanvas(uint8_t friends_run = 0, uint8_t friends_tot = 0,
                      String last_friend_name = "", signed int rssi = -1000,
                      String override_text = "");
void drawMenu();
void updateUi(bool show_toolbars = false);
