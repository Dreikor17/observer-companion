# MeshCore - Observer Companion

**MeshCore companion firmware that also uplinks the packets it hears to MQTT over WiFi.**

Observer Companion is a fork of [MeshCore](https://github.com/meshcore-dev/MeshCore) that brings
the MQTT "observer" uplink — originally built by [agessaman](https://github.com/agessaman/MeshCore)
for the repeater and room-server roles — onto the **companion radio** firmware. The result is a
single device that behaves as a normal MeshCore companion (pair it with the phone/web app) *and*
publishes every mesh packet it receives to one or more MQTT brokers (LetsMesh, MeshMapper, and the
other community map/analyzer services) for monitoring and mapping.

WiFi station mode is already part of MeshCore; this project wires the MQTT bridge into the companion
build and lets it own the WiFi connection for the uplink.

---

## Status

This is an early, working MVP focused on one board (Heltec WiFi LoRa 32 V3):

- ✅ New build target `Heltec_v3_companion_radio_observer_mqtt` — a USB companion + MQTT uplink.
- ✅ Heard packets (`logRx`) and self-originated adverts (`logTx`), plus raw radio data, are fed to
  the MQTT bridge and published using the same slot/preset system as the repeater observer.
- ✅ Config is supplied at **build time** (WiFi credentials + broker presets via build flags).
- 🚧 Runtime configuration on the companion (via the app, no reflash) is on the [roadmap](ROADMAP.md).
- 🚧 More boards, and a BLE-companion variant, are on the [roadmap](ROADMAP.md).

> The MQTT uplink itself (slots, presets, JWT signing, reconnection, JSON formats) is the mature
> implementation from agessaman's branch. See **[MQTT_IMPLEMENTATION.md](MQTT_IMPLEMENTATION.md)**
> for the full reference.

---

## Build & flash

You'll need [PlatformIO](https://platformio.org/). The build embeds a TLS root-certificate bundle,
so the first build downloads it (network required).

1. Set your WiFi and (optionally) brokers. Either uncomment and edit the placeholders in
   `variants/heltec_v3/platformio.ini` under `[env:Heltec_v3_companion_radio_observer_mqtt]`, or pass
   them on the command line:

   ```sh
   pio run -e Heltec_v3_companion_radio_observer_mqtt \
     --project-option="build_flags=-D OBSERVER_WIFI_SSID='\"my-wifi\"' -D OBSERVER_WIFI_PWD='\"my-pass\"'"
   ```

   Key build flags:

   | Flag | Purpose | Default |
   |------|---------|---------|
   | `OBSERVER_WIFI_SSID` | WiFi SSID for the uplink | *(unset → uplink disabled)* |
   | `OBSERVER_WIFI_PWD` | WiFi password | *(unset)* |
   | `MQTT_DEFAULT_SLOT1_PRESET` | First broker preset | `analyzer-us` |
   | `MQTT_DEFAULT_SLOT2_PRESET` | Second broker preset | `analyzer-eu` |
   | `MQTT_DEFAULT_IATA` | Optional IATA airport code tag | *(empty)* |

   Broker preset names (e.g. `analyzer-us`, `meshmapper`, `meshrank`, `cascadiamesh`, …) are listed
   in [MQTT_IMPLEMENTATION.md](MQTT_IMPLEMENTATION.md). **If `OBSERVER_WIFI_SSID` is left unset, the
   firmware builds and runs as a plain companion with the uplink disabled.**

2. Build and flash:

   ```sh
   pio run -e Heltec_v3_companion_radio_observer_mqtt -t upload
   ```

3. Connect the MeshCore app to the device over **USB** as usual. On the serial console you can watch
   the device boot; once WiFi associates, it begins publishing heard packets to your configured
   brokers.

> **Why USB?** This MVP keeps the app on the USB serial link and lets the MQTT bridge own the WiFi
> radio for the uplink. (MQTT debug logging is intentionally disabled in this target so it can't
> corrupt the app's binary serial frames.)

---

## How it works

The MQTT bridge is self-contained (`src/helpers/bridges/MQTTBridge.*`, `MQTTMessageBuilder`,
`JWTHelper`, `MQTTPresets`) and reads its configuration from a `NodePrefs` struct. The companion
firmware has its *own* `NodePrefs`, so the integration is kept in a thin glue module:

- `examples/companion_radio/MqttObserver.{h,cpp}` — owns the bridge and a CommonCLI-style
  `NodePrefs`. Only the `.cpp` includes `CommonCLI.h`, so the two `NodePrefs` definitions never
  collide. The header exposes a tiny API (`begin / loop / onRx / onRxRaw / onTx`).
- `examples/companion_radio/MyMesh.cpp` — overrides `logRx` / `logRxRaw` / `logTx` (the same hooks
  the repeater observer uses) and forwards packets to the glue. All of this is guarded by
  `#ifdef WITH_MQTT_BRIDGE`, so the stock companion builds are unaffected.

---

## Keeping up to date with upstream

This repo is a fork of `meshcore-dev/MeshCore`, with the MQTT work tracked from
`agessaman/MeshCore`. Typical remotes:

```sh
git remote add upstream  https://github.com/meshcore-dev/MeshCore.git   # core updates
git remote add agessaman https://github.com/agessaman/MeshCore.git      # MQTT observer updates
git fetch upstream && git merge upstream/dev      # pull core changes
```

---

## Roadmap

See **[ROADMAP.md](ROADMAP.md)**.

---

## Credits

- **[MeshCore](https://github.com/meshcore-dev/MeshCore)** by Scott Powell (ripplebiz) and
  contributors — the underlying mesh firmware. Released under the MIT License.
- **[agessaman/MeshCore](https://github.com/agessaman/MeshCore)** (`mqtt-bridge-implementation-flex`)
  — the MQTT observer/bridge implementation that this project builds on.

Observer Companion only adds the companion-role integration, build target, and packaging on top of
their work. Please support the upstream projects and the broader MeshCore community.

## License

MIT — see [license.txt](license.txt). Same license as upstream MeshCore.
