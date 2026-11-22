#!/usr/bin/env python3
"""Exercise the real ESP8266 /audio socket without sending any audio frames.

Mirrors the companion's idle hello/sync/clock/status cadence, detects
unexpected socket closure or reboot, and logs free heap/fragmentation.
Usage:
  python -m pip install websockets
  python tools/ws_idle_soak.py ws://192.168.4.1/audio --minutes 20
"""
import argparse
import asyncio
import json
import time

def difference(a, b):
    """Signed subtraction modulo 2^32 ms."""
    return ((int(a) - int(b) + 2**31) % 2**32) - 2**31

async def command(ws, message, expected):
    await ws.send(json.dumps(message))
    reply = json.loads(await asyncio.wait_for(ws.recv(), 4))
    if reply.get("type") != expected:
        raise RuntimeError(f"Expected {expected}: {reply}")
    return reply

async def soak(url, minutes):
    import websockets
    start = time.monotonic()
    last_uptime = None
    initial_heap = None
    samples = 0
    while time.monotonic() - start < minutes * 60:
        try:
            async with websockets.connect(url, max_size=4096, ping_interval=20) as ws:
                hello = await command(ws, {"type": "hello", "version": 1,
                    "band_counts": [32], "client": "esp-led-wasm-lab"}, "hello_ack")
                print("Connected:", hello, flush=True)
                # Browser performance.now() is relative to page navigation.
                epoch = time.monotonic()
                last_sync = -100000.
                while time.monotonic() - start < minutes * 60:
                    now = time.monotonic()
                    local_ms = lambda: (time.monotonic() - epoch) * 1000
                    if now - last_sync >= 10:
                        t0 = local_ms()
                        reply = await command(ws, {"type": "sync", "id": samples + 1,
                            "t0": t0}, "sync_ack")
                        t3 = local_ms()
                        offset = (difference(reply["t1"], t0)
                                + difference(reply["t2"], t3)) / 2
                        rtt = max(0, (t3 - t0) - difference(reply["t2"], reply["t1"]))
                        await command(ws, {"type": "clock",
                             "offset_ms": offset, "rtt_ms": rtt}, "clock_ack")
                        last_sync = now
                    status = await command(ws, {"type": "status"}, "status")
                    samples += 1
                    heap = status.get("heap")
                    if initial_heap is None:
                        initial_heap = heap
                    uptime = status.get("uptime_ms")
                    print("sample", samples, "uptime", uptime,
                          "reset", status.get("reset_reason"),
                          "heap", heap, "delta", (heap - initial_heap) if heap is not None else "?",
                          "largest", status.get("heap_max_block"),
                          "fragmentation", status.get("heap_fragmentation"),
                          "queue_drops", status.get("queue_drops"),
                          "buffer", status.get("buffer_occupancy"), flush=True)
                    if last_uptime is not None and uptime is not None and uptime < last_uptime:
                        raise RuntimeError("Device reboot detected (uptime decreased)")
                    last_uptime = uptime
                    await asyncio.sleep(2)
        except (OSError, ConnectionError, asyncio.TimeoutError,
                websockets.exceptions.WebSocketException) as exc:
            raise RuntimeError(f"Idle socket disconnected or timed out after {samples} statuses: {exc}") from exc
    print("PASS: idle connection survived for", minutes, "minutes")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("url")
    parser.add_argument("--minutes", type=float, default=20)
    args = parser.parse_args()
    if args.minutes <= 0:
        parser.error("--minutes must be positive")
    asyncio.run(soak(args.url, args.minutes))
