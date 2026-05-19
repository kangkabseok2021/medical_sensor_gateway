"""Fault-injection tests: start gateway daemon as subprocess, inject sensor
values via CLI flags, verify alarm events on the named pipe."""


def test_overdose_triggers_error_halt(make_gateway):
    # [REQ-004] Flow > 15.0 L/min must immediately latch ERROR_HALT.
    gw = make_gateway(flow=50.0, hr=75.0)
    event = gw.read_event(timeout=2.0)
    assert event is not None, "expected an alarm event within 2 s"
    assert event["event"] == "ERROR_HALT"
    assert "overdose" in event["trigger"]


def test_occlusion_triggers_alarm(make_gateway):
    # [REQ-005] Flow < 0.1 L/min for 5 consecutive readings → ALARM.
    gw = make_gateway(flow=0.0, hr=75.0)
    event = gw.read_event(timeout=2.0)
    assert event is not None, "expected an alarm event within 2 s"
    assert event["event"] in ("ALARM", "ERROR_HALT")


def test_three_alarms_latch_error_halt(make_gateway):
    # [REQ-006] Three consecutive ALARM cycles without operator ack → ERROR_HALT.
    # Flow=0.0 triggers ALARM after 5 readings, then ERROR_HALT after kMaxAlarms.
    gw = make_gateway(flow=0.0, hr=75.0)
    # Collect events until ERROR_HALT or timeout
    events = []
    for _ in range(10):
        ev = gw.read_event(timeout=1.0)
        if ev is None:
            break
        events.append(ev)
        if ev["event"] == "ERROR_HALT":
            break
    states = [e["event"] for e in events]
    assert "ERROR_HALT" in states, f"expected ERROR_HALT in {states}"


def test_tachycardia_triggers_error_halt(make_gateway):
    # [REQ-007] HR > 200 bpm for 3 s (3000 samples at 1 kHz) → ERROR_HALT.
    gw = make_gateway(flow=1.0, hr=300.0)
    event = gw.read_event(timeout=5.0)
    assert event is not None, "expected ERROR_HALT within 5 s"
    assert event["event"] == "ERROR_HALT"
    assert "tachycardia" in event["trigger"]


def test_normal_range_no_alarm(make_gateway):
    # [REQ-008] Valid readings must not produce any alarm events.
    gw = make_gateway(flow=1.0, hr=75.0)
    assert gw.no_event(window=0.4), "unexpected alarm during normal operation"


def test_bradycardia_triggers_alarm(make_gateway):
    # [REQ-009] HR < 30 bpm for 2 s (2000 samples at 1 kHz) → ALARM.
    gw = make_gateway(flow=1.0, hr=20.0)
    event = gw.read_event(timeout=4.0)
    assert event is not None, "expected an alarm event within 4 s"
    assert event["event"] in ("ALARM", "ERROR_HALT")
