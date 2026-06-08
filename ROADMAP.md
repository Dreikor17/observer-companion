# MeshCore - Companion Observer — Roadmap

Rough, unordered list of where this fork is headed. Contributions welcome.

## Done

- [x] Bring agessaman's MQTT observer/bridge onto the **companion radio** role.
- [x] First build target: `Heltec_v3_companion_radio_observer_mqtt` (USB companion + MQTT uplink).
- [x] Feed `logRx` / `logRxRaw` / `logTx` into the bridge via a glue module that avoids the
      `NodePrefs` name clash.
- [x] Repaired the stock `companion_radio` example so it compiles (upstream `getOutboundTotal` →
      `getOutboundCount` rename had rotted, since flex CI never built the companion role).
- [x] Strip upstream-author-specific CI (private flasher push) and Pages/CNAME config.

## Next

- [ ] **Runtime configuration on the companion** — set WiFi credentials and broker presets from the
      MeshCore app instead of build-time flags. Add new `CMD_*` codes handled in
      `MyMesh::handleCmdFrame()`, persist them in the companion `DataStore`, and feed them to the
      bridge. (Today config is baked in via `OBSERVER_WIFI_SSID` / `MQTT_DEFAULT_SLOTn_PRESET`.)
- [ ] **More boards** — template the `companion_radio_observer_mqtt` env to the other ESP32 targets
      (Heltec V4, RAK3112, T-Beam SX1262/SX1276, Station G2, Xiao S3 / S3 WIO, LilyGo T3S3, …). The
      env name suffix `observer_mqtt` makes them build automatically in the full-fleet workflow.
- [ ] **BLE-companion variant** — app over BLE (frees the USB serial for MQTT debug). Needs RAM
      validation on non-PSRAM boards, since BLE + WiFi + TLS is tight on 320 KB-class parts.
- [ ] **Optional SNMP** — bring `WITH_SNMP` parity with the repeater observer for monitoring.
- [ ] **Upstream-sync docs/automation** — document/streamline merging `upstream/dev` (core) and
      `agessaman/mqtt-bridge-implementation-flex` (MQTT) so the fork stays current.

## Out of scope

- **WiFi station mode** — already part of MeshCore and used by the uplink; nothing to add here.
