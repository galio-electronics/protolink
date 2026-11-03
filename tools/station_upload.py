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
    # Enviar ascendente y sin huecos para pasar por cada limite de borrado.
    # 16 bytes -> 44 caracteres con CR, dentro del buffer de 64 del loader.
    end = (max(memory) + 16) & ~15
    lines, bank = [], None
    for start in range(APP_START, end, 16):
        if start >> 16 != bank:
            bank = start >> 16
            lines.append(record(4, data=bank.to_bytes(2, 'big')))
        lines.append(record(0, start & 0xFFFF,
                            bytes(memory.get(a, 255) for a in range(start, start + 16))))
    lines.append(record(1))
    return lines, len(memory), skipped


def await_ack(ser, timeout=3):
    deadline, ack, stopped = time.monotonic() + timeout, False, False
    while time.monotonic() < deadline:
        value = ser.read(1)
        if not value:
            continue
        if value == b'\x13' and not stopped and not ack:
            stopped = True
        elif value == b'\x06' and stopped and not ack:
            ack = True
        elif value == b'\x11' and stopped:
            if not ack:
                raise RuntimeError('Loader rechazo el registro (XON sin ACK)')
            return
        else:
            raise RuntimeError(f'Respuesta inesperada del loader: {value!r}')
    raise TimeoutError('Sin ACK/XON; carga interrumpida. Reintentar desde el inicio')


def upload(config, lines, monitor_seconds=5, commands=()):
    ser = open_serial(config)
    try:
        time.sleep(0.3)
        ser.reset_input_buffer()
        rx = bytearray()

        def wait(ms):
            deadline = time.monotonic() + ms / 1000
            while time.monotonic() < deadline:
                rx.extend(ser.read(ser.in_waiting or 1))

        crossed_sequence(lambda line, value: setattr(ser, line, value), wait, 100, 200, 500)
        expected = b'name=Protolink-Station-Monitor,version=BOOT:1.2,'
        deadline = time.monotonic() + 3
        while expected not in rx or b'\n' not in rx[rx.find(expected):]:
            if time.monotonic() >= deadline:
                raise TimeoutError(f'No se identifico el bootloader esperado. RX={bytes(rx)!r}')
            wait(20)
        print(bytes(rx).decode('ascii', errors='replace').strip(), flush=True)
        # Margen despues del banner: el firmware espera 1000 ms antes de getc().
        time.sleep(1.2)
        for index, line in enumerate(lines, 1):
            if ser.write(line) != len(line):
                raise RuntimeError('Escritura serie incompleta')
            ser.flush()
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
