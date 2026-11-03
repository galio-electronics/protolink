#!/usr/bin/env python3
"""Compilar y cargar Station con CCS/MPLAB y el bootloader existente."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import time
from autoboot_probe import crossed_sequence
from project import ROOT, DEFAULT_CONFIG, load_config, compile_project, dependency_status
from serial_monitor import monitor, open_serial

APP_START, FLASH_END = 0x600, 0x20000
FLASH_ROW = 128  # PIC18F67K40 / CCS 5.081
MAX_RECORD_DATA = 26  # 64 caracteres incluyendo CR; buffer del loader = 64


def record(kind, address=0, data=b''):
    body = bytes([len(data), address >> 8, address & 255, kind]) + data
    return b':' + (body + bytes([-sum(body) & 255])).hex().upper().encode() + b'\r'


def prepare_hex(path):
    """Validar todo antes de tocar el puerto; excluir fuses, nunca el loader."""
    memory, upper, eof, skipped = {}, 0, False, 0
    for number, line in enumerate(Path(path).read_text(encoding='utf-8-sig').splitlines(), 1):
        line = line.strip()
        if not line or line.startswith(';'):  # comentarios generados por CCS
            continue
        if eof or not line.startswith(':'):
            raise ValueError(f'HEX linea {number}: contenido inesperado')
        data = bytes.fromhex(line[1:])
        if len(data) < 5 or len(data) != data[0] + 5 or sum(data) & 255:
            raise ValueError(f'HEX linea {number}: longitud/checksum invalido')
        count, address, kind = data[0], int.from_bytes(data[1:3], 'big'), data[3]
        payload = data[4:-1]
        if kind == 4 and count == 2 and address == 0:
            upper = int.from_bytes(payload, 'big') << 16
        elif kind == 1 and count == 0 and address == 0:
            eof = True
        elif kind == 0 and count and address + count <= 0x10000:
            start = upper + address
            if 0x300000 <= start and start + count <= 0x30000E:
                skipped += count  # fuses instalados por ICSP, no se envian
                continue
            if start < APP_START or start + count > FLASH_END:
                raise ValueError(f'HEX fuera de aplicacion: 0x{start:06X}')
            for offset, value in enumerate(payload):
                loc = start + offset
                if loc in memory:
                    raise ValueError(f'HEX solapado: 0x{loc:06X}')
                memory[loc] = value
        else:
            raise ValueError(f'HEX linea {number}: tipo/formato no admitido ({kind})')
    if not eof or any(a not in memory for a in range(APP_START, APP_START + 4)):
        raise ValueError('Falta EOF o vector de reset de aplicacion en 0x0600')
    # Ascendente, sin huecos y sin cruzar filas flash de 128 bytes.
    # 26 bytes -> 64 caracteres con CR; data[32] del loader tambien alcanza.
    end = (max(memory) + 16) & ~15
    lines, bank = [], None
    start = APP_START
    while start < end:
        if start >> 16 != bank:
            bank = start >> 16
            lines.append(record(4, data=bank.to_bytes(2, 'big')))
        count = min(MAX_RECORD_DATA, FLASH_ROW - start % FLASH_ROW, end - start)
        lines.append(record(0, start & 0xFFFF,
                            bytes(memory.get(a, 255) for a in range(start, start + count))))
        start += count
    lines.append(record(1))
    return lines, len(memory), skipped


def await_ack(ser, timeout=3):
    deadline = time.monotonic() + timeout
    expected = b'\x13\x06\x11'
    received = bytearray()
    while time.monotonic() < deadline:
        chunk = ser.read(len(expected) - len(received))
        if not chunk:
            continue
        received.extend(chunk)
        if not expected.startswith(received):
            raise RuntimeError(f'Registro rechazado o respuesta inesperada: {bytes(received)!r}')
        if len(received) == len(expected):
            return
    raise TimeoutError('Sin ACK/XON; carga interrumpida. Reintentar desde el inicio')


def upload(config, lines, monitor_seconds=5, commands=()):
    upload_started = time.perf_counter()
    ser = open_serial(config)
    try:
        time.sleep(0.05)
        ser.reset_input_buffer()
        rx = bytearray()
        expected = b'name=Protolink-Station-Monitor,version=BOOT:1.2,'
        banner_completed_at = None

        def wait(ms):
            nonlocal banner_completed_at
            deadline = time.monotonic() + ms / 1000
            while time.monotonic() < deadline:
                rx.extend(ser.read(ser.in_waiting or 1))
                if banner_completed_at is None and expected in rx and b'\n' in rx[rx.find(expected):]:
                    banner_completed_at = time.perf_counter()

        crossed_sequence(lambda line, value: setattr(ser, line, value), wait, 50, 200, 200)
        deadline = time.monotonic() + 3
        while expected not in rx or b'\n' not in rx[rx.find(expected):]:
            if time.monotonic() >= deadline:
                raise TimeoutError(f'No se identifico el bootloader esperado. RX={bytes(rx)!r}')
            wait(20)
        print(bytes(rx).decode('ascii', errors='replace').strip(), flush=True)
        # El loader espera 1 s tras el banner; descontar lo ya esperado con BOOT.
        if banner_completed_at is None:
            banner_completed_at = time.perf_counter()
        time.sleep(max(0, 1.05 - (time.perf_counter()-banner_completed_at)))
        transfer_started = time.perf_counter()
        for index, line in enumerate(lines, 1):
            if ser.write(line) != len(line):
                raise RuntimeError('Escritura serie incompleta')
            # El ACK demuestra recepcion; flush() por registro era redundante.
            await_ack(ser)
            if index % 32 == 0 or index == len(lines):
                print(f'Carga: {index}/{len(lines)} registros', flush=True)
        # El loader envia ACK/XON adicional tras EOF, espera 1s y reinicia.
        final = bytearray()
        deadline = time.monotonic() + 2
        while len(final) < 2 and time.monotonic() < deadline:
            final.extend(ser.read(2-len(final)))
        if final != b'\x06\x11':
            raise RuntimeError('Falta confirmacion final de EOF')
        elapsed = time.perf_counter() - transfer_started
        print(f'TRANSFER {len(lines)} registros en {elapsed:.3f} s; entrada+carga {time.perf_counter()-upload_started:.3f} s', flush=True)
        print('Carga aceptada. Capturando reinicio sin cerrar el puerto; no hay lectura de flash.')
        monitor(ser, ROOT/'logs', config['profile'], monitor_seconds, commands)
    finally:
        if ser.is_open:
            try:
                ser.dtr = False
                ser.rts = False
            finally:
                ser.close()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('action', choices=['build', 'check', 'upload', 'build-upload',
                                      'build-upload-monitor', 'monitor', 'select', 'profiles', 'dependencies', 'tests'])
    ap.add_argument('--config', type=Path, default=DEFAULT_CONFIG)
    ap.add_argument('--profile')
    ap.add_argument('--port')
    ap.add_argument('--seconds', type=float, default=None, help='0=monitor continuo')
    ap.add_argument('--command', action='append', default=[], choices=['info','status','inputs','errors'])
    args = ap.parse_args()
    if args.seconds is not None and args.seconds < 0:
        ap.error('--seconds debe ser >= 0')
    config = load_config(args.config, args.profile, args.port)
    if args.action == 'tests':
        import unittest
        suite = unittest.defaultTestLoader.discover(str(ROOT/'tests'))
        return int(not unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful())
    if args.action == 'profiles':
        for name, entry in config['profiles'].items():
            print(('* ' if name == config['active_profile'] else '  ') + name + ': ' + entry['source'])
        return 0
    if args.action == 'select':
        if not args.profile:
            ap.error('select requiere --profile')
        saved = json.loads(args.config.read_text(encoding='utf-8-sig'))
        saved['active_profile'] = args.profile
        args.config.write_text(json.dumps(saved, indent=2)+'\n', encoding='utf-8')
        print(f"Perfil activo: {args.profile}\nFuente: {config['source']}")
        return 0
    if args.action == 'dependencies':
        states = dependency_status(config)
        for name, issues in states.items():
            print(name + ': ' + ('; '.join(issues) if issues else 'OK (huellas coinciden)'))
        return int(any(states.values()))
    if args.action == 'monitor':
        ser = open_serial(config)
        try:
            monitor(ser, ROOT/'logs', config['profile'], args.seconds or 0, args.command)
        finally:
            ser.close()
        return 0
    if args.action.startswith('build'):
        path = compile_project(config)
    else:
        path = config['hex_path']
    lines, size, skipped = prepare_hex(path)
    print(f'HEX validado: {size} bytes de aplicacion; {skipped} bytes de fuses excluidos.', flush=True)
    original_records = sum(1 for line in path.read_text().splitlines() if line.startswith(':'))
    print(f'HEX original: {original_records} lineas; carga: {len(lines)} registros, {sum(map(len,lines))} bytes serie.', flush=True)
    print(f'Archivo: {path}', flush=True)
    if args.action in ('upload', 'build-upload', 'build-upload-monitor'):
        seconds = args.seconds if args.seconds is not None else (0 if args.action.endswith('monitor') else 5)
        upload(config, lines, seconds, args.command)
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print('Cancelado. Si se interrumpio la escritura, repetir la carga completa.', file=sys.stderr)
        raise SystemExit(130)
    except Exception as error:
        print(f'ERROR: {error}', file=sys.stderr)
        raise SystemExit(1)
