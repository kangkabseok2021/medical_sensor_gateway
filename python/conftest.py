"""Fixtures for fault-injection tests against the live gateway daemon."""

import json
import os
import select
import subprocess
import time

import pytest

GATEWAY_BIN = os.environ.get(
    "GATEWAY_BIN",
    os.path.join(os.path.dirname(__file__), "../build/gateway_daemon"),
)


class GatewayFixture:
    def __init__(self, proc: subprocess.Popen, pipe_r: int):
        self._proc = proc
        self._pipe_r = pipe_r
        self._buf = b""

    def read_event(self, timeout: float = 2.0) -> dict | None:
        """Read one JSON alarm line from the named pipe within *timeout* s."""
        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return None
            ready, _, _ = select.select([self._pipe_r], [], [], remaining)
            if not ready:
                return None
            chunk = os.read(self._pipe_r, 4096)
            if not chunk:
                return None
            self._buf += chunk
            if b"\n" in self._buf:
                line, self._buf = self._buf.split(b"\n", 1)
                return json.loads(line.decode())

    def no_event(self, window: float = 0.4) -> bool:
        """Return True if no alarm event arrives within *window* seconds."""
        return self.read_event(timeout=window) is None

    def stop(self):
        self._proc.terminate()
        try:
            self._proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self._proc.kill()


def start_gateway(pipe_path: str, flow: float, hr: float) -> GatewayFixture:
    """Launch the gateway daemon and return a fixture wrapping it."""
    pipe_r = os.open(pipe_path, os.O_RDONLY | os.O_NONBLOCK)
    proc = subprocess.Popen(
        [
            GATEWAY_BIN,
            "--alarm-pipe", pipe_path,
            "--flow", str(flow),
            "--hr",   str(hr),
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    # Wait for calibration phase to complete (~120 ms at 1 kHz, 100 samples).
    time.sleep(0.25)
    return GatewayFixture(proc, pipe_r)


@pytest.fixture
def make_gateway(tmp_path):
    """Factory fixture: yields a function that starts a gateway with given params."""
    gateways: list[GatewayFixture] = []

    def _make(flow: float = 1.0, hr: float = 75.0) -> GatewayFixture:
        pipe_path = str(tmp_path / f"alarm_{len(gateways)}.fifo")
        os.mkfifo(pipe_path)
        gw = start_gateway(pipe_path, flow, hr)
        gateways.append(gw)
        return gw

    yield _make

    for gw in gateways:
        gw.stop()
