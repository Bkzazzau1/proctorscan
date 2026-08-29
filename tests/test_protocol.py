import unittest

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


if __name__ == "__main__":
    unittest.main()

