#include "ArduinoJson.h"
#include "M5Cardputer.h"
#include "M5Unified.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
// #include "esp_event.h"
// #include "esp_event_loop.h"

#define ARROW_UP ';'
#define ARROW_DOWN '.'

uint8_t pwngrid_signature_addr[] = {0xde, 0xad, 0xbe, 0xef, 0xde, 0xad};
uint8_t pwngrid_broadcast_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

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

void setup() {
  state = STATE_INIT;

  init_m5();

  wifi_config_t ap_config;
  ap_config.ap.ssid_hidden = 1;
  ap_config.ap.beacon_interval = 10000;
  //  ap_config.ap.ssid = "{name:\"Palnagotchi\",face=\"(x_x)\"}";
  ap_config.ap.ssid_len = 0;

  wifi_init_config_t WIFI_INIT_CONFIG = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&WIFI_INIT_CONFIG);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  //  esp_wifi_set_mac(WIFI_IF_AP, pwngrid_signature_addr);
  //  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_mode(WIFI_MODE_AP);
  //  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&pwnSnifferCallback);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_ps(WIFI_PS_NONE);

  delay(2000);
}

uint8_t current_channel = 1;
uint8_t max_channel = 14;

// True pwngrid beacon frame dump
// Had to remove Radiotap headers, since its automatically added (guess from
// physical layer?) Also there were some trailing bytes I had to remove for the
// packet not to appear malformed (last bytes .u9.) in Wireshark (added from
// some TCP/IP layer?)

/*
0000   00 00 24 00 2f 40 00 a0 20 08 00 00 00 00 00 00   ..$./@.. .......
0010   6e c9 08 81 ac 05 00 00 10 02 99 09 a0 00 db 00   n...............
0020   00 00 db 00 80 00 00 00 ff ff ff ff ff ff de ad   ................
0030   be ef de ad a2 00 64 e6 0b 8b 40 43 00 00 00 00   ......d...@C....
0040   00 00 00 00 64 00 11 04 de ff 7b 22 65 70 6f 63   ....d.....{"epoc
0050   68 22 3a 32 2c 22 66 61 63 65 22 3a 22 28 e2 87   h":2,"face":"(..
0060   80 e2 80 bf e2 80 bf e2 86 bc 29 22 2c 22 67 72   ..........)","gr
0070   69 64 5f 76 65 72 73 69 6f 6e 22 3a 22 31 2e 31   id_version":"1.1
0080   30 2e 33 22 2c 22 69 64 65 6e 74 69 74 79 22 3a   0.3","identity":
0090   22 33 32 65 39 66 33 31 35 65 39 32 64 39 37 34   "32e9f315e92d974
00a0   33 34 32 63 39 33 64 30 66 64 39 35 32 61 39 31   342c93d0fd952a91
00b0   34 62 66 62 34 65 36 38 33 38 39 35 33 35 33 36   4bfb4e6838953536
00c0   65 61 36 66 36 33 64 35 34 64 62 36 62 39 36 31   ea6f63d54db6b961
00d0   30 22 2c 22 6e 61 6d 65 22 3a 22 6e 69 6b 6f 56   0","name":"nikoV
00e0   22 2c 22 70 6f 6c 69 63 79 22 3a 7b 22 61 64 76   ","policy":{"adv
00f0   65 72 74 69 73 65 22 3a 74 72 75 65 2c 22 61 70   ertise":true,"ap
0100   5f 74 74 6c 22 3a 33 31 38 2c 22 61 73 73 6f 63   _ttl":318,"assoc
0110   69 61 74 65 22 3a 74 72 75 65 2c 22 62 6f 6e 64   iate":true,"bond
0120   5f 65 6e 63 6f 75 6e 74 65 72 73 5f 66 61 63 74   _encounters_fact
0130   6f 72 22 3a 32 30 30 30 30 2c 22 62 6f 72 65 64   or":20000,"bored
0140   5f 6e 75 6d 5f 65 70 6f 63 de ff 68 73 22 3a 31   _num_epoc..hs":1
0150   36 2c 22 63 68 61 6e 6e 65 6c 73 22 3a 5b 32 2c   6,"channels":[2,
0160   33 2c 34 2c 35 2c 36 2c 37 2c 38 2c 39 2c 31 30   3,4,5,6,7,8,9,10
0170   2c 31 31 2c 31 32 5d 2c 22 64 65 61 75 74 68 22   ,11,12],"deauth"
0180   3a 74 72 75 65 2c 22 65 78 63 69 74 65 64 5f 6e   :true,"excited_n
0190   75 6d 5f 65 70 6f 63 68 73 22 3a 39 2c 22 68 6f   um_epochs":9,"ho
01a0   70 5f 72 65 63 6f 6e 5f 74 69 6d 65 22 3a 33 36   p_recon_time":36
01b0   2c 22 6d 61 78 5f 69 6e 61 63 74 69 76 65 5f 73   ,"max_inactive_s
01c0   63 61 6c 65 22 3a 37 2c 22 6d 61 78 5f 69 6e 74   cale":7,"max_int
01d0   65 72 61 63 74 69 6f 6e 73 22 3a 32 30 2c 22 6d   eractions":20,"m
01e0   61 78 5f 6d 69 73 73 65 73 5f 66 6f 72 5f 72 65   ax_misses_for_re
01f0   63 6f 6e 22 3a 38 2c 22 6d 69 6e 5f 72 65 63 6f   con":8,"min_reco
0200   6e 5f 74 69 6d 65 22 3a 31 33 2c 22 6d 69 6e 5f   n_time":13,"min_
0210   72 73 73 69 22 3a 2d 31 38 34 2c 22 72 65 63 6f   rssi":-184,"reco
0220   6e 5f 69 6e 61 63 74 69 76 65 5f 6d 75 6c 74 69   n_inactive_multi
0230   70 6c 69 65 72 22 3a 32 2c 22 72 65 63 6f 6e 5f   plier":2,"recon_
0240   74 69 6d 65 22 3a 34 32 2c 22 de 95 73 61 64 5f   time":42,"..sad_
0250   6e 75 6d 5f 65 70 6f 63 68 73 22 3a 32 38 2c 22   num_epochs":28,"
0260   73 74 61 5f 74 74 6c 22 3a 31 33 37 7d 2c 22 70   sta_ttl":137},"p
0270   77 6e 64 5f 72 75 6e 22 3a 32 2c 22 70 77 6e 64   wnd_run":2,"pwnd
0280   5f 74 6f 74 22 3a 31 36 33 2c 22 73 65 73 73 69   _tot":163,"sessi
0290   6f 6e 5f 69 64 22 3a 22 61 32 3a 30 30 3a 36 34   on_id":"a2:00:64
02a0   3a 65 36 3a 30 62 3a 38 62 22 2c 22 74 69 6d 65   :e6:0b:8b","time
02b0   73 74 61 6d 70 22 3a 31 36 38 33 33 38 37 34 36   stamp":168338746
02c0   35 2c 22 75 70 74 69 6d 65 22 3a 32 36 34 2c 22   5,"uptime":264,"
02d0   76 65 72 73 69 6f 6e 22 3a 22 31 2e 38 2e 34 22   version":"1.8.4"
02e0   7d 8e 75 39 0d                                    }.u9.
 */
uint8_t pwngrid_beacon[] = {
    0x80, 0x00,                          // FC
    0x00, 0x00,                          // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // DA (broadcast)
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad,  // SA
    0xa2, 0x00, 0x64, 0xe6, 0x0b, 0x8b,  // BSSID
    0x40, 0x43,  // Sequence number / fragment number / seq-ctl
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Timestamp
    0x64, 0x00,                                      // Beacon interval
    0x11, 0x04,                                      // Capability info
    // First ESSID payload (max length = 255)
    0xde, 0xff,  // Length = 255
    0x7b, 0x22, 0x65, 0x70, 0x6f, 0x63, 0x68, 0x22, 0x3a, 0x32, 0x2c, 0x22,
    0x66, 0x61, 0x63, 0x65, 0x22, 0x3a, 0x22, 0x28, 0xe2, 0x87, 0x80, 0xe2,
    0x80, 0xbf, 0xe2, 0x80, 0xbf, 0xe2, 0x86, 0xbc, 0x29, 0x22, 0x2c, 0x22,
    0x67, 0x72, 0x69, 0x64, 0x5f, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e,
    0x22, 0x3a, 0x22, 0x31, 0x2e, 0x31, 0x30, 0x2e, 0x33, 0x22, 0x2c, 0x22,
    0x69, 0x64, 0x65, 0x6e, 0x74, 0x69, 0x74, 0x79, 0x22, 0x3a, 0x22, 0x33,
    0x32, 0x65, 0x39, 0x66, 0x33, 0x31, 0x35, 0x65, 0x39, 0x32, 0x64, 0x39,
    0x37, 0x34, 0x33, 0x34, 0x32, 0x63, 0x39, 0x33, 0x64, 0x30, 0x66, 0x64,
    0x39, 0x35, 0x32, 0x61, 0x39, 0x31, 0x34, 0x62, 0x66, 0x62, 0x34, 0x65,
    0x36, 0x38, 0x33, 0x38, 0x39, 0x35, 0x33, 0x35, 0x33, 0x36, 0x65, 0x61,
    0x36, 0x66, 0x36, 0x33, 0x64, 0x35, 0x34, 0x64, 0x62, 0x36, 0x62, 0x39,
    0x36, 0x31, 0x30, 0x22, 0x2c, 0x22, 0x6e, 0x61, 0x6d, 0x65, 0x22, 0x3a,
    0x22, 0x6e, 0x69, 0x6b, 0x6f, 0x56, 0x22, 0x2c, 0x22, 0x70, 0x6f, 0x6c,
    0x69, 0x63, 0x79, 0x22, 0x3a, 0x7b, 0x22, 0x61, 0x64, 0x76, 0x65, 0x72,
    0x74, 0x69, 0x73, 0x65, 0x22, 0x3a, 0x74, 0x72, 0x75, 0x65, 0x2c, 0x22,
    0x61, 0x70, 0x5f, 0x74, 0x74, 0x6c, 0x22, 0x3a, 0x33, 0x31, 0x38, 0x2c,
    0x22, 0x61, 0x73, 0x73, 0x6f, 0x63, 0x69, 0x61, 0x74, 0x65, 0x22, 0x3a,
    0x74, 0x72, 0x75, 0x65, 0x2c, 0x22, 0x62, 0x6f, 0x6e, 0x64, 0x5f, 0x65,
    0x6e, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x73, 0x5f, 0x66, 0x61,
    0x63, 0x74, 0x6f, 0x72, 0x22, 0x3a, 0x32, 0x30, 0x30, 0x30, 0x30, 0x2c,
    0x22, 0x62, 0x6f, 0x72, 0x65, 0x64, 0x5f, 0x6e, 0x75, 0x6d, 0x5f, 0x65,
    0x70, 0x6f, 0x63,
    // Second ESSID payload (length = 255)
    0xde, 0xff,  // Length = 255
    0x68, 0x73, 0x22, 0x3a, 0x31, 0x36, 0x2c, 0x22, 0x63, 0x68, 0x61, 0x6e,
    0x6e, 0x65, 0x6c, 0x73, 0x22, 0x3a, 0x5b, 0x32, 0x2c, 0x33, 0x2c, 0x34,
    0x2c, 0x35, 0x2c, 0x36, 0x2c, 0x37, 0x2c, 0x38, 0x2c, 0x39, 0x2c, 0x31,
    0x30, 0x2c, 0x31, 0x31, 0x2c, 0x31, 0x32, 0x5d, 0x2c, 0x22, 0x64, 0x65,
    0x61, 0x75, 0x74, 0x68, 0x22, 0x3a, 0x74, 0x72, 0x75, 0x65, 0x2c, 0x22,
    0x65, 0x78, 0x63, 0x69, 0x74, 0x65, 0x64, 0x5f, 0x6e, 0x75, 0x6d, 0x5f,
    0x65, 0x70, 0x6f, 0x63, 0x68, 0x73, 0x22, 0x3a, 0x39, 0x2c, 0x22, 0x68,
    0x6f, 0x70, 0x5f, 0x72, 0x65, 0x63, 0x6f, 0x6e, 0x5f, 0x74, 0x69, 0x6d,
    0x65, 0x22, 0x3a, 0x33, 0x36, 0x2c, 0x22, 0x6d, 0x61, 0x78, 0x5f, 0x69,
    0x6e, 0x61, 0x63, 0x74, 0x69, 0x76, 0x65, 0x5f, 0x73, 0x63, 0x61, 0x6c,
    0x65, 0x22, 0x3a, 0x37, 0x2c, 0x22, 0x6d, 0x61, 0x78, 0x5f, 0x69, 0x6e,
    0x74, 0x65, 0x72, 0x61, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x22, 0x3a,
    0x32, 0x30, 0x2c, 0x22, 0x6d, 0x61, 0x78, 0x5f, 0x6d, 0x69, 0x73, 0x73,
    0x65, 0x73, 0x5f, 0x66, 0x6f, 0x72, 0x5f, 0x72, 0x65, 0x63, 0x6f, 0x6e,
    0x22, 0x3a, 0x38, 0x2c, 0x22, 0x6d, 0x69, 0x6e, 0x5f, 0x72, 0x65, 0x63,
    0x6f, 0x6e, 0x5f, 0x74, 0x69, 0x6d, 0x65, 0x22, 0x3a, 0x31, 0x33, 0x2c,
    0x22, 0x6d, 0x69, 0x6e, 0x5f, 0x72, 0x73, 0x73, 0x69, 0x22, 0x3a, 0x2d,
    0x31, 0x38, 0x34, 0x2c, 0x22, 0x72, 0x65, 0x63, 0x6f, 0x6e, 0x5f, 0x69,
    0x6e, 0x61, 0x63, 0x74, 0x69, 0x76, 0x65, 0x5f, 0x6d, 0x75, 0x6c, 0x74,
    0x69, 0x70, 0x6c, 0x69, 0x65, 0x72, 0x22, 0x3a, 0x32, 0x2c, 0x22, 0x72,
    0x65, 0x63, 0x6f, 0x6e, 0x5f,
    // Third ESSID payload
    0x74, 0x69,  // Length = 149
    0x6d, 0x65, 0x22, 0x3a, 0x34, 0x32, 0x2c, 0x22, 0xde, 0x95, 0x73, 0x61,
    0x64, 0x5f, 0x6e, 0x75, 0x6d, 0x5f, 0x65, 0x70, 0x6f, 0x63, 0x68, 0x73,
    0x22, 0x3a, 0x32, 0x38, 0x2c, 0x22, 0x73, 0x74, 0x61, 0x5f, 0x74, 0x74,
    0x6c, 0x22, 0x3a, 0x31, 0x33, 0x37, 0x7d, 0x2c, 0x22, 0x70, 0x77, 0x6e,
    0x64, 0x5f, 0x72, 0x75, 0x6e, 0x22, 0x3a, 0x32, 0x2c, 0x22, 0x70, 0x77,
    0x6e, 0x64, 0x5f, 0x74, 0x6f, 0x74, 0x22, 0x3a, 0x31, 0x36, 0x33, 0x2c,
    0x22, 0x73, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x5f, 0x69, 0x64, 0x22,
    0x3a, 0x22, 0x61, 0x32, 0x3a, 0x30, 0x30, 0x3a, 0x36, 0x34, 0x3a, 0x65,
    0x36, 0x3a, 0x30, 0x62, 0x3a, 0x38, 0x62, 0x22, 0x2c, 0x22, 0x74, 0x69,
    0x6d, 0x65, 0x73, 0x74, 0x61, 0x6d, 0x70, 0x22, 0x3a, 0x31, 0x36, 0x38,
    0x33, 0x33, 0x38, 0x37, 0x34, 0x36, 0x35, 0x2c, 0x22, 0x75, 0x70, 0x74,
    0x69, 0x6d, 0x65, 0x22, 0x3a, 0x32, 0x36, 0x34, 0x2c, 0x22, 0x76, 0x65,
    0x72, 0x73, 0x69, 0x6f, 0x6e, 0x22, 0x3a, 0x22, 0x31, 0x2e, 0x38, 0x2e,
    0x34, 0x22, 0x7d};

// uint8_t beacon_raw[] = {
//   0x80, 0x00,             // 0-1: Frame Control
//   0x00, 0x00,             // 2-3: Duration
//   0xff, 0xff, 0xff, 0xff, 0xff, 0xff,       // 4-9: Destination address
//   (broadcast) 0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,       // 10-15: Source
//   address 0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,       // 16-21: BSSID 0x00,
//   0x00,             // 22-23: Sequence / fragment number 0x00, 0x01, 0x02,
//   0x03, 0x04, 0x05, 0x06, 0x07,     // 24-31: Timestamp (GETS OVERWRITTEN TO
//   0 BY HARDWARE) 0x64, 0x00,             // 32-33: Beacon interval 0x31,
//   0x04,             // 34-35: Capability info 0x00, 0x00, /* FILL CONTENT
//   HERE */       // 36-38: SSID parameter set, 0x00:length:content 0x01, 0x08,
//   0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, // 39-48: Supported rates
//   0x03, 0x01, 0x01,           // 49-51: DS Parameter set, current channel 1
//   (= 0x01), 0x05, 0x04, 0x01, 0x02, 0x00, 0x00,       // 52-57: Traffic
//   Indication Map
//
// };

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len,
                            bool en_sys_seq);

void loop() {
  M5Cardputer.update();
  M5Cardputer.Display.setTextPadding(15);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextFont(&fonts::Font0);

  if (state == STATE_HALT) {
    return;
  }

  if (state == STATE_INIT) {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(15, 15);
    M5Cardputer.Display.println("Sending Pwngrid beacon frame...");
    state++;
  }

  esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
  current_channel = (current_channel + 1) % max_channel;
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html#_CPPv417esp_wifi_80211_tx16wifi_interface_tPKvib
  esp_err_t result = esp_wifi_80211_tx(WIFI_IF_AP, pwngrid_beacon,
                                       sizeof(pwngrid_beacon), false);
  //  esp_err_t result = esp_wifi_80211_tx(WIFI_IF_STA, pwngrid_beacon,
  //  sizeof(pwngrid_beacon), false);
  delay(random(0, 10));

  if (result == ESP_ERR_WIFI_IF) {
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println("Error: invalid interface");
    state = STATE_HALT;
  } else if (result == ESP_ERR_INVALID_ARG) {
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println("Error: invalid argument");
    state = STATE_HALT;
  }

  if (state == STATE_ADVT) {
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.println("Advertisement started! (x_x)");
    state++;
  }
}
