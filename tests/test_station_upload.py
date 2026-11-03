from pathlib import Path
import unittest
from unittest.mock import patch
from station_upload import prepare_hex, record, await_ack


class LoaderTests(unittest.TestCase):
    def parse(self, *lines):
        with patch.object(Path, 'read_text', return_value=b'\n'.join(lines).decode('ascii')):
            return prepare_hex('test.hex')

    def test_application_and_fuses(self):
        lines, size, skipped = self.parse(record(0, 0x600, b'\x01\x02\x03\x04'),
            record(4, data=b'\x00\x30'), record(0, 0, b'\xAA\xFF'), record(1))
        self.assertEqual((size, skipped), (4, 2))
        self.assertTrue(all(len(line) < 64 for line in lines))
        self.assertEqual(lines[0], record(4, data=b'\0\0'))
        self.assertEqual(lines[1], record(0, 0x600, b'\x01\x02\x03\x04' + b'\xff'*12))

    def test_reject_bootloader(self):
        with self.assertRaises(ValueError):
            self.parse(record(0, 0, b'1234'), record(1))

    def test_reject_checksum(self):
        with self.assertRaises(ValueError):
            self.parse(b':040600000102030400', record(1))

    def test_reject_missing_eof_and_overlap(self):
        with self.assertRaises(ValueError):
            self.parse(record(0, 0x600, b'1234'))
        with self.assertRaises(ValueError):
            self.parse(record(0, 0x600, b'1234'), record(0, 0x602, b'12'), record(1))

    def test_bank_boundary(self):
        lines, _, _ = self.parse(record(0, 0x600, b'1234'),
            record(4, data=b'\0\1'), record(0, 0, b'1234'), record(1))
        self.assertIn(record(4, data=b'\0\1'), lines)

    def test_ack_flow(self):
        class Port:
            def __init__(self, data): self.data = iter(data)
            def read(self, n): return next(self.data, b'')
        await_ack(Port([b'\x13', b'\x06', b'\x11']))
        with self.assertRaises(RuntimeError):
            await_ack(Port([b'\x13', b'\x11']))
        with self.assertRaises(TimeoutError):
            await_ack(Port([]), timeout=0.001)
        with self.assertRaises(RuntimeError):
            await_ack(Port([b'X']))


if __name__ == '__main__':
    unittest.main()
