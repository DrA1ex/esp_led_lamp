# Remote Audio DSP — wire protocol v1

Remote Audio is compiled independently of the ADC microphone via
`REMOTE_AUDIO` in `src/constants.h` (default: ENABLED). The existing
`AUDIO` remains DISABLED. Remote audio frames never call
`Application::update()`, never save presets and never write flash.
`main` remains unchanged.

## WebSocket

Open `ws://DEVICE_IP/audio`. This endpoint carries only remote audio; the
existing `/ws` commands, HTTP and UDP port 1423 are unchanged.

Control messages are short JSON **text** messages. All commands after
`hello` must provide the same nonzero uint32 `session` and must come from
the same WebSocket client. Only one client may own the session at a time.

Send in this order:

1. `{"type":"hello","version":1,"session":12345}` ->
   `hello_ack` with dimensions, 32-band count, fps and capabilities.
2. `{"type":"sync","session":12345,"t0":123456789}` ->
   `sync_ack` with `t0`, `t1` (device receive), `t2` (device reply).
   Capture `t3` as soon as the sender receives `sync_ack`.
3. Compute device-minus-sender monotonic clock offset, treating all
   timestamps as modulo-2^32 uint32 milliseconds:

   `offset = ((t1 - t0) + (t2 - t3)) / 2`

   `rtt = (t3 - t0) - (t2 - t1)`

   For subtractions use signed modular differences and median multiple
   measurements for improved accuracy.
4. `{"type":"clock","session":12345,"offset_ms":-28,"rtt_ms":7}`
   -> `clock_ack`. Update the offset only before `start`.
5. Optional temporary config, e.g.
   `{"type":"config","session":12345,"audio_effect":3,"audio_min":0,"audio_max":255,"speed":100}`
   -> `config_ack`.
6. `{"type":"start","session":12345}` -> `start_ack`.
   Start sending binary frames to play audio.
7. `{"type":"status","session":12345}` returns diagnostics, including
   received/dropped/duplicate/stale/overflow frames, frame age, clock offset,
   RTT and buffer occupancy. `status` is infrequent, never per frame.
8. `{"type":"stop","session":12345}` -> `stop_ack`. Any disconnect or
   stream timeout also restores the ordinary visual effect.

All invalid control messages return an `error` object. `config` is an
atomic, runtime-only override and cannot persist a preset. Supported
optional fields: `audio_effect` (0–11), `audio_enabled` (boolean), `audio_min`, `audio_max`,
`color_effect` (0–9), `brightness_effect` (0–4), `palette` (valid
device palette index), `speed`, `scale`, `light` (all 0–255).
The saved preset, power, gamma and eco settings remain authoritative.

## AUDIO_FRAME

Exactly **102 binary bytes**, little endian, with manual serialization
and validation rather than a packed C++ struct:

| Offset | Field | Type |
|---|---|---|
| 0 | Magic `ELAF` | 4 ASCII bytes |
| 4 | Version 1 | uint8 |
| 5 | Type 1 | uint8 |
| 6 | Flags (bit 0 = live input) | uint8 |
| 7 | Band count 32 | uint8 |
| 8 | Session ID | uint32 |
| 12 | Sequence | uint32 |
| 16 | Sender monotonic presentation milliseconds | uint32 |
| 20 | RMS | uint16 |
| 22 | Bass | uint16 |
| 24 | Mid | uint16 |
| 26 | High | uint16 |
| 28 | Peak | uint16 |
| 30 | Onset strength | uint16 |
| 32 | Onset mask (bits low/mid/high) | uint8 |
| 33 | Beat phase (0–255) | uint8 |
| 34 | BPM × 10 (0 if unknown) | uint16 |
| 36 | Beat confidence | uint8 |
| 37 | Reserved 0 | uint8 |
| 38 | 32 logarithmic frequency bands (~40 Hz–8 kHz) | 32 × uint16 |

Amplitudes and bands must be within 0–4096 inclusive. The session ID
must match the active sender. Sequence numbers are tracked in a 32-packet
reorder/replay window with modular arithmetic.

The ESP8266 receiver uses a **24-frame bounded queue** sorted by
presentation time (device clock = sender timestamp + clock offset),
drops frames outside the permitted arrival window and interpolates
continuous RMS/spectral features at the 60 FPS render tick.
Onset masks are treated as one-shot events rather than interpolated.
The renderer maintains a 25 ms rolling RMS history for Wave modes.
The stream timeout is 300 ms once the buffered frames are consumed.

## Existing audio modes

Original IDs, unchanged:

| ID | Audio mode |
|---|---|
| 0 | Wave |
| 1 | Wave Center |
| 2 | Spectrum |
| 3 | Spectrum Center |
| 4 | Brightness |
| 5 | Line |
| 6 | Line Center |
| 7 | Speed Control |
| 8 | Scale Control |
| 9 | Light Control |
| 10 | Bass / Mid / High (WASM experimental) |
| 11 | Beat-aware reactive (WASM experimental) |

Spectrum maps all 32 bands to the matrix width; the centered version maps
all bands to the appropriate half-width. Speed, Scale and Light operate on
a temporary preset copy **before** running ColorEffects and
BrightnessEffects.

### New experimental modes from the WASM lab

The two appended modes have the same color-preserving design as
`esp_led_wasm_lab/src/render.cpp` (modes 11 and 12). They modulate
the *existing* framebuffer by multiplying RGB channels (never inserting
white flashes or standalone bright pixels).

* `Bass / Mid / High` uses spectral content across columns, low/mid/high
  energy and decaying low/mid/high onset pulses. The upper and lower half
  respond to different bands. The receiver approximates the browser's
  individual onset envelopes from packet onset masks/strength.
* `Beat-aware reactive` uses the 32-band spectrum and a center-origin
  radial beat ripple. The wire protocol transports beat phase/confidence
  but not the browser DSP's separate `beatPulse`, so the receiver derives
  a short pulse from phase/confidence and onset strength.

These are not pixel-identical reproductions of the browser implementation;
the protocol carries fewer transient/tempo details than the lab's full
159-feature WASM array.

### Compatibility with the supplied browser lab

`esp_led_wasm_lab/web/remote.js` uses an earlier provisional control
contract: hello without session, `sync.id`, floating-point clock offsets,
and a `start.session_id`. The receiver recognizes this only for a
`hello` with `client:"esp-led-wasm-lab"`. The browser's mode
numbers are shifted because it has an explicit `Off` item:
browser 0 = Off, browser 1..10 = firmware 0..9,
browser 11..12 = firmware 10..11. Browser pause/stop preserves the
connected control lease and synchronization, allowing a new `start`
without disconnecting.

**Only 32 frequency bands** are accepted and advertised by the ESP8266
receiver. Select 32 in Remote Output; the lab's optional 16/64 band
packets are not part of this fixed-size RFC and will be rejected.

The lab-specific `Audio strength`, `Spectrum expression`,
`Spectrum onset`, `Speed floor` and `Line fast response`
modulation sliders are not transmitted by the current browser
`remoteRuntimeConfig()`, so the ESP applies the core effects
using the transmitted features, not all the experimental preview
post-processing knobs.

## Build and test

PlatformIO: `pio run -e release`. GitHub Actions uploads the generated `firmware.bin` as the `esp8266-remote-audio-firmware` artifact (14-day retention) after a successful branch build.
Host parser tests:

```sh
g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_audio_protocol.cpp -o /tmp/remote_audio_protocol
/tmp/remote_audio_protocol
g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_audio_timeline.cpp -o /tmp/remote_audio_timeline
/tmp/remote_audio_timeline
```

Synthetic sender:

```sh
python -m pip install websockets
python tools/remote_audio_sender.py ws://192.168.4.1/audio --hz 40 --seconds 30 --effect 3
```

Use `--effect 0` through `11` for twelve firmware modes (the sender's default CLI validation will be updated accordingly). `--lead-ms 45`
controls the scheduled presentation lead (reduce for a lower-latency
LAN). For normal Wi-Fi setup change `WIFI_MODE` to `WIFI_STA_MODE` in
`constants.h` and configure `WIFI_SSID`/`WIFI_PASSWORD` in
`src/credentials.h`; the device prints its STA IP in serial debug.

**Security:** The HTTP/WebSocket server uses the existing
`WEB_AUTH` mechanism, with the existing 192.168/16 LAN exception. Do
not forward port 80 or publish the unauthenticated LAN endpoint on the
Internet; use a trusted network or VPN. Non-192.168 networks may require
Basic authentication depending on the current WebAuthHandler settings.

## Caveats

The parser/timeline tests run on native C++. Physical matrix tests,
real-world synchronization/latency, websocket fragment fuzzing and
long-duration memory stress testing require an ESP8266 device.
The two supplementary WASM experimental reactive modes are included as
separate enum entries, without changing the ten stock mode IDs.
The physical matrix, browser control path, real-world latency and
synchronization accuracy still require device-side measurements.
