"""Execute the actual FAT/RTC C on a host, adapting only CCS integer spelling.

Usage: python tests/rtc_sd/run_host_tests.py --cc path/to/tcc.exe
Mocks are test-only. The firmware always uses real SPI/I2C ports.
"""
import argparse
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--cc', default='gcc')
parser.add_argument('--sd-root', type=Path, default=ROOT.parent / 'sd-card')
args = parser.parse_args()
out = ROOT / 'build' / 'rtc-sd-host'
out.mkdir(parents=True, exist_ok=True)

def translate(text):
    for bits in (8, 16, 32):
        text = re.sub(rf'unsigned int{bits}\b', f'uint{bits}_t', text)
    return re.sub(r'\bint1\b', 'bool', text)

for name in ('fat32.c', 'fat32.h', 'sd_card.c', 'sd_card.h'):
    (out/name).write_text(translate((args.sd_root/name).read_text()), encoding='utf-8')
for name in ('mcp79402.c', 'mcp79402.h', 'station_logger.c', 'station_logger.h'):
    code = translate((ROOT/'firmware/common'/name).read_text())
    if name == 'station_logger.c':
        # CCS callback printf and its integer format are host ABI differences.
        code = code.replace('printf(logger_putc,', 'callback_printf(logger_putc,')
        code = code.replace('%04Lu', '%04u')
    (out/name).write_text(code, encoding='utf-8')
for suite in ('fat_rtc_test', 'sd_test'):
    source = ROOT/'tests/rtc_sd'/f'{suite}.c'
    exe = out/f'{suite}.exe'
    subprocess.run([args.cc, '-I'+str(out), str(source), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
