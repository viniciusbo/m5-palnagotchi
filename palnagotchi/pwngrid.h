#include "ArduinoJson.h"
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
} pwngrid_peer;

void initPwngrid();
esp_err_t advertisePalnagotchi(uint8_t channel, String face);
void getPeers(pwngrid_peer *buffer);
uint16_t getRunTotalPeers();
uint16_t getTotalPeers();
String getLastFriendName();
void peerManager(DynamicJsonDocument json);
