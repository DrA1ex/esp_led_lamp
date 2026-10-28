#!/usr/bin/env python3
"""Synthetic Remote Audio Protocol v1 sender. pip install websockets"""
import argparse
import asyncio
import json
import math
import random
import struct
import time

FRAME = struct.Struct("<4sBBBBIII6HBBHBB32H")
assert FRAME.size == 102

async def stream(url, hz, seconds):
    import websockets
    async with websockets.connect(url) as ws:
        session = random.randrange(1, 2**32)
        await ws.send(json.dumps({"type": "hello", "version": 1, "session": session}))
        print(await ws.recv())
        await ws.send(json.dumps({"type": "start", "session": session}))
        print(await ws.recv())
        start = time.monotonic()
        seq = 0
        while time.monotonic() - start < seconds:
            t = time.monotonic() - start
            rms = int((0.5 + 0.5 * math.sin(2 * math.pi * 2 * t)) * 4096)
            bands = [int(rms * (0.3 + 0.7 * math.sin(i * 0.3 + t) ** 2)) for i in range(32)]
            packet = FRAME.pack(b"ELAF", 1, 1, 1, 32, session, seq,
                                int(time.monotonic() * 1000) & 0xffffffff,
                                rms, bands[0], bands[15], bands[31], rms, 0,
                                0, 0, 0, 0, 0, *bands)
            await ws.send(packet)
            seq = (seq + 1) & 0xffffffff
            await asyncio.sleep(1 / hz)
        await ws.send(json.dumps({"type": "stop", "session": session}))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("url", help="ws://DEVICE_IP/audio")
    parser.add_argument("--hz", type=float, default=40)
    parser.add_argument("--seconds", type=float, default=30)
    args = parser.parse_args()
    if not 1 <= args.hz <= 100 or args.seconds <= 0:
        parser.error("hz must be 1..100 and seconds must be positive")
    asyncio.run(stream(args.url, args.hz, args.seconds))
