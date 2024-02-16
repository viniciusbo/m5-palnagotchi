#include "ArduinoJson.h"
#include "M5Cardputer.h"
#include "M5Unified.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
// #include "esp_event.h"
// #include "esp_event_loop.h"

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

void init_m5() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextDatum(middle_center);
  //  M5Cardputer.Display.setTextFont(&fonts::Orbitron_Light_24);
  M5Cardputer.Display.setTextFont(&fonts::FreeSans9pt7b);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.drawString("Palnagotchi", M5Cardputer.Display.width() / 2,
                                 M5Cardputer.Display.height() / 2);
}

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

const uint8_t max_peers = 256;
uint8_t tot_peers = 0;
pwngrid_peer peers[max_peers];

void peerManager(DynamicJsonDocument json) {
  // Skip if peer list is full
  if (tot_peers == max_peers) {
    return;
  }

  for (int i = 0; i < tot_peers; i++) {
    String identity = json["identity"].as<String>();
    // Check if peer identity is already in peers array
    if (peers[i].identity == identity) {
      return;
    }

    peers[tot_peers].name = json["name"].as<String>();
    peers[tot_peers].face = json["face"].as<String>();
    peers[tot_peers].epoch = json["epoch"].as<int>();
    peers[tot_peers].grid_version = json["grid_version"].as<String>();
    peers[tot_peers].identity = json["identity"].as<String>();
    peers[tot_peers].pwnd_run = json["pwnd_run"].as<int>();
    peers[tot_peers].pwnd_tot = json["pwnd_tot"].as<int>();
    peers[tot_peers].session_id = json["session_id"].as<String>();
    peers[tot_peers].timestamp = json["timestamp"].as<int>();
    peers[tot_peers].uptime = json["uptime"].as<int>();
    peers[tot_peers].version = json["version"].as<String>();
    tot_peers++;
  }
}

// Detect pwnagotchi adapted from Marauder
// https://github.com/justcallmekoko/ESP32Marauder/wiki/detect-pwnagotchi
// https://github.com/justcallmekoko/ESP32Marauder/blob/master/esp32_marauder/WiFiScan.cpp#L2255

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

const wifi_promiscuous_filter_t filt = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};

void getMAC(char *addr, uint8_t *data, uint16_t offset) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset + 0],
          data[offset + 1], data[offset + 2], data[offset + 3],
          data[offset + 4], data[offset + 5]);
}

void pwnSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String src = "";
  String essid = "";

  if (type == WIFI_PKT_MGMT) {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt =
        (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;

    if ((snifferPacket->payload[0] == 0x80) && (buf == 0)) {
      char addr[] = "00:00:00:00:00:00";
      getMAC(addr, snifferPacket->payload, 10);
      src.concat(addr);
      if (src == "de:ad:be:ef:de:ad") {
        delay(random(0, 10));
        Serial.print("RSSI: ");
        Serial.print(snifferPacket->rx_ctrl.rssi);
        Serial.print(" Ch: ");
        Serial.print(snifferPacket->rx_ctrl.channel);
        Serial.print(" BSSID: ");
        Serial.print(addr);
        display_string.concat("CH: " + (String)snifferPacket->rx_ctrl.channel);
        Serial.print(" ESSID: ");
        display_string.concat(" -> ");

        // Just grab the first 255 bytes of the pwnagotchi beacon
        // because that is where the name is
        for (int i = 0; i < len - 37; i++) {
          Serial.print((char)snifferPacket->payload[i + 38]);
          if (isAscii(snifferPacket->payload[i + 38])) {
            essid.concat((char)snifferPacket->payload[i + 38]);
          }
        }

        DynamicJsonDocument json(1024);  // ArduinoJson v6
        if (deserializeJson(json, essid)) {
          Serial.println("\nCould not parse Pwnagotchi json");
          display_string.concat(essid);
        } else {
          Serial.println("\nSuccessfully parsed json");
          String json_output;
          serializeJson(json, json_output);  // ArduinoJson v6
          Serial.println(json_output);
          display_string.concat(json["name"].as<String>() +
                                " pwnd: " + json["pwnd_tot"].as<String>());
          peerManager(json);
        }

        //        int temp_len = display_string.length();
        //        for (int i = 0; i < 40 - temp_len; i++)
        //        {
        //          display_string.concat(" ");
        //        }
        //
        //        Serial.print(" ");
        //
        //        #ifdef HAS_SCREEN
        //          if (display_obj.display_buffer->size() == 0)
        //          {
        //            display_obj.loading = true;
        //            display_obj.display_buffer->add(display_string);
        //            display_obj.loading = false;
        //          }
        //        #endif
        //
        Serial.println();
        //
        //        buffer_obj.append(snifferPacket, len);
      }
    }
  }
}

#define STATE_INIT 0
#define STATE_ADVT 1
#define STATE_HALT 255

uint8_t state;

DynamicJsonDocument peer_json(2048);
String json_str = "";
int json_len = 0;

void setup() {
  state = STATE_INIT;

  init_m5();

  // wifi_config_t ap_config;
  // ap_config.ap.ssid_hidden = 1;
  // ap_config.ap.beacon_interval = 10000;
  //  ap_config.ap.ssid = "{name:\"Palnagotchi\",face=\"(x_x)\"}";
  // ap_config.ap.ssid_len = 0;

  wifi_init_config_t WIFI_INIT_CONFIG = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&WIFI_INIT_CONFIG);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  //  esp_wifi_set_mac(WIFI_IF_AP, pwngrid_signature_addr);
  esp_wifi_set_mode(WIFI_MODE_STA);
  //  esp_wifi_set_mode(WIFI_MODE_AP);
  //  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&pwnSnifferCallback);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_ps(WIFI_PS_NONE);

  peer_json["name"] = "Palnagotchi";
  peer_json["face"] = "(x_x)";
  peer_json["epoch"] = 1;
  peer_json["grid_version"] = "1.10.3";
  peer_json["identity"] =
      "32e9f315e92d974342c93d0fd952a914bfb4e6838953536ea6f63d54db6b9610";
  peer_json["pwnd_run"] = 0;
  peer_json["pwnd_tot"] = 0;
  peer_json["session_id"] = "a2:00:64:e6:0b:8b";
  peer_json["timestamp"] = 255;
  peer_json["uptime"] = 255;
  peer_json["version"] = "1.8.4";
  peer_json["policy"]["advertise"] = true;
  peer_json["policy"]["ap_ttl"] = 32;
  peer_json["policy"]["associate"] = true;
  peer_json["policy"]["bond_encounters_factor"] = 20000;
  peer_json["policy"]["bored_num_epochs"] = 0;
  peer_json["policy"]["channels"][0] = 1;
  peer_json["policy"]["channels"][1] = 2;
  peer_json["policy"]["channels"][2] = 5;
  peer_json["policy"]["channels"][3] = 10;
  peer_json["policy"]["deauth"] = true;
  peer_json["policy"]["excited_num_epochs"] = 255;
  peer_json["policy"]["hop_recon_time"] = 33;
  peer_json["policy"]["max_inactive_scale"] = 9;
  peer_json["policy"]["max_interactions"] = 6;
  peer_json["policy"]["max_misses_for_recon"] = 7;
  peer_json["policy"]["min_recon_time"] = 30;
  peer_json["policy"]["min_rssi"] = 0;
  peer_json["policy"]["recon_inactive_multiplier"] = 2;
  peer_json["policy"]["recon_time"] = 45;
  peer_json["policy"]["sad_num_epochs"] = 45;
  peer_json["policy"]["sta_ttl"] = 212;

  json_len = measureJson(peer_json);
  serializeJson(peer_json, json_str);
  serializeJson(peer_json, Serial);
  delay(2000);
}

// Had to remove Radiotap headers, since its automatically added
// Also had to remove the last 4 bytes (frame check sequence)
const uint8_t pwngrid_beacon_raw[] = {
    0x80, 0x00,                          // FC
    0x00, 0x00,                          // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // DA (broadcast)
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad,  // SA
    0xa1, 0x00, 0x64, 0xe6, 0x0b, 0x8b,  // BSSID
    0x40, 0x43,  // Sequence number/fragment number/seq-ctl
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Timestamp
    0x64, 0x00,                                      // Beacon interval
    0x11, 0x04,                                      // Capability info
};

const int raw_beacon_len = sizeof(pwngrid_beacon_raw);

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len,
                            bool en_sys_seq);

esp_err_t advertisePalnagotchi(uint8_t channel) {
  uint8_t header_count = 2 + (json_len / 255 * 2);
  uint8_t pwngrid_beacon_frame[raw_beacon_len + json_len + header_count];
  memcpy(pwngrid_beacon_frame, pwngrid_beacon_raw, raw_beacon_len);
  // Serial.println();
  // Serial.print("Raw beacon len: ");
  // Serial.println(raw_beacon_len);
  // Serial.print("JSON len: ");
  // Serial.println(json_len);

  // Iterate through json string and copy it to beacon frame
  int frame_byte = raw_beacon_len;
  for (int i = 0; i < json_len; i++) {
    // Write AC tags before every 255 bytes
    // Serial.print(i);
    // Serial.print(" ");

    if (i == 0 || i % 255 == 0) {
      pwngrid_beacon_frame[frame_byte++] = 0xde;  // AC = 222
      uint8_t payload_len = 255;
      if (json_len - i < 255) {
        payload_len = json_len - i;
      }

      // Serial.print("payload len = ");
      // Serial.println(payload_len);
      // Serial.println("---");

      // Serial.print("Payload len: ");
      // Serial.println(payload_len);
      pwngrid_beacon_frame[frame_byte++] = payload_len;
    }

    // Append json byte to frame
    // If current byte is not ascii, add ? instead
    uint8_t next_byte = (uint8_t)'?';
    if (isAscii(json_str[i])) {
      next_byte = (uint8_t)json_str[i];
    } else {
      // Serial.println('not ascii');
    }

    // Serial.print(" ");
    // Serial.print((char)next_byte);
    // Serial.print(" ");
    pwngrid_beacon_frame[frame_byte++] = next_byte;
  }

  for (int i = 0; i < sizeof(pwngrid_beacon_frame); i++) {
    Serial.print((char)pwngrid_beacon_frame[i]);
  }
  Serial.println("---");

  vTaskDelay(500 / portTICK_PERIOD_MS);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html#_CPPv417esp_wifi_80211_tx16wifi_interface_tPKvib
  vTaskDelay(102 / portTICK_PERIOD_MS);
  esp_err_t result = esp_wifi_80211_tx(WIFI_IF_STA, pwngrid_beacon_frame,
                                       sizeof(pwngrid_beacon_frame), true);
  return result;
}

uint8_t current_channel = 1;

void loop() {
  M5Cardputer.update();
  // M5Cardputer.Display.setTextPadding(15);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextFont(&fonts::Font0);

  if (state == STATE_HALT) {
    return;
  }

  if (state == STATE_INIT) {
    M5Cardputer.Display.fillScreen(BLACK);
    // M5Cardputer.Display.setCursor(15, 15);
    M5Cardputer.Display.println("");
    M5Cardputer.Display.println(" Sending Pwngrid beacon frame...");
    state++;
  }

  if (state == STATE_ADVT) {
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.println(" Advertisement started! (x_x)");
    state++;
  }

  esp_err_t result = advertisePalnagotchi(current_channel++);
  if (current_channel == 15) {
    current_channel = 1;
  }

  if (result == ESP_ERR_WIFI_IF) {
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println(" Error: invalid interface");
    state = STATE_HALT;
  } else if (result == ESP_ERR_INVALID_ARG) {
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println(" Error: invalid argument");
    state = STATE_HALT;
  } else if (result != ESP_OK) {
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println(" Error: unknown");
    state = STATE_HALT;
  }
}
