"""Console probe for the Station RTC/SD demo; no automatic reset or erase."""
import argparse
from datetime import datetime, timezone, timedelta
from pathlib import Path
import time
from project import load_config, ROOT
from serial_monitor import open_serial

ap = argparse.ArgumentParser()
ap.add_argument('--port', default=None)
ap.add_argument('--command', action='append', default=[])
ap.add_argument('--wait', type=float, default=1.5)
ap.add_argument('--observe', type=float, default=2)
ap.add_argument('--reset', action='store_true', help='Stop logging then pulse MCLR using the confirmed crossed circuit')
a = ap.parse_args()
config = load_config(profile='station-rtc-sd', port=a.port)
path = ROOT/'logs'/(datetime.now().strftime('%Y%m%d-%H%M%S')+'-rtc-sd-probe.log')
with path.open('w', encoding='utf-8', buffering=1) as log:
    def emit(text):
        line = datetime.now().isoformat(timespec='milliseconds')+' '+text
        print(line, flush=True); log.write(line+'\n')
    with open_serial(config) as ser:
        pending=bytearray()
        def capture(seconds):
            result=[]
            until=time.monotonic()+seconds
            while time.monotonic()<until:
                pending.extend(ser.read(ser.in_waiting or 1))
                while b'\n' in pending:
                    line, _, rest=pending.partition(b'\n'); pending[:]=rest
                    text=line.rstrip(b'\r').decode('ascii', errors='replace')
                    result.append(text); emit('RX '+text)
            return result
        emit('SESSION port='+ser.port)
        capture(1)
        ser.write(b'info\r'); emit('TX info')
        lines=capture(2)
        if not any('INFO app=station-rtc-sd ' in line for line in lines):
            raise SystemExit('No se identifico el demo; no se enviaron comandos de prueba')
        if a.reset:
            ser.write(b'log off\r'); emit('TX log off')
            lines=capture(2)
            if not any('LOG off pending=0' in line for line in lines):
                raise SystemExit('No se confirmo cola vacia; no se reinicio')
            emit('RESET MCLR 200 ms, BOOT inactive')
            ser.rts=True
            try:
                time.sleep(0.2)
            finally:
                ser.rts=False
            lines=capture(3)
            if not any('INFO app=station-rtc-sd ' in line for line in lines):
                raise SystemExit('No se confirmo arranque despues de RESET')
        for command in a.command:
            if command=='@set-clock':
                command='rtc '+datetime.now(timezone(timedelta(hours=-6))).strftime('%Y%m%d%H%M%S')
            if len(command)>23 or '\r' in command or '\n' in command:
                raise ValueError('Comando fuera del limite de consola')
            ser.write(command.encode('ascii')+b'\r'); emit('TX '+command)
            capture(a.wait)
        capture(a.observe)
        emit('SESSION_END')
print('LOG: '+str(path))
