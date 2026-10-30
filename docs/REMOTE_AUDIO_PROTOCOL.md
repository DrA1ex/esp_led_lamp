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
   `hello_ack` with dimensions, the accepted band counts (`band_counts: [16, 32, 64]`), fps, capabilities
   and the visualization effect ids this build renders (`visualizations: [12, …, 29]`). Firmware
   that takes effect parameters lists the `params` capability and `params_version` (1).
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
   Then, optionally, the visualization's effect parameters (the `params`
   capability, see [Effect parameters](#effect-parameters)):
   `{"type":"params","session":12345,"version":1,"effect":25,"values":[0,0,100]}`
   -> `params_ack`.
6. `{"type":"start","session":12345}` -> `start_ack`.
   Start sending binary frames to play audio.
7. `{"type":"status","session":12345}` returns diagnostics, including
   received/dropped/duplicate/stale/overflow frames, frame age, clock offset,
   RTT and buffer occupancy. `status` is infrequent, never per frame.
8. `{"type":"stop","session":12345}` -> `stop_ack`. Any disconnect or
   stream timeout also restores the ordinary visual effect.
9. Lamp controls (the `control` capability), allowed at any time after
   `hello`, streaming or not: `{"type":"power","session":12345,"on":false}`
   and `{"type":"brightness","session":12345,"value":128}` (0–255, the
   lamp's maximum brightness). Both reply
   `{"type":"control_ack","power":true,"brightness":128}`. Unlike `config`
   they persist, like `/api/power` and `/api/brightness`. `hello_ack` and
   `status` also carry the current `power` and `brightness`, so changes
   from the lamp's own UI or button show up within one status period.

All invalid control messages return an `error` object. `config` is an
atomic, runtime-only override and cannot persist a preset. Supported
optional fields: `audio_effect` (0–11, or a listed visualization 12–29), `audio_enabled` (boolean), `audio_min`, `audio_max`,
`color_effect` (0–9), `brightness_effect` (0–4), `palette` (valid
device palette index), `speed`, `scale`, `light` (all 0–255).
The saved preset, power, gamma and eco settings remain authoritative.

Reaction tuning (advertised as the `tuning` capability; every field
defaults to 0 on `hello`, which reproduces the stock firmware effects):

| Field | Range | Effect |
|---|---|---|
| `param_source` | 0–7 | Value driving Brightness, Line, Line Center, Speed/Scale/Light: 0 `rms` (loudness), 1 spectral activity (0.45 · strongest band + 0.55 · mean, ×1.55), 2 `bass`, 3 `mid`, 4 `high`, 5 `peak`, 6 onset envelope, 7 predicted beat pulse |
| `speed_floor` | 0–255 | Speed Control result is mapped into `floor..255` (Gradient stops at 128). The current lab sends 0 and sets the lower bound through `audio_min` and the signal window, so a gated reaction can stop the effect |
| `spectrum_shape` | 0–100 % | Wave/Spectrum brightness blends from `dim8_raw` towards `0.25·v + 0.75·√v` |
| `spectrum_onset` | 0–100 % | Wave/Spectrum columns add `0.22 · onset envelope` of their low/mid/high group |
| `input_min`, `input_max` | 0–255, `min < max` | Signal window: the input range (of the provider maximum) stretched onto `audio_min..audio_max`; above `input_max` gives `audio_max` |
| `input_gate` | bool | Below `input_min`: 0 instead of `audio_min` |
| `input_curve` | 25–400 (exponent × 100) | Curve inside the window: position `t` becomes `t^(input_curve / 100)`. 100 is linear; above 100 keeps low and mid signals dim and spreads the top (LED brightness looks logarithmic, so 200–250 makes the reaction look even); below 100 lifts quiet signals |
| `origin_shift` | −500..500 ‰ of WIDTH | Moves the start of Wave / Spectrum / Line / Bass·Mid·High and the centre of the Center modes and Beat-aware |
| `origin_reverse` | bool | Runs the effect from the other end |
| `origin_ring` | bool | Closed strip: the shifted effect wraps past the last LED instead of leaving the first LEDs dark |
| `origin_radial` | bool | Matrices: Center modes and Beat-aware use the 2-D distance from (WIDTH / 2, `origin_y`) |
| `origin_y` | 0–1000 ‰ of HEIGHT − 1 | Row of the radial centre (default 500) |
| `viz_color` | 0–1 | Visualizations: 0 draws with the palette, 1 uses the level as a mask over the colour effect |
| `viz_mirror` | bool | Visualizations: strips run out from the centre both ways, matrices mirror around the middle row |
| `viz_auto` | bool (default true) | Visualizations: auto-level, each band / meter stretched over its own recent ~3 s range |
| `viz_decay` | 0–100 % (default 50) | Visualizations: meter release, peak fall and trail length |
| `viz_motion` | 0–100 % (default 50) | Visualizations: motion speed (Flow, Pulses, Fire, Aurora, Juggle, Radial Spectrum, Blobs) |

The signal window applies to every audio mode that maps a value
(`audioTransfer()` in `src/audio/transfer.h`). The origin maps each column
`x` to `x' = (reverse ? WIDTH − 1 − x : x) − round(WIDTH · origin_shift / 1000)`,
wrapped modulo WIDTH with `origin_ring` and dark outside `0..WIDTH − 1`
otherwise (`RemoteReactive::OriginMap`); modes are then evaluated at `x'`.
Defaults (`0`, `255`, `false`, `100`, `0`, `500`, `false`) reproduce the stock
output, and the stock `AudioEffectManager` path is used whenever the origin
and spectrum shaping are at their defaults. A config with
`input_min >= input_max` or an out-of-range field is rejected.

These match the lab's new-FFT preview (`src/render.cpp`,
`web/js/view.js`), so the lamp shows what the browser previews.

## AUDIO_FRAME

**38 + 2 × band count bytes** (16 → 70, 32 → 102, 64 → 166), little endian,
with manual serialization and validation rather than a packed C++ struct:

| Offset | Field | Type |
|---|---|---|
| 0 | Magic `ELAF` | 4 ASCII bytes |
| 4 | Version 1 | uint8 |
| 5 | Type 1 | uint8 |
| 6 | Flags (bit 0 = live input) | uint8 |
| 7 | Band count: 16, 32 or 64 | uint8 |
| 8 | Session ID | uint32 |
| 12 | Sequence | uint32 |
| 16 | Sender monotonic presentation milliseconds | uint32 |
| 20 | Loudness (normalised RMS envelope) | uint16 |
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
| 38 | Logarithmic frequency bands (the lab's analysed range, 40 Hz–8 kHz by default) | count × uint16 |

Amplitudes and bands must be within 0–4096 inclusive. The session ID
must match the active sender. Sequence numbers are tracked in a 32-packet
reorder/replay window with modular arithmetic.

The ESP8266 receiver uses a **24-frame bounded queue** sorted by
presentation time (device clock = sender timestamp + clock offset),
drops frames outside the permitted arrival window and interpolates
continuous RMS/spectral features at the 60 FPS render tick.
Onset masks are treated as one-shot events rather than interpolated.
The renderer maintains a 25 ms rolling RMS history for Wave modes
(128 samples, 3.2 s). Senders should put a normalised loudness envelope
(0–4096, full scale for loud passages) into `rms`, not the raw signal RMS:
Line, Brightness and the parametric modes use it directly.
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

Spectrum maps all received bands to the matrix width; the centered version maps
all bands to the appropriate half-width. When there are fewer columns than
bands (short strips such as 7 LEDs, or a centered 16-wide matrix) each column
averages its range of bands instead of sampling single bands.

### LED strips

Strips are the same `Led` with `HEIGHT` 1 (or 2, e.g. 72×2). Remote modes
are geometry-aware:

* Wave / Wave Center use one column per 25 ms sample; spans longer than the
  128-sample history (e.g. a 153-LED strip) stretch the history instead of
  wrapping it, which used to repeat the same pattern along the strip.
* Bass / Mid / High splits rows at `HEIGHT / 2` (upper rows bass, lower mid);
  a single-row strip blends bass and mid (previously it showed bass only and
  disagreed with the lab, which showed mid). Speed, Scale and Light operate on
a temporary preset copy **before** running ColorEffects and
BrightnessEffects.

### New experimental modes from the WASM lab

The two appended modes have the same color-preserving design as
`esp_led_lab/src/render.cpp` (modes 11 and 12). They modulate
the *existing* framebuffer by multiplying RGB channels (never inserting
white flashes or standalone bright pixels).

* `Bass / Mid / High` uses spectral content across columns, low/mid/high
  energy and decaying low/mid/high onset pulses. The upper and lower half
  respond to different bands. The receiver approximates the browser's
  individual onset envelopes from packet onset masks/strength.
* `Beat-aware reactive` uses the received spectrum and a center-origin
  radial beat ripple. The wire protocol transports beat phase, BPM and
  confidence but not the browser DSP's separate `beatPulse`; the receiver
  reconstructs it as the same 230 ms decay after each beat from phase × BPM.

These are not pixel-identical reproductions of the browser implementation;
the protocol carries fewer transient/tempo details than the lab's full
159-feature WASM array.

### Visualizations (12–29)

Remote-only modes appended after the experimental ones, not offered by the
lamp's own UI (`AudioEffects` does not list them). They live in
`src/remote_audio/viz_engine.h`, which is a byte-identical copy of
`esp_led_lab/src/viz_engine.h`, the engine the lab preview renders, so the
lamp computes the same picture (only random sparks differ).
`src/remote_audio/visualizations.h` feeds it the stream and draws with the
current palette.

| Id | Lab | Mode | Strip (HEIGHT < 4, all rows equal) | Matrix |
|---|---|---|---|---|
| 12 | 13 | VU Meter | fill from the origin or both ways from the centre, peak dot | fill from the bottom / middle row, or a circle with `origin_radial` |
| 13 | 14 | Spectrum Bars | bands spread along the LEDs | a bar per column with falling peaks |
| 14 | 15 | Flow | loudness river coloured by tone | scrolling spectrogram |
| 15 | 16 | Pulses | onsets launch comets | onsets launch rings |
| 16 | 17 | Sparkles | treble and onsets sprinkle sparks over a bass glow | same |
| 17 | 18 | Mood | the whole lamp breathes, colour follows the tone | same |
| 18 | 19 | Waveform | loudness history | history bars or a mirrored waveform |
| 19 | 20 | Bass · Mid · Treble | three segments | three bars |
| 20 | 21 | Fire | flames from the start or the centre | a flame per band from the bottom |
| 21 | 22 | Aurora | slow noise growing with the music | same in 2-D |
| 22 | 23 | Juggle | eight band dots swinging with trails | Lissajous dots |
| 23 | 24 | Radial Spectrum | symmetric spectrum rolling along | rotating spectrum sun |
| 24 | 25 | Blobs | four band blobs merging | lava lamp |
| 25 | 26 | Particles | hits launch sparks along the strip | fountains, rain, bursts, bubbles or comets |
| 26 | 27 | Field | a moving pattern lit by the level or the spectrum | same in 2-D (clouds, plasma, rings, stripes, spiral) |
| 27 | 28 | Zones | segments: an equalizer, or flashes on hits | columns, rows, rings, sectors or tiles |
| 28 | 29 | Melody | the dominant frequency as a dot or a bar | a dot, a bar or a scrolling trace |
| 29 | 30 | Groove | waves running from the origin, or a swinging bar: a step per beat | rings, spiral arms, stripes or a swinging ball: a step per beat |

Groove is the mode that keeps time: it runs a beat clock of its own, locked
(a phase-locked loop) to the frame's beat phase while the beat confidence is
above 0.3 (it lets go below 0.2), and moves its pattern one eased step per
beat. Between frames the receiver carries the phase on at the frame's BPM
for the frame's age (`Host::beatPhase()`), so the clock does not see 50 Hz
steps. Without a confident tempo, bass and mid onsets start the steps.

Levels go through the signal window and `audio_min..audio_max` like every
other mode; the origin (shift / reverse / ring / radial centre) applies. The
lab's fast line envelope is not on the wire, so modes following loudness use
the control source (`param_source`). Building with `-DVIZ_WITH_HEAVY=0` drops
the per-pixel modes 20, 21, 23, 24 and 26 (noise, angles, fields) and removes them
from `visualizations`.

The engine is integer only, since the ESP8266 has no FPU and soft-float costs
hundreds of cycles per operation: levels are Q12 (the stream's 0..4096),
time is in ms, 2-D positions are 1/16 LED, phases Q16 turns; square roots
and angles are integer approximations. `tests/check_viz_integer.sh` compiles
it with the xtensa toolchain and fails on any soft-float call. Cost: about
51 KB flash (Groove 4 KB of it) and, for a 16×16 matrix, 2.1 KB RAM (0.9 KB
of per-band meter state, two bytes per LED, the particles and the
parameters; Groove's beat clock shares the particles' memory). The `status` response reports the last
visualization frame time as `viz_us`, which should stay well under the 16 ms
frame budget.

### Effect parameters

Every visualization can take up to 11 parameters, 0–255 each
(`Viz::MAX_PARAMS`, `Viz::paramDefaults()` in `viz_engine.h`; the lab names
them in `web/js/fx-params.js`). Firmware that takes them lists `params` in
`hello_ack.capabilities` with `"params_version":1`; the version changes
whenever a parameter's meaning or order does, so a sender never drives a
lamp with a different layout.

`{"type":"params","session":S,"version":1,"effect":E,"values":[...]}`:

- `effect` is numbered like `config.audio_effect` (the bundled lab, which
  has an Off item, adds 1); without it the running effect is meant. It must
  be a visualization this build renders.
- `values` are in the order below, at most the mode's count; missing ones
  take their defaults.
- Replies `params_ack`, or `error` / `invalid_params` (another version, an
  effect that is not a visualization, too many values, a value outside 0–255).

The values apply while that effect runs; any other effect draws with its
defaults, and they last for the session like `config`. Choices wrap
(a value past the last choice is the first again). *Brightness* scales
everything the mode draws: 128 = as drawn, 255 ≈ ×2.

| Id | Mode | Parameters in order (default) |
|---|---|---|
| 12 | VU Meter | colours: 0 along, 1 by level, 2 by tone (0); peak dot: 0 off (1) |
| 13 | Spectrum Bars | colours: 0 by height, 1 by band, 2 by level (0); falling peaks: 0 off (1) |
| 14 | Flow | — |
| 15 | Pulses | trigger: 0 any onset, 1 bass, 2 mid, 3 treble, 4 beats (0); width, 1/16 LED (24); glow with the level (31) |
| 16 | Sparkles | treble sparks, 64 = ×1 (64); sparks per hit, /256 of the LEDs (13); bass glow (89); colours: 0 by group, 1 random, 2 by tone (0) |
| 17 | Mood | colour spread (48); bass flash (90); tone range (200); drawn from: 0 level, 1 spectrum (0) |
| 18 | Waveform | colours: 0 classic, 1 along the history, 2 by tone (0) |
| 19 | Bass · Mid · Treble | — |
| 20 | Fire | fuel: 0 bands, 1 bass, 2 loudness, 3 hits only (0); height (170); intensity (140); flares on hits (140); wind, 128 still (128); flames merge (140); colours: 0 heat, 1 palette by heat, 2 palette by tone (0); embers (60) |
| 21 | Aurora | zoom, 64 = ×1 (64); threshold (128); colour spread (90); bass flash (77); drawn from: 0 level, 1 spectrum (0) |
| 22 | Juggle | dots 1–8 (8); dot size, 128 = ×1 (128) |
| 23 | Radial Spectrum | spin, 128 = ×1 (128); bass core (90) |
| 24 | Blobs | size, 128 = ×1 (128); blobs 1–4 (4); softness (127) |
| 25 | Particles | shape: 0 fountain, 1 rain, 2 bursts, 3 bubbles, 4 comets (0); launched by: 0 band hits, 1 bass, 2 mid, 3 treble, 4 any hit, 5 beats, 6 a stream with the level (0); amount, 1 + v·15/255 per hit (100); speed, (1024 + 81v)/4096 lamps/s (100); gravity, 128 none (170); spread (90); life, 150 + 15v ms (110); trail, 20 + 4v ms (90); colours: 0 by frequency, 1 by tone, 2 random, 3 cooling (0); brightness (128) |
| 26 | Field | pattern: 0 clouds, 1 plasma, 2 rings, 3 stripes, 4 spiral (0); zoom, (8 + v)/32 waves across (100); flow (80); lit by: 0 level, 1 spectrum (1); driven by: 0 loudness, 1 bass, 2 mid, 3 treble, 4 beats (1); pump (120); kick on hits (100); contrast (60); colour spread (120); colour drift (30); brightness (128) |
| 27 | Zones | zones 1–16 (8); layout: 0 columns, 1 rows, 2 rings, 3 sectors, 4 tiles (0); lit by: 0 equalizer, 1 band hits, 2 bass, 3 mid, 4 treble, 5 any hit, 6 beats (0); on a hit: 0 all, 1 chase, 2 random, 3 alternate, 4 bounce, 5 build up (1); decay, 50 + 6v ms (100); glow with the level (40); colours: 0 by zone, 1 by what hit, 2 new each hit, 3 by tone (0); borders (60); brightness (128) |
| 28 | Melody | from, to: the part of the bands, 0–255 (40, 200); style: 0 dot, 1 bar, 2 trace (0); glide, 10 + 3v ms (80); size, (128 + 4v)/128 LED (100); trail (120); colours: 0 by note, 1 by tone, 2 by level (0); ignore below, v·16 of 4096 (40); brightness (128) |
| 29 | Groove | shape: 0 rings, 1 spin, 2 stripes, 3 bounce (0); step per beat, v/128 of a period (64); snap to the beat: 0 a steady glide .. 255 a sharp step (200); bass expansion (160); zoom, (8 + v)/32 periods across (100); treble sparks (100); colours: 0 along the pattern, 1 by tone, 2 new each beat (0); brightness (128) |

### Compatibility with the supplied browser lab

`esp_led_lab/web/js/remote.js` uses an earlier provisional control
contract: hello without session, `sync.id`, floating-point clock offsets,
and a `start.session_id`. The receiver recognizes this only for a
`hello` with `client:"esp-led-wasm-lab"`. The browser's mode
numbers are shifted because it has an explicit `Off` item:
browser 0 = Off, browser 1..10 = firmware 0..9,
browser 11..12 = firmware 10..11, browser 13..30 = visualizations 12..29
(sent only to lamps that list them in `visualizations`; others get Off).
Browser pause/stop preserves the
connected control lease and synchronization, allowing a new `start`
without disconnecting.

**16, 32 or 64 frequency bands** are accepted and advertised
(`band_counts`). Each frame states its count; the receiver stores up to 64
bands per queued frame (about 1.7 KB more RAM than the former fixed 32) and
does not interpolate across a change of count. Spectrum modes average or
interpolate the received bands over the columns, so more bands give long
strips and wide matrices finer detail, fewer bands a calmer picture.

The lab transmits its control source, `Speed floor`, `Spectrum expression`,
`Spectrum onset`, the signal window and the origin as reaction tuning (see
`config`). `Audio strength`
(mix) and `Line fast response` are not transmitted: the lamp renders at
100 % strength and Line follows the selected control source only.

The lab re-synchronises the clock every 5 s, also while streaming, and the
receiver rebases queued frames on each `clock`, so long sessions do not
accumulate crystal drift.

## Build and test

PlatformIO: `pio run -e release`. GitHub Actions uploads the generated `firmware.bin` as the `esp8266-remote-audio-firmware` artifact (14-day retention) after a successful branch build.
Host parser tests:

```sh
g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_audio_protocol.cpp -o /tmp/remote_audio_protocol
/tmp/remote_audio_protocol
g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_audio_timeline.cpp -o /tmp/remote_audio_timeline
/tmp/remote_audio_timeline
g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_viz.cpp -o /tmp/remote_viz
/tmp/remote_viz
tests/check_viz_integer.sh
```

Synthetic sender:

```sh
python -m pip install websockets
python tools/remote_audio_sender.py ws://192.168.4.1/audio --hz 40 --seconds 30 --effect 3
```

Use `--effect 0` through `29`: twelve firmware modes and the 18 visualizations;
`--params 0,4,200` sends a visualization's effect parameters (see above) to a
lamp with the `params` capability. `--lead-ms 45`
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

## ESP8266 idle WebSocket stability regression (October 2026)

The browser companion sends status approximately every 2 seconds and
sync/clock approximately every 10 seconds **even without music playing**.
This idle control traffic is sufficient to exercise the transport code.

The remote endpoint now places incoming **JSON control messages** in a
fixed-sized 4-slot ring (3 usable messages). JSON parsing, session changes
and WebSocket replies run in the 20 ms service loop, not inside AsyncTCP
callbacks. Full control queues drop messages instead of blocking the callback.
Binary audio frames stay on a separate, allocation-free fast path so the
browser's normal 240 ms lookahead bursts aren't limited by the smaller
control queue. Client cleanup runs once per second for both `/audio`
and the legacy `/ws` endpoint, instead of 50 times per second.

The WebSocket dependencies are pinned to:
- `ESP32Async/ESPAsyncWebServer@3.12.1`
- `ESP32Async/ESPAsyncTCP@2.0.0` **for ESP8266**, not AsyncTCP for ESP32.

To reproduce the *no-audio* freeze/restart report and monitor the fix:

```sh
python -m pip install websockets
python tools/ws_idle_soak.py ws://192.168.4.1/audio --minutes 20
```

The tool connects with the same browser `hello` client ID, does not send
binary audio frames, keeps synchronizing the clock and querying status,
and checks for an unexpected disconnect or device uptime rollback.
The `status` response includes raw `reset_reason`, `uptime_ms`,
`heap`, `heap_max_block`, `heap_fragmentation`, `queue_drops` and `viz_us`
(the last visualization frame time).
The browser UI need not display these diagnostics; the test tool does.

**Acceptance still requires running the soak on a real ESP8266**, ideally
with no playback and then separately during playback. A successful native
build does not establish that a network-induced reset has been eliminated.
If a reset occurs, record the reason, status telemetry and the serial
exception trace (UART at 115200 for debug firmware, 74880 for the ROM boot
message). For the watchdog or an exception, decode the stack addresses
against the exact ELF of that build.
