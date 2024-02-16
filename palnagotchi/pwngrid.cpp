#include "pwngrid.h"

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

DynamicJsonDocument peer_json(2048);
String json_str = "";
int json_len = 0;

esp_err_t advertisePalnagotchi(uint8_t channel) {
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
  peer_json["policy"]["sad_num_epochs"] = 0;
  peer_json["policy"]["excited_num_epochs"] = 9999;

  int json_len = measureJson(peer_json);
  serializeJson(peer_json, json_str);
  uint8_t header_len = 2 + (json_len / 255 * 2);
  uint8_t pwngrid_beacon_frame[raw_beacon_len + json_len + header_len];
  memcpy(pwngrid_beacon_frame, pwngrid_beacon_raw, raw_beacon_len);

  // Iterate through json string and copy it to beacon frame
  int frame_byte = raw_beacon_len;
  for (int i = 0; i < json_len; i++) {
    // Write AC and len tags before every 255 bytes
    if (i == 0 || i % 255 == 0) {
      pwngrid_beacon_frame[frame_byte++] = 0xde;  // AC = 222
      uint8_t payload_len = 255;
      if (json_len - i < 255) {
        payload_len = json_len - i;
      }

      pwngrid_beacon_frame[frame_byte++] = payload_len;
    }

    // Append json byte to frame
    // If current byte is not ascii, add ? instead
    uint8_t next_byte = (uint8_t)'?';
    if (isAscii(json_str[i])) {
      next_byte = (uint8_t)json_str[i];
    }

    pwngrid_beacon_frame[frame_byte++] = next_byte;
  }

  // vTaskDelay(500 / portTICK_PERIOD_MS);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html#_CPPv417esp_wifi_80211_tx16wifi_interface_tPKvib
  vTaskDelay(102 / portTICK_PERIOD_MS);
  esp_err_t result = esp_wifi_80211_tx(WIFI_IF_AP, pwngrid_beacon_frame,
                                       sizeof(pwngrid_beacon_frame), true);
  return result;
}

// Detect pwnagotchi adapted from Marauder
// https://github.com/justcallmekoko/ESP32Marauder/wiki/detect-pwnagotchi
// https://github.com/justcallmekoko/ESP32Marauder/blob/master/esp32_marauder/WiFiScan.cpp#L2255
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
