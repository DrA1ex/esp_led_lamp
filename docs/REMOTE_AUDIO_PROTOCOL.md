# Remote Audio Protocol v1 — implementation status

The protocol parser and synthetic sender are in the branch. The firmware receiver, WebSocket endpoint, clock synchronization, runtime controls, audio providers and rendering integration are **not yet implemented**. Do not flash this branch expecting remote audio to work.

## Binary frame

102 bytes; all integers little-endian. Bytes 0–3: ELAF magic; 4: version 1; 5: type 1; 6: flags, bit 0 live; 7: band count 32; 8: uint32 session; 12: uint32 sequence; 16: uint32 monotonic presentation timestamp (ms); 20/22/24/26/28/30: uint16 RMS/bass/mid/high/peak/onset; 32: onset mask; 33: beat phase; 34: uint16 BPM x10; 36: beat confidence; 37: reserved zero; 38–101: 32 uint16 frequency bands ~40 Hz to 8 kHz. Amplitudes 0..4096 inclusive. The parser rejects invalid magic, version, length, session zero, reserved bits and amplitudes out of bounds.

## Planned control protocol

WebSocket endpoint: ws://DEVICE_IP/audio (not yet added). The RFC specifies text JSON hello/hello_ack, sync/sync_ack, clock, config, start/stop and status. Remote temporary overrides must not persist or modify saved presets.

## Testing

Native parser test: g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_audio_protocol.cpp -o /tmp/remote_audio_test && /tmp/remote_audio_test

Synthetic sender (usable only after implementing receiver): pip install websockets; python tools/remote_audio_sender.py ws://DEVICE_IP/audio --hz 40 --seconds 30

PlatformIO build, receiver behavior, synchronization, stream fallback, physical LED visual verification and latency are not yet verified.
