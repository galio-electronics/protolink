"""Shared port selection; enumeration never opens or resets a device."""
import json
from pathlib import Path
from serial.tools import list_ports
from project import ROOT

LOCAL_CONFIG = ROOT / 'config/local.json'


def detected_ports():
    return sorted(list_ports.comports())


def describe(port):
    return f'{port.device} - {port.description or "Puerto serie"}'


def save_port(port, path=LOCAL_CONFIG):
    path = Path(path)
    saved = json.loads(path.read_text(encoding='utf-8-sig')) if path.exists() else {}
    saved['port'] = port
    temporary = path.with_suffix('.json.tmp')
    temporary.write_text(json.dumps(saved, indent=2) + '\n', encoding='utf-8')
    temporary.replace(path)


def select_port(current, requested=None):
    while True:
        ports = detected_ports()
        print(f'\nPuerto actual: {current}')
        for index, port in enumerate(ports, 1):
            marker = ' [actual]' if port.device.casefold() == current.casefold() else ''
            print(f'  {index}. {describe(port)}{marker}')
        if not ports:
            print('No se detectaron puertos. Conecta la Station y actualiza la lista.')
        if requested:
            chosen = next((p for p in ports if p.device.casefold() == requested.casefold()), None)
            if chosen is None:
                raise ValueError(f'Puerto no detectado: {requested}. No se cambio la configuracion.')
        else:
            try:
                answer = input('Numero o COM; R = actualizar; Enter = cancelar: ').strip()
            except EOFError:
                answer = ''
            if not answer:
                print('Cancelado: se conserva el puerto actual.')
                return
            if answer.lower() == 'r':
                continue
            chosen = next((p for p in ports if p.device.casefold() == answer.casefold()), None)
            if chosen is None and answer.isdecimal() and 1 <= int(answer) <= len(ports):
                chosen = ports[int(answer)-1]
            if chosen is None:
                print('Seleccion invalida. Elige un puerto de la lista.')
                continue
        if chosen.device not in [p.device for p in detected_ports()]:
            raise ValueError('El puerto se desconecto. Actualiza la lista y vuelve a seleccionar.')
        save_port(chosen.device)
        print(f'Puerto guardado: {describe(chosen)}\nAplica a todas las apps de Protolink.\nArchivo: {LOCAL_CONFIG}')
        return


def require_port(config):
    ports = detected_ports()
    chosen = next((p for p in ports if p.device.casefold() == config['port'].casefold()), None)
    if chosen is None:
        available = '\n'.join('  ' + describe(p) for p in ports) or '  Ninguno'
        raise ValueError(f"Puerto {config['port']} no conectado. Disponibles:\n{available}\n"
                         'Ejecuta la tarea Protolink: Seleccionar puerto COM.')
    print(f"Aplicacion: {config['profile']}\nPuerto: {describe(chosen)}", flush=True)
