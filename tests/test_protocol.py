import unittest
from contextlib import redirect_stdout
from io import StringIO

from app.monitor import simulate
from app.hardware import HARDWARE_ITEMS, hardware_status_text
from app.protocol import DeviceStatus, ProtocolError, parse_line


class ProtocolTests(unittest.TestCase):
    def test_identity_updates_status(self):
        status = DeviceStatus()
        message = parse_line(
            '{"protocol":1,"type":"identity","device_id":"proctorscan-aabb",'
            '"board":"waveshare-esp32-p4-wifi6-dev-kit","firmware":"0.1.0"}'
        )
        status.update(message, now=10.0)
        self.assertEqual(status.device_id, "proctorscan-aabb")
        self.assertEqual(status.connection(now=14.9), "ONLINE")
        self.assertEqual(status.connection(now=15.1), "STALE")

    def test_heartbeat_updates_status(self):
        status = DeviceStatus()
        message = parse_line(
            b'{"protocol":1,"type":"heartbeat","sequence":7,"uptime_ms":14000}'
        )
        status.update(message, now=20.0)
        self.assertEqual(status.heartbeat_sequence, 7)
        self.assertEqual(status.uptime_ms, 14000)

    def test_rejects_unknown_protocol(self):
        with self.assertRaises(ProtocolError):
            parse_line('{"protocol":2,"type":"heartbeat"}')

    def test_rejects_console_noise(self):
        with self.assertRaises(ProtocolError):
            parse_line("I (123) boot: normal ESP-IDF output")

    def test_simulator_exercises_status_path(self):
        output = StringIO()
        with redirect_stdout(output):
            result = simulate(count=2, interval=0)
        self.assertEqual(result, 0)
        self.assertIn("proctorscan-simulator", output.getvalue())
        self.assertIn("Heartbeat: 2", output.getvalue())
        self.assertIn("Status: ONLINE", output.getvalue())

    def test_rejects_removed_accelerometer_messages(self):
        with self.assertRaises(ProtocolError):
            parse_line(
                '{"protocol":1,"type":"accelerometer","source":"hardware",'
                '"x_g":0,"y_g":0,"z_g":1}'
            )

    def test_microsd_identification_status(self):
        status = DeviceStatus()
        status.update(
            parse_line(
                '{"protocol":1,"type":"storage_status","component":"microsd",'
                '"mode":"identification_only","state":"DETECTED",'
                '"capacity_bytes":32000000000}'
            )
        )
        self.assertEqual(status.storage_state, "DETECTED")
        self.assertEqual(status.storage_capacity_bytes, 32000000000)

    def test_rejects_writable_storage_mode(self):
        with self.assertRaises(ProtocolError):
            DeviceStatus().update(
                parse_line(
                    '{"protocol":1,"type":"storage_status","component":"microsd",'
                    '"mode":"read_write","state":"DETECTED","capacity_bytes":1}'
                )
            )

    def test_hardware_readiness_keeps_unverified_peripherals_safe(self):
        peripherals = HARDWARE_ITEMS[2:]
        self.assertTrue(peripherals)
        self.assertTrue(all("DISCONNECTED" in item.state for item in peripherals))
        self.assertIn("No unverified peripheral is approved for powered use", hardware_status_text())


if __name__ == "__main__":
    unittest.main()
