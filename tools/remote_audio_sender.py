#!/usr/bin/env python3
"""Synthetic Remote Audio v1 sender. Install: python -m pip install websockets"""
import argparse
import asyncio
import json
import math
import random
import struct
import time

FRAME = struct.Struct("<4sBBBBIII6HBBHBB32H")
assert FRAME.size == 102

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

async def stream(url, hz, seconds, lead_ms, effect):
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
        await exchange(ws, {"type": "start", "session": session}, "start_ack")
        print(f"clock offset: {offset} ms, RTT: {rtt} ms")
        started = time.monotonic()
        seq = 0
        next_send = started
        try:
            while time.monotonic() - started < seconds:
                t = time.monotonic() - started
                rms = int((0.5 + 0.5 * math.sin(2 * math.pi * 2 * t)) * 4096)
                bands = [int(rms * (0.3 + 0.7 * math.sin(i * 0.3 + t) ** 2)) for i in range(32)]
                packet = FRAME.pack(b"ELAF", 1, 1, 1, 32, session, seq,
                                    (now_ms() + lead_ms) & 0xffffffff,
                                    rms, bands[0], bands[15], bands[31], rms, 0,
                                    0, 0, 0, 0, 0, *bands)
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
    parser.add_argument("--effect", type=int, default=3, help="Original audio mode index 0..9")
    parser.add_argument("--sweep-modes", action="store_true", help="Run all ten modes sequentially")
    args = parser.parse_args()
    if not (1 <= args.hz <= 100 and args.seconds > 0 and 0 <= args.lead_ms <= 1000
            and 0 <= args.effect <= 9):
        parser.error("invalid hz, seconds, lead-ms or effect")
    for mode in (range(10) if args.sweep_modes else (args.effect,)):
        print(f"Testing mode {mode}")
        asyncio.run(stream(args.url, args.hz, args.seconds, args.lead_ms, mode))
