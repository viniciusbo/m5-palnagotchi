#include "ArduinoJson.h"
#include "EEPROM.h"
#include "M5Cardputer.h"
#include "M5Unified.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"

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
esp_err_t advertisePalnagotchi(uint8_t channel, String face);
void getPeers(pwngrid_peer *buffer);
uint8_t getRunTotalPeers();
uint8_t getTotalPeers();
String getLastFriendName();
signed int getClosestRssi();
void checkGoneFriends();
