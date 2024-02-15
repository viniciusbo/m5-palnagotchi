#include "M5Cardputer.h"
#include "M5Unified.h"
#include "ArduinoJson.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
//#include "esp_system.h"
//#include "esp_event.h"
//#include "esp_event_loop.h"

#define ARROW_UP ';'
#define ARROW_DOWN '.'

uint8_t pwngrid_signature_addr[] = { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad };
uint8_t pwngrid_broadcast_addr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

//bool check_prev_press() {
//  if (M5Cardputer.Keyboard.isKeyPressed(ARROW_UP)) {
//    return true;
//  }
//
//  return false;
//}
//
//bool check_next_press() {
//  if (M5Cardputer.Keyboard.isKeyPressed(ARROW_DOWN)) {
//    return true;
//  }
//
//  return false;
//}

void init_m5() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextDatum(middle_center);
//  M5Cardputer.Display.setTextFont(&fonts::Orbitron_Light_24);
  M5Cardputer.Display.setTextFont(&fonts::FreeSans9pt7b);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.drawString("Palnagotchi", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2);
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

typedef struct
{
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

const wifi_promiscuous_filter_t filt = {.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};

void getMAC(char *addr, uint8_t* data, uint16_t offset) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5]);
}

void pwnSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{ 
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String src = "";
  String essid = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *) snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
    
    if ((snifferPacket->payload[0] == 0x80) && (buf == 0))
    {
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
          Serial.print((char) snifferPacket->payload[i + 38]);
          if (isAscii(snifferPacket->payload[i + 38])) {
            essid.concat((char) snifferPacket->payload[i + 38]);
          }
        }

        DynamicJsonDocument json(1024); // ArduinoJson v6
        if (deserializeJson(json, essid)) {
          Serial.println("\nCould not parse Pwnagotchi json");
          display_string.concat(essid);
        } else {
          Serial.println("\nSuccessfully parsed json");
          String json_output;
          serializeJson(json, json_output); // ArduinoJson v6
          Serial.println(json_output);
          display_string.concat(json["name"].as<String>() + " pwnd: " + json["pwnd_tot"].as<String>());
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
//  ap_config.ap.ssid = "{name:\"Palnagotchi\",session_id:\"c7:87:3e:b0:c1:f1\",epoch:0,face=\"(x_x)\"}";
  ap_config.ap.ssid_len = 0;
  
  wifi_init_config_t WIFI_INIT_CONFIG = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&WIFI_INIT_CONFIG);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
//  esp_wifi_set_mac(WIFI_IF_AP, pwngrid_signature_addr);
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&pwnSnifferCallback);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_ps(WIFI_PS_NONE);

  delay(2000);
}

uint8_t current_channel = 1;
uint8_t max_channel = 14;

// Need to forge the Dot11 packet on Scapy then hardcode it here
// https://forum.micropython.org/viewtopic.php?p=68811
// https://www.4armed.com/blog/forging-wifi-beacon-frames-using-scapy/
// But first I gotta reverse eng the Pwngrid Dot11 protocol
// https://github.com/evilsocket/pwngrid/blob/master/wifi/pack.go#L22
// This is the hexdump of the forged beacon frame, maybe we can manipulate the right bytes here to modify Pwngrid advertisemenet in ESSID
// Idk why scapy dumped the first byte as 0x00 instead of 0x80
  /*
0000  00 00 08 00 00 00 00 00 80 00 00 00 FF FF FF FF  ................
0010  FF FF DE AD BE EF DE AD C7 87 3E B0 C1 F1 00 00  ..........>.....
0020  00 00 00 00 00 00 00 00 64 00 11 00 00 AA 7B 6E  ........d.....{n
0030  61 6D 65 3A 22 50 61 6C 6E 61 67 6F 74 63 68 69  ame:"Palnagotchi
0040  22 2C 69 64 65 6E 74 69 74 79 3A 22 33 32 65 39  ",identity:"32e9
0050  66 33 31 35 65 39 32 64 39 37 34 33 34 32 63 39  f315e92d974342c9
0060  33 64 30 66 64 39 35 32 61 39 31 34 62 66 62 34  3d0fd952a914bfb4
0070  65 36 38 33 38 39 35 33 35 33 36 65 61 36 66 36  e6838953536ea6f6
0080  33 64 35 34 64 62 36 62 39 36 31 30 22 2C 73 65  3d54db6b9610",se
0090  73 73 69 6F 6E 5F 69 64 3A 22 63 37 3A 38 37 3A  ssion_id:"c7:87:
00a0  33 65 3A 62 30 3A 63 31 3A 66 31 22 2C 67 72 69  3e:b0:c1:f1",gri
00b0  64 5F 76 65 72 73 69 6F 6E 3A 22 31 2E 31 30 2E  d_version:"1.10.
00c0  33 22 2C 65 70 6F 63 68 3A 30 2C 66 61 63 65 3D  3",epoch:0,face=
00d0  22 28 78 5F 78 29 22 7D 30 1C 01 00 00 0F C2 AC  "(x_x)"}0.......
00e0  02 02 00 00 0F C2 AC 04 00 0F C2 AC 02 01 00 00  ................
00f0  0F C2 AC 02 00 00                                ......
  */
uint8_t pwngrid_beacon[] = {
  0x80, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xC7, 0x87, 0x3E, 0xB0, 0xC1, 0xF1, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x11, 0x00, 0x00, 0xAA, 0x7B, 0x6E,
  0x61, 0x6D, 0x65, 0x3A, 0x22, 0x50, 0x61, 0x6C, 0x6E, 0x61, 0x67, 0x6F, 0x74, 0x63, 0x68, 0x69,
  0x22, 0x2C, 0x69, 0x64, 0x65, 0x6E, 0x74, 0x69, 0x74, 0x79, 0x3A, 0x22, 0x33, 0x32, 0x65, 0x39,
  0x66, 0x33, 0x31, 0x35, 0x65, 0x39, 0x32, 0x64, 0x39, 0x37, 0x34, 0x33, 0x34, 0x32, 0x63, 0x39,
  0x33, 0x64, 0x30, 0x66, 0x64, 0x39, 0x35, 0x32, 0x61, 0x39, 0x31, 0x34, 0x62, 0x66, 0x62, 0x34,
  0x65, 0x36, 0x38, 0x33, 0x38, 0x39, 0x35, 0x33, 0x35, 0x33, 0x36, 0x65, 0x61, 0x36, 0x66, 0x36,
  0x33, 0x64, 0x35, 0x34, 0x64, 0x62, 0x36, 0x62, 0x39, 0x36, 0x31, 0x30, 0x22, 0x2C, 0x73, 0x65,
  0x73, 0x73, 0x69, 0x6F, 0x6E, 0x5F, 0x69, 0x64, 0x3A, 0x22, 0x63, 0x37, 0x3A, 0x38, 0x37, 0x3A,
  0x33, 0x65, 0x3A, 0x62, 0x30, 0x3A, 0x63, 0x31, 0x3A, 0x66, 0x31, 0x22, 0x2C, 0x67, 0x72, 0x69,
  0x64, 0x5F, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3A, 0x22, 0x31, 0x2E, 0x31, 0x30, 0x2E,
  0x33, 0x22, 0x2C, 0x65, 0x70, 0x6F, 0x63, 0x68, 0x3A, 0x30, 0x2C, 0x66, 0x61, 0x63, 0x65, 0x3D,
  0x22, 0x28, 0x78, 0x5F, 0x78, 0x29, 0x22, 0x7D, 0x30, 0x1C, 0x01, 0x00, 0x00, 0x0F, 0xC2, 0xAC,
  0x02, 0x02, 0x00, 0x00, 0x0F, 0xC2, 0xAC, 0x04, 0x00, 0x0F, 0xC2, 0xAC, 0x02, 0x01, 0x00, 0x00,
  0x0F, 0xC2, 0xAC, 0x02, 0x00, 0x00
};

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
  esp_err_t result = esp_wifi_80211_tx(WIFI_IF_AP, pwngrid_beacon, sizeof(pwngrid_beacon), false);
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
