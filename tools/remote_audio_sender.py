#!/usr/bin/env python3
"""Synthetic Remote Audio v1 sender (with chroma for devices that list it).
Install: python -m pip install websockets"""
import argparse
import asyncio
import json
import math
import random
import struct
import time

FRAME = struct.Struct("<4sBBBBIII6HBBHBB32H")
assert FRAME.size == 102
EXT_CHROMA = 1  # byte 37: 12 chroma bytes (C..B) follow the bands; only for devices listing "chroma"
# Two seconds each: C major, A minor, F major, G major (pitch classes, C = 0).
CHORDS = [(0, 4, 7), (9, 0, 4), (5, 9, 0), (7, 11, 2)]

def now_ms():
    return int(time.monotonic() * 1000) & 0xffffffff

def diff32(a, b):
    return ((a - b + (1 << 31)) & 0xffffffff) - (1 << 31)

async def exchange(ws, command, expected):
    await ws.send(json.dumps(command))
    reply = json.loads(await ws.recv())
    if reply.get("type") != expected:
        raise RuntimeError(f"Expected {expected}; got {reply}")
    return reply

async def stream(url, hz, seconds, lead_ms, effect, params=None):
    import websockets
    async with websockets.connect(url, max_size=4096) as ws:
        session = random.randrange(1, 2**32)
        hello = await exchange(ws, {"type": "hello", "version": 1, "session": session}, "hello_ack")
        print("device:", hello)
        t0 = now_ms()
        synced = await exchange(ws, {"type": "sync", "session": session, "t0": t0}, "sync_ack")
        t3 = now_ms()
        offset = round((diff32(synced["t1"], t0) + diff32(synced["t2"], t3)) / 2)
        rtt = max(0, diff32(t3, t0) - diff32(synced["t2"], synced["t1"]))
        await exchange(ws, {"type": "clock", "session": session,
                            "offset_ms": offset, "rtt_ms": rtt}, "clock_ack")
        await exchange(ws, {"type": "config", "session": session, "audio_effect": effect}, "config_ack")
        if params is not None:
            # Older firmware has no "params" capability: it keeps the effect's defaults.
            if "params" in hello.get("capabilities", []):
                await exchange(ws, {"type": "params", "session": session, "version": hello.get("params_version", 1),
                                    "effect": effect, "values": params}, "params_ack")
            else:
                print("device has no params capability; effect parameters not sent")
        await exchange(ws, {"type": "start", "session": session}, "start_ack")
        print(f"clock offset: {offset} ms, RTT: {rtt} ms")
        chroma = "chroma" in hello.get("capabilities", [])
        if not chroma:
            print("device has no chroma capability; v1 frames without notes")
        started = time.monotonic()
        seq = 0
        next_send = started
        beats = -1
        try:
            while time.monotonic() - started < seconds:
                t = time.monotonic() - started
                rms = int((0.5 + 0.5 * math.sin(2 * math.pi * 2 * t)) * 4096)
                bands = [int(rms * (0.3 + 0.7 * math.sin(i * 0.3 + t) ** 2)) for i in range(32)]
                # A 120 BPM beat: its phase, a sure tempo and a bass onset on each beat.
                onset = 1 if int(t * 2) != beats else 0
                beats = int(t * 2)
                packet = FRAME.pack(b"ELAF", 1, 1, 1, 32, session, seq,
                                    (now_ms() + lead_ms) & 0xffffffff,
                                    rms, bands[0], bands[15], bands[31], rms, 4096 * onset,
                                    onset, int((t * 2) % 1 * 255), 1200, 230,
                                    EXT_CHROMA if chroma else 0, *bands)
                if chroma:
                    # The root strongest, the third and fifth a little weaker.
                    chord = CHORDS[int(t / 2) % len(CHORDS)]
                    packet += bytes(255 if k == chord[0] else 200 if k in chord else 0 for k in range(12))
                await ws.send(packet)
                seq = (seq + 1) & 0xffffffff
                next_send += 1 / hz
                await asyncio.sleep(max(0, next_send - time.monotonic()))
            status = await exchange(ws, {"type": "status", "session": session}, "status")
            print("status:", status)
        finally:
            await exchange(ws, {"type": "stop", "session": session}, "stop_ack")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("url", help="ws://DEVICE_IP/audio")
    parser.add_argument("--hz", type=float, default=40)
    parser.add_argument("--seconds", type=float, default=30)
    parser.add_argument("--lead-ms", type=int, default=45)
    parser.add_argument("--effect", type=int, default=3, help="Audio mode 0..30 (10/11 experimental, 12..30 visualizations)")
    parser.add_argument("--params", help="Effect parameters for a visualization, comma-separated 0..255 (docs: params)")
    parser.add_argument("--sweep-modes", action="store_true", help="Run all 31 modes sequentially")
    args = parser.parse_args()
    if not (1 <= args.hz <= 100 and args.seconds > 0 and 0 <= args.lead_ms <= 1000
            and 0 <= args.effect <= 30):
        parser.error("invalid hz, seconds, lead-ms or effect")
    params = None
    if args.params is not None:
        try:
            params = [int(v) for v in args.params.split(",") if v.strip()]
        except ValueError:
            parser.error("--params takes comma-separated integers")
        if len(params) > 11 or any(not 0 <= v <= 255 for v in params):
            parser.error("--params takes at most 11 values 0..255")
    for mode in (range(31) if args.sweep_modes else (args.effect,)):
        print(f"Testing mode {mode}")
        asyncio.run(stream(args.url, args.hz, args.seconds, args.lead_ms, mode, params))
