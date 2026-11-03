#!/usr/bin/env python3
"""Diagnostico de entrada al bootloader: no envia datos ni programa flash."""
import argparse
import time


def crossed_sequence(set_line, wait, pre_ms, reset_ms, hold_ms):
    # CP210x activo bajo. T2: base=DTR, emisor=RTS.
    # T1: base=RTS, emisor=DTR. Ambos iguales: transistores apagados.
    set_line('dtr', False)
    set_line('rts', False)
    wait(pre_ms)
    set_line('rts', True)  # DTR fisico alto, RTS bajo: MCLR bajo.
    wait(reset_ms)
    # Sin espera entre comandos: ambos bajos brevemente, luego BOOT bajo.
    set_line('dtr', True)
    set_line('rts', False)  # DTR bajo, RTS alto: RA4 bajo, MCLR liberado.
    wait(hold_ms)
    set_line('dtr', False)


def sequence(set_line, wait, boot_line, boot_value, reset_value,
             pre_ms, reset_ms, hold_ms):
    reset_line = "rts" if boot_line == "dtr" else "dtr"
    set_line(reset_line, not reset_value)
    set_line(boot_line, boot_value)
    wait(pre_ms)
    set_line(reset_line, reset_value)
    wait(reset_ms)
    set_line(reset_line, not reset_value)
    wait(hold_ms)
    set_line(boot_line, not boot_value)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--port')
    ap.add_argument('--list', action='store_true', help='Enumerar, sin abrir puertos')
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('--circuit', choices=['independent', 'crossed'], default='independent')
    ap.add_argument('--boot-line', choices=['dtr', 'rts'])
    ap.add_argument('--boot-assert', type=int, choices=[0, 1], default=1,
                    help='Valor API que lleva RA4 a bajo; verificar circuito')
    ap.add_argument('--reset-assert', type=int, choices=[0, 1], default=1,
                    help='Valor API que lleva MCLR a bajo; verificar circuito')
    ap.add_argument('--pre-ms', type=int, default=100)
    ap.add_argument('--reset-ms', type=int, default=200)
    ap.add_argument('--hold-ms', type=int, default=500)
    ap.add_argument('--listen-ms', type=int, default=2500)
    ap.add_argument('--baud', type=int, default=115200)
    args = ap.parse_args()
    if args.list:
        from serial.tools import list_ports
        for port in list_ports.comports():
            print(port.device, port.description, port.hwid)
        return 0
    if (args.circuit == 'independent' and not args.boot_line) or (not args.port and not args.dry_run):
        ap.error('--boot-line y --port son obligatorios (salvo --dry-run)')
    if min(args.pre_ms, args.reset_ms, args.hold_ms, args.listen_ms) < 0:
        ap.error('Los tiempos no pueden ser negativos')
    if args.reset_ms == 0:
        ap.error('--reset-ms debe ser mayor que cero')

    start = time.monotonic()
    received = bytearray()
    ser = None
    reset_line = 'rts' if args.boot_line == 'dtr' else 'dtr'
    idle = {args.boot_line: not bool(args.boot_assert),
            reset_line: not bool(args.reset_assert)}
    if args.circuit == 'crossed':
        idle = {'dtr': False, 'rts': False}

    def log(message):
        print(f'{(time.monotonic()-start)*1000:9.1f} ms  {message}', flush=True)

    def set_line(line, value):
        log(f'{line.upper()} API={int(value)}')
        if ser is not None:
            setattr(ser, line, value)

    def wait(ms):
        if args.dry_run:
            log(f'ESPERAR {ms} ms')
            return
        deadline = time.monotonic() + ms / 1000
        while time.monotonic() < deadline:
            data = ser.read(ser.in_waiting or 1)
            if data:
                received.extend(data)
                log(f'RX {data!r}')

    try:
        if not args.dry_run:
            import serial
            ser = serial.Serial(port=None, baudrate=args.baud, timeout=0.01,
                                rtscts=False, dsrdtr=False, xonxoff=False)
            # Preconfigurar antes de abrir reduce transitorios; el driver puede
            # aun cambiar DTR/RTS. Los valores API no garantizan niveles MCU.
            ser.dtr = idle['dtr']
            ser.rts = idle['rts']
            ser.port = args.port
            ser.open()
            log(f'ABIERTO {args.port}, {args.baud} 8N1')
            wait(300)
            # Separar datos anteriores de la prueba. No purgar despues del reset.
            received.clear()
            ser.reset_input_buffer()
        if args.circuit == 'crossed':
            crossed_sequence(set_line, wait, args.pre_ms, args.reset_ms, args.hold_ms)
        else:
            sequence(set_line, wait, args.boot_line, bool(args.boot_assert),
                     bool(args.reset_assert), args.pre_ms, args.reset_ms, args.hold_ms)
        wait(args.listen_ms)
        if args.dry_run:
            return 0
        expected = b'name=Protolink-Station-Monitor,version=BOOT:'
        if expected in received and b'\n' in received[received.find(expected):]:
            log('IDENTIFICACION STATION/MONITOR RECIBIDA; no se envio firmware.')
            log('La recepcion de HEX empieza 1000 ms despues del mensaje completo.')
            return 0
        log('SIN IDENTIFICACION ESPERADA: no confirma entrada al bootloader.')
        return 1
    finally:
        if ser is not None and ser.is_open:
            try:
                for line in ('dtr', 'rts'):
                    set_line(line, idle[line])
            finally:
                ser.close()
                log('CERRADO: el driver puede cambiar las lineas al cerrar/reabrir.')


if __name__ == '__main__':
    raise SystemExit(main())
