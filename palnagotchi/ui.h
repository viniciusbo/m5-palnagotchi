#include "M5Cardputer.h"

void initUi();
void wakeUp();
void showMood(String face, String phrase, bool broken = false);
void drawTopCanvas(uint8_t channel = 0);
void drawBottomCanvas(uint8_t friends_run = 0, uint8_t friends_tot = 0,
                      String last_friend_name = "");
