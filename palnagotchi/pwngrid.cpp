#include "pwngrid.h"
#include <ArduinoJson.h>

using namespace ArduinoJson;

uint8_t pwngrid_friends_tot = 0;
pwngrid_peer pwngrid_peers[255];
String pwngrid_last_friend_name = "";

uint8_t getPwngridTotalPeers() { return EEPROM.read(0) + pwngrid_friends_tot; }
uint8_t getPwngridRunTotalPeers() { return pwngrid_friends_tot; }
String getPwngridLastFriendName() { return pwngrid_last_friend_name; }
pwngrid_peer *getPwngridPeers() { return pwngrid_peers; }

// Had to remove Radiotap headers, since its automatically added
// Also had to remove the last 4 bytes (frame check sequence)
const uint8_t pwngrid_beacon_raw[] = {
    0x80, 0x00,                         // FC
    0x00, 0x00,                         // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // DA (broadcast)
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, // SA
    0xa1, 0x00, 0x64, 0xe6, 0x0b, 0x8b, // BSSID
    0x40, 0x43, // Sequence number/fragment number/seq-ctl
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
    0x64, 0x00,                                     // Beacon interval
    0x11, 0x04,                                     // Capability info
    // 0xde (AC = 222) + 1 byte payload len + payload (AC Header)
    // For each 255 bytes of the payload, a new AC header should be set
};

const int raw_beacon_len = sizeof(pwngrid_beacon_raw);

// Required declaration from esp_wifi
esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len,
                            bool en_sys_seq);

// Advertise Palnagotchi beacon over raw 802.11 packets
esp_err_t pwngridAdvertise(uint8_t channel, String face) {
  DynamicJsonDocument pal_json(2048);
  String pal_json_str = "";

  pal_json["pal"] = true; // Also detect other Palnagotchis
  pal_json["name"] = "Palnagotchi";
  pal_json["face"] = face;
  pal_json["epoch"] = 1;
  pal_json["grid_version"] = "1.10.3";
  pal_json["identity"] =
      "32e9f315e92d974342c93d0fd952a914bfb4e6838953536ea6f63d54db6b9610";
  pal_json["pwnd_run"] = 0;
  pal_json["pwnd_tot"] = 0;
  pal_json["session_id"] = "a2:00:64:e6:0b:8b";
  pal_json["timestamp"] = 0;
  pal_json["uptime"] = 0;
  pal_json["version"] = "1.8.4";
  pal_json["policy"]["advertise"] = true;
  pal_json["policy"]["bond_encounters_factor"] = 20000;
  pal_json["policy"]["bored_num_epochs"] = 0;
  pal_json["policy"]["sad_num_epochs"] = 0;
  pal_json["policy"]["excited_num_epochs"] = 9999;

  serializeJson(pal_json, pal_json_str);
  uint16_t pal_json_len = measureJson(pal_json);
  uint8_t header_len = 2 + ((uint8_t)(pal_json_len / 255) * 2);
  uint8_t pwngrid_beacon_frame[raw_beacon_len + pal_json_len + header_len];
  memcpy(pwngrid_beacon_frame, pwngrid_beacon_raw, raw_beacon_len);

  // Iterate through JSON string and copy it to beacon frame
  int frame_byte = raw_beacon_len;
  for (int i = 0; i < pal_json_len; i++) {
    if (i == 0 || i % 255 == 0) {
      pwngrid_beacon_frame[frame_byte++] = 0xde; // AC = 222
      uint8_t payload_len = min(255, pal_json_len - i);
      pwngrid_beacon_frame[frame_byte++] = payload_len;
    }

    // Append JSON byte to frame
    uint8_t next_byte =
        isAscii(pal_json_str[i]) ? (uint8_t)pal_json_str[i] : (uint8_t)'?';
    pwngrid_beacon_frame[frame_byte++] = next_byte;
  }

  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(102);

  return esp_wifi_80211_tx(WIFI_IF_AP, pwngrid_beacon_frame,
                           sizeof(pwngrid_beacon_frame), false);
}

// Add a peer to the list if new
void pwngridAddPeer(DynamicJsonDocument &json, signed int rssi) {
  String identity = json["identity"].as<String>();

  for (uint8_t i = 0; i < pwngrid_friends_tot; i++) {
    if (pwngrid_peers[i].identity == identity) {
      pwngrid_peers[i].last_ping = millis();
      pwngrid_peers[i].gone = false;
      pwngrid_peers[i].rssi = rssi;
      return;
    }
  }

  pwngrid_peer &peer = pwngrid_peers[pwngrid_friends_tot];
  peer.rssi = rssi;
  peer.last_ping = millis();
  peer.gone = false;
  peer.name = json["name"].as<String>();
  peer.face = json["face"].as<String>();
  peer.epoch = json["epoch"].as<int>();
  peer.grid_version = json["grid_version"].as<String>();
  peer.identity = identity;
  peer.pwnd_run = json["pwnd_run"].as<int>();
  peer.pwnd_tot = json["pwnd_tot"].as<int>();
  peer.session_id = json["session_id"].as<String>();
  peer.timestamp = json["timestamp"].as<int>();
  peer.uptime = json["uptime"].as<int>();
  peer.version = json["version"].as<String>();

  pwngrid_last_friend_name = peer.name;
  pwngrid_friends_tot++;
  EEPROM.write(0, pwngrid_friends_tot);
}

const int away_threshold = 120000;

// Mark peers as gone if not seen for a while
void checkPwngridGoneFriends() {
  for (uint8_t i = 0; i < pwngrid_friends_tot; i++) {
    if ((millis() - pwngrid_peers[i].last_ping) > away_threshold) {
      pwngrid_peers[i].gone = true;
    }
  }
}

// Get RSSI of the closest active peer
signed int getPwngridClosestRssi() {
  signed int closest = -1000;

  for (uint8_t i = 0; i < pwngrid_friends_tot; i++) {
    if (!pwngrid_peers[i].gone && pwngrid_peers[i].rssi > closest) {
      closest = pwngrid_peers[i].rssi;
    }
  }

  return closest;
}

// Sniffer packet structure
typedef struct {
  int16_t fctl;
  int16_t duration;
  uint8_t da;
  uint8_t sa;
  uint8_t bssid;
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;

typedef struct {
  uint8_t payload[0];
  WifiMgmtHdr hdr;
} wifi_ieee80211_packet_t;

// Extract MAC from raw data
void getMAC(char *addr, uint8_t *data, uint16_t offset) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset + 0],
          data[offset + 1], data[offset + 2], data[offset + 3],
          data[offset + 4], data[offset + 5]);
}

// Called for every received 802.11 management packet
void pwnSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;

  String src = "";
  String essid = "";

  if (type == WIFI_PKT_MGMT && snifferPacket->payload[0] == 0x80) {
    int len = snifferPacket->rx_ctrl.sig_len - 4;
    char addr[] = "00:00:00:00:00:00";
    getMAC(addr, snifferPacket->payload, 10);
    src.concat(addr);

    if (src == "de:ad:be:ef:de:ad") {
      for (int i = 38; i < len; i++) {
        if (isAscii(snifferPacket->payload[i])) {
          essid.concat((char)snifferPacket->payload[i]);
        }
      }

      DynamicJsonDocument sniffed_json(2048);
      DeserializationError result = deserializeJson(sniffed_json, essid);

      if (!result) {
        pwngridAddPeer(sniffed_json, snifferPacket->rx_ctrl.rssi);
      } else {
        Serial.println("Deserialization error: " + String(result.c_str()));
      }
    }
  }
}

const wifi_promiscuous_filter_t filter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};

// Initialize promiscuous mode and sniffer
void initPwngrid() {
  wifi_init_config_t WIFI_INIT_CONFIG = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&WIFI_INIT_CONFIG);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_start();
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&pwnSnifferCallback);
  esp_wifi_set_channel(random(1, 14), WIFI_SECOND_CHAN_NONE);
  delay(1);
}
