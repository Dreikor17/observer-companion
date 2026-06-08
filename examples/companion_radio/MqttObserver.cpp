#include "MqttObserver.h"

#ifdef WITH_MQTT_BRIDGE

#include <Arduino.h>
#include <string.h>
#include <MeshCore.h>
#include <Mesh.h>
#include <Utils.h>
#include <helpers/CommonCLI.h>          // defines the NodePrefs the bridge reads
#include <helpers/MQTTDefaults.h>       // applyMQTTDefaults helpers + MQTT_DEFAULT_* macros
#include <helpers/bridges/MQTTBridge.h>

// Build-time WiFi credentials for the uplink. We deliberately use OBSERVER_WIFI_*
// instead of WIFI_SSID/WIFI_PWD: the companion's main.cpp switches its app
// transport to TCP (and calls WiFi.begin() itself) when WIFI_SSID is defined.
// Here we want the app to stay on USB/BLE while the MQTT bridge owns the WiFi STA.
#ifndef OBSERVER_WIFI_SSID
#define OBSERVER_WIFI_SSID ""
#endif
#ifndef OBSERVER_WIFI_PWD
#define OBSERVER_WIFI_PWD ""
#endif

namespace {
  // CommonCLI NodePrefs is large; keep it in .bss rather than on a stack.
  NodePrefs   s_prefs;
  MQTTBridge* s_bridge = nullptr;
}

namespace MqttObserver {

void begin(mesh::Dispatcher* dispatcher,
           mesh::Radio* radio,
           mesh::MainBoard* board,
           mesh::MillisecondClock* ms,
           mesh::RTCClock* rtc,
           mesh::PacketManager* mgr,
           mesh::LocalIdentity* identity,
           const char* node_name,
           const char* firmware_version,
           const char* build_date,
           float radio_freq, float radio_bw, uint8_t radio_sf, uint8_t radio_cr) {
  if (s_bridge) return;  // already started

  memset(&s_prefs, 0, sizeof(s_prefs));

  // Node name is used as the MQTT origin when mqtt_origin is empty.
  if (node_name) {
    strncpy(s_prefs.node_name, node_name, sizeof(s_prefs.node_name) - 1);
  }

  // Radio params feed the status message's radio_info ("freq,bw,sf,cr").
  // The bridge reads these from prefs; the companion's real values live in its
  // own NodePrefs, so they must be copied in here (otherwise they publish as 0).
  s_prefs.freq = radio_freq;
  s_prefs.bw   = radio_bw;
  s_prefs.sf   = radio_sf;
  s_prefs.cr   = radio_cr;

  // Message defaults (mirror applyMQTTDefaults() in MQTTDefaults.h).
  s_prefs.mqtt_status_enabled  = 1;
  s_prefs.mqtt_packets_enabled = 1;
  s_prefs.mqtt_raw_enabled     = 0;
  s_prefs.mqtt_tx_enabled      = 1;   // 0=off, 1=all TX, 2=self-advert only ("mqtt.tx all")
  s_prefs.mqtt_rx_enabled      = 1;
  s_prefs.mqtt_status_interval = 300000;
  s_prefs.wifi_power_save      = 1;

  // Broker slots — defaults overridable via -D MQTT_DEFAULT_SLOTn_PRESET.
  mqttDefaultSlotPreset(s_prefs.mqtt_slot_preset[0], sizeof(s_prefs.mqtt_slot_preset[0]), MQTT_DEFAULT_SLOT1_PRESET);
  mqttDefaultSlotPreset(s_prefs.mqtt_slot_preset[1], sizeof(s_prefs.mqtt_slot_preset[1]), MQTT_DEFAULT_SLOT2_PRESET);
  mqttDefaultSlotPreset(s_prefs.mqtt_slot_preset[2], sizeof(s_prefs.mqtt_slot_preset[2]), MQTT_DEFAULT_SLOT3_PRESET);
  mqttDefaultSlotPreset(s_prefs.mqtt_slot_preset[3], sizeof(s_prefs.mqtt_slot_preset[3]), MQTT_DEFAULT_SLOT4_PRESET);
  mqttDefaultSlotPreset(s_prefs.mqtt_slot_preset[4], sizeof(s_prefs.mqtt_slot_preset[4]), MQTT_DEFAULT_SLOT5_PRESET);
  mqttDefaultSlotPreset(s_prefs.mqtt_slot_preset[5], sizeof(s_prefs.mqtt_slot_preset[5]), MQTT_DEFAULT_SLOT6_PRESET);

  // Optional IATA + timezone (build-flag overridable).
  if (MQTT_DEFAULT_IATA[0] != '\0') {
    strncpy(s_prefs.mqtt_iata, MQTT_DEFAULT_IATA, sizeof(s_prefs.mqtt_iata) - 1);
  }
  if (MQTT_DEFAULT_TIMEZONE[0] != '\0') {
    strncpy(s_prefs.timezone_string, MQTT_DEFAULT_TIMEZONE, sizeof(s_prefs.timezone_string) - 1);
  }
  s_prefs.timezone_offset = MQTT_DEFAULT_TIMEZONE_OFFSET;

  // WiFi credentials baked in at build time.
  strncpy(s_prefs.wifi_ssid, OBSERVER_WIFI_SSID, sizeof(s_prefs.wifi_ssid) - 1);
  strncpy(s_prefs.wifi_password, OBSERVER_WIFI_PWD, sizeof(s_prefs.wifi_password) - 1);

  // No WiFi configured -> behave as a plain companion, don't start the uplink.
  if (s_prefs.wifi_ssid[0] == '\0') return;

  s_bridge = new MQTTBridge(&s_prefs, mgr, rtc, identity);
  if (!s_bridge) return;

  // Device metadata for status/packet JSON (mirrors simple_repeater setBridgeState()).
  if (identity) {
    char device_id[65];
    mesh::Utils::toHex(device_id, identity->pub_key, PUB_KEY_SIZE);
    s_bridge->setDeviceID(device_id);
  }
  s_bridge->setFirmwareVersion(firmware_version ? firmware_version : "");
  if (board) s_bridge->setBoardModel(board->getManufacturerName());
  s_bridge->setBuildDate(build_date ? build_date : "");
  s_bridge->setStatsSources(dispatcher, radio, board, ms);
  s_bridge->begin();
}

void loop() {
  if (s_bridge) s_bridge->loop();
}

void onRxRaw(float snr, float rssi, const uint8_t* raw, int len) {
  if (s_bridge) s_bridge->storeRawRadioData(raw, len, snr, rssi);
}

void onRx(mesh::Packet* pkt) {
  if (s_bridge && pkt) s_bridge->onPacketReceived(pkt);
}

void onTx(mesh::Packet* pkt) {
  if (s_bridge && pkt) s_bridge->sendPacket(pkt);
}

bool isRunning() {
  return s_bridge != nullptr;
}

void getStatusLine(char* buf, size_t buf_size) {
  if (buf_size == 0) return;
  if (!s_bridge) {
    strncpy(buf, "MQTT: off", buf_size);
  } else if (WiFi.status() != WL_CONNECTED) {
    strncpy(buf, "WiFi: connecting", buf_size);
  } else {
    IPAddress ip = WiFi.localIP();
    snprintf(buf, buf_size, "%u.%u.%u.%u  MQTT:%d",
             ip[0], ip[1], ip[2], ip[3], s_bridge->getConnectedBrokers());
  }
  buf[buf_size - 1] = '\0';
}

} // namespace MqttObserver

#endif // WITH_MQTT_BRIDGE
