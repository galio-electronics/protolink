"""Monitor compartido por carga y consola; un solo handle serie."""
from datetime import datetime
from pathlib import Path
import sys
import time


def monitor(ser, log_dir, profile, seconds=0, commands=()):
    log_dir = Path(log_dir)
    log_dir.mkdir(parents=True, exist_ok=True)
    path = log_dir / (datetime.now().strftime('%Y%m%d-%H%M%S-%f') + '-' + profile + '.log')
    print(f'MONITOR {ser.port} | log: {path}\nComandos: info, status, inputs, errors. Ctrl+C para cerrar.', flush=True)
    start, pending, typed = time.monotonic(), bytearray(), ''
    sent = False
    app_ready = False
    keyboard = None
    if sys.stdin.isatty() and sys.platform == 'win32':
        import msvcrt
        keyboard = msvcrt
    with path.open('w', encoding='utf-8', buffering=1) as log:
        def emit(message):
            line = datetime.now().isoformat(timespec='milliseconds') + ' ' + message
            print(line, flush=True)
            log.write(line + '\n')
        emit(f'SESSION profile={profile} port={ser.port} baud={ser.baudrate}')
        try:
            while not seconds or time.monotonic()-start < seconds:
                chunk = ser.read(ser.in_waiting or 1)
                if chunk:
                    pending.extend(chunk)
                    while b'\n' in pending:
                        line, _, rest = pending.partition(b'\n')
                        pending = bytearray(rest)
                        text = line.rstrip(b'\r').decode('ascii', errors='replace')
                        if text.startswith(('INFO app=', 'STAT t=')):
                            app_ready = True
                        emit('RX ' + text)
                    if len(pending) > 4096:
                        emit('RX_PARTIAL ' + bytes(pending).decode('ascii', errors='replace'))
                        pending.clear()
                if commands and not sent and app_ready and time.monotonic()-start >= 2:
                    for command in commands:
                        ser.write(command.encode('ascii') + b'\r')
                        emit('TX ' + command)
                    sent = True
                if keyboard and keyboard.kbhit():
                    char = keyboard.getwch()
                    if char == '\x03':
                        break
                    if char in ('\x00', '\xe0'):
                        keyboard.getwch()
                    elif char in ('\r', '\n'):
                        if typed:
                            print(flush=True)
                            if app_ready:
                                ser.write(typed.encode('ascii') + b'\r')
                                emit('TX ' + typed)
                            else:
                                emit('NO_TX esperando identificacion/estado de aplicacion')
                            typed = ''
                    elif char == '\b':
                        if typed:
                            print('\b \b', end='', flush=True)
                        typed = typed[:-1]
                    elif char.isascii() and char.isprintable() and len(typed) < 23:
                        typed += char
                        print(char, end='', flush=True)
        except KeyboardInterrupt:
            pass
        finally:
            if pending:
                emit('RX_PARTIAL ' + bytes(pending).decode('ascii', errors='replace'))
            emit('SESSION_END')
    return path


def open_serial(config):
    import serial
    ser = serial.Serial(port=None, baudrate=config['baud'], timeout=0.02,
                        write_timeout=3, rtscts=False, dsrdtr=False, xonxoff=False)
    ser.dtr = ser.rts = False
    ser.port = config['port']
    ser.open()
    return ser
