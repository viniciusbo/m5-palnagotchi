#include "mood.h"

int friends_tot;
int friends_run;
String last_friend_name;
uint8_t current_mood = 0;

void moodManager() {}

String getCurrentFace() { return palnagotchi_moods[current_mood]; }
String getCurrentMood() { return palnagotchi_moods_desc[current_mood]; }
void setMood(uint8_t mood) { current_mood = mood; }
