"""Compile the real event engine, no translated/reimplemented storage algorithm."""
from pathlib import Path
import subprocess
import argparse
import re
root = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser()
p.add_argument('--cc', default=str(root/'build/host-tools/tcc/tcc.exe'))
args = p.parse_args()
out = root/'build/events-host'
out.mkdir(parents=True, exist_ok=True)
for name in ('mcp79402.h', 'mcp79402.c', 'event_rtc.c'):
    code = (root/'firmware/common'/name).read_text()
    for bits in (8, 16, 32):
        code = re.sub(rf'unsigned int{bits}\b', f'uint{bits}_t', code)
    code = re.sub(r'\bint1\b', 'bool', code)
    (out/name).write_text(code)
for suite in ('event_test', 'event_rtc_test', 'disabled_test'):
    exe = out/(suite+'.exe')
    subprocess.run([args.cc, '-I'+str(out), '-I'+str(root/'firmware/common'), str(Path(__file__).with_name(suite+'.c')), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
