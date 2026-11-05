"""Read the hardware test console and verify EEPROM history across MCLR.
Never clears history or injects fake hardware failures.
"""
import argparse
import binascii
from datetime import datetime
import json
import re
import struct
import time
from project import load_config, ROOT
from serial_monitor import open_serial

p = argparse.ArgumentParser()
p.add_argument('--port')
p.add_argument('--reset', action='store_true')
a = p.parse_args()
c = load_config(profile='station-hw-test', port=a.port)
path = ROOT/'logs'/(datetime.now().strftime('%Y%m%d-%H%M%S')+'-hardware-events.log')
records = []
with path.open('w', encoding='utf-8', buffering=1) as log, open_serial(c) as ser:
    pending = bytearray()
    def emit(message):
        line = datetime.now().isoformat(timespec='milliseconds')+' '+message
        print(line, flush=True)
        log.write(line+'\n')
    def capture(seconds):
        lines = []
        end = time.monotonic()+seconds
        while time.monotonic()<end:
            pending.extend(ser.read(ser.in_waiting or 1))
            while b'\n' in pending:
                line, _, rest = pending.partition(b'\n')
                pending[:] = rest
                text = line.rstrip(b'\r').decode('ascii', errors='replace')
                lines.append(text)
                emit('RX '+text)
                if re.fullmatch(r'LOG [0-9A-Fa-f]{64}', text):
                    raw = bytes.fromhex(text[4:])
                    assert binascii.crc_hqx(raw[:30], 65535)==int.from_bytes(raw[30:], 'little'), 'CRC mismatch'
                    records.append(raw)
                    fields = struct.unpack('<IIIHBBBBIIHHH', raw)
                    emit(f'CHECK CRC=OK seq={fields[0]} event=0x{fields[3]:04X} boot={fields[10]}')
        return lines
    def command(text, wait=1.0):
        emit('TX '+text)
        ser.write(text.encode('ascii')+b'\r')
        return capture(wait)
    emit('SESSION '+c['port'])
    capture(0.3)
    lines = command('info')
    if not any('INFO app=station-hw-test ' in line for line in lines):
        raise SystemExit('Unexpected firmware; no test/reset commands sent')
    command('ADC PROBE')
    command('LOG STATUS')
    command('HEALTH STATUS')
    count_lines = command('LOG COUNT')
    counts = [int(m.group(1)) for line in count_lines if (m := re.fullmatch(r'LOG count=(\d+)', line))]
    assert counts and counts[-1]>0, 'No persisted history available'
    sample_count = min(counts[-1], 4)
    for i in range(sample_count):
        command('LOG READ '+str(i))
    assert len(records)==sample_count, 'Missing history records'
    before = records[:]
    if a.reset:
        emit('RESET MCLR 200ms; BOOT inactive; no EEPROM erase')
        ser.rts = True
        try:
            time.sleep(0.2)
        finally:
            ser.rts = False
        capture(10)
        command('LOG STATUS')
        for i in range(sample_count):
            command('LOG READ '+str(i))
        assert records[sample_count:]==before, 'History changed across reset'
        emit(f'CHECK retention=PASS first {sample_count} records byte-identical after MCLR')
    command('status')
    emit('SESSION_END')
summary = {'log':str(path), 'crc_records_verified':len(records), 'retention_mclr':bool(a.reset),
           'first_records_hex':[r.hex() for r in before]}
path.with_suffix('.json').write_text(json.dumps(summary, indent=2)+'\n')
print('LOG: '+str(path))
