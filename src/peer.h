#include "ArduinoJson.h"

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
} pwngrid_peer;

void peerManager(DynamicJsonDocument json);
