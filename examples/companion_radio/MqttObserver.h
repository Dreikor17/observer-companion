#pragma once

// Observer Companion — MQTT uplink glue for the companion firmware.
//
// This header is deliberately thin: it forward-declares only core mesh:: types
// and exposes a tiny C++ API. It MUST NOT include CommonCLI.h / MQTTBridge.h,
// because those pull in a global `struct NodePrefs` that collides with the
// companion firmware's own NodePrefs (examples/companion_radio/NodePrefs.h).
// All of the MQTT bridge wiring lives in MqttObserver.cpp, which is the only
// companion translation unit that sees CommonCLI's NodePrefs.

#ifdef WITH_MQTT_BRIDGE

#include <stdint.h>

namespace mesh {
  class Packet;
  class Dispatcher;
  class Radio;
  class MainBoard;
  class MillisecondClock;
  class RTCClock;
  class PacketManager;
  class LocalIdentity;
}

namespace MqttObserver {

// Construct and start the MQTT bridge. Call once from MyMesh::begin(), after the
// identity and prefs are loaded. WiFi credentials are baked in at build time via
// the OBSERVER_WIFI_SSID / OBSERVER_WIFI_PWD flags; if no SSID is set the bridge
// is not started and the device behaves as a normal companion.
void begin(mesh::Dispatcher* dispatcher,
           mesh::Radio* radio,
           mesh::MainBoard* board,
           mesh::MillisecondClock* ms,
           mesh::RTCClock* rtc,
           mesh::PacketManager* mgr,
           mesh::LocalIdentity* identity,
           const char* node_name,
           const char* firmware_version,
           const char* build_date);

// Pump the bridge. No-op on ESP32 (the bridge runs its own FreeRTOS task there).
void loop();

// Packet hooks — mirror the repeater observer's logRxRaw/logRx/logTx path.
// Call order per received frame is: onRxRaw() (stages raw bytes), then onRx()
// (queues the decoded packet together with the staged raw data).
void onRxRaw(float snr, float rssi, const uint8_t* raw, int len);
void onRx(mesh::Packet* pkt);
void onTx(mesh::Packet* pkt);

// True once the bridge has been constructed and started.
bool isRunning();

} // namespace MqttObserver

#endif // WITH_MQTT_BRIDGE
