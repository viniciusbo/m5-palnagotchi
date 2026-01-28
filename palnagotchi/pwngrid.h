#include "Arduino.h"
#include "ArduinoJson.h"
#include "EEPROM.h"
#include "M5Unified.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
// #include "freertos/FreeRTOS.h"

typedef struct {
  int epoch;
  String face;
  String grid_version;
  String identity;
  String name;
  int pwnd_run;
  int pwnd_tot;
  String session_id;
  int timestamp;
  int uptime;
  String version;
  signed int rssi;
  int last_ping;
  bool gone;
} pwngrid_peer;

void initPwngrid();
esp_err_t pwngridAdvertise(uint8_t channel, char session_id[18], String face);
pwngrid_peer* getPwngridPeers();
uint8_t getPwngridRunTotalPeers();
String getPwngridLastFriendName();
signed int getPwngridClosestRssi();
void checkPwngridGoneFriends();
String getPwnName();
void setPwnName(String new_name);
