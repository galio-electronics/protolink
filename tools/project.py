"""Perfiles y compilacion CCS reproducible desde el repositorio."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import dependencies

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = None


def load_config(path=DEFAULT_CONFIG, profile=None, port=None):
    if path is None:
        if profile:
            matches = [p for folder in ('apps', 'examples')
                       for p in (ROOT/'firmware'/folder).glob('*/config/project.json')
                       if p.parent.parent.name == profile]
            if len(matches) != 1:
                raise ValueError(f'App no encontrada: {profile}; usar --config')
            path = matches[0]
        else:
            path = Path.cwd()/'config/project.json'
    path = Path(path).resolve()
    if not path.is_file():
        raise ValueError('Abrir el workspace de una app o indicar --config / --profile')
    app_root = path.parent.parent
    config = json.loads(path.read_text(encoding='utf-8-sig'))
    local = ROOT / 'config/local.json'
    if local.exists():
        overrides = json.loads(local.read_text(encoding='utf-8-sig'))
        for key in ('compiler', 'ccs_root', 'port'):
            if key in overrides:
                config[key] = overrides[key]
    local = path.parent/'local.json'
    if local.exists():
        overrides = json.loads(local.read_text(encoding='utf-8-sig'))
        for key in ('compiler', 'ccs_root', 'port'):
            if key in overrides:
                config[key] = overrides[key]
    chosen = profile or config['active_profile']
    if chosen not in config['profiles'] or not re.fullmatch(r'[a-zA-Z0-9_-]+', chosen):
        raise ValueError(f'Perfil no valido: {chosen}')
    source = (app_root / config['profiles'][chosen]['source']).resolve()
    if not source.is_file():
        raise ValueError(f'Fuente inexistente o fuera del repositorio: {source}')
    config.update(profile=chosen, source=source, output_dir=app_root/'build'/chosen, app_root=app_root)
    config['hex_path'] = config['output_dir'] / (source.stem + '.hex')
    config['config_path'] = path
    config['library_root'] = app_root/'galio'
    if port:
        config['port'] = port
    return config


def dependency_status(config, all_libraries=False):
    return dependencies.status(config['app_root'], config['profile'], all_libraries)


def compile_project(config):
    dependencies.ensure(config['app_root'], config['profile'])
    source, directory = config['source'], config['output_dir']
    directory.mkdir(parents=True, exist_ok=True)
    output = config['hex_path']
    # Evitar aceptar un binario anterior aunque CCS retorne exit code cero.
    for suffix in ('.hex', '.err', '.lst'):
        path = directory / (source.stem + suffix)
        if path.exists():
            path.unlink()
    print(f"PERFIL: {config['profile']}\nFUENTE: {source}\nHEX: {output}", flush=True)
    command = [config['compiler'], f'out={directory}', str(source), '+FH', '+DF',
               '+CC', '+Y=9', '+EA', f"I+={Path(config['ccs_root'])/'Devices'}",
               f"I+={Path(config['ccs_root'])/'Drivers'}", f'I+={ROOT}',
               f"I+={config['library_root']}", '+LN', '+T', '+A', '+M', '-J', '+Z', '-P']
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, errors='replace', timeout=120)
    for line in result.stdout.splitlines():
        match = re.match(r'^(.*):(\d+):(\d+):\s+(Error|Warning)#(\d+)\s+(.*)$', line)
        if match:
            file, row, col, severity, code, message = match.groups()
            print(f'{file}:{row}:{col}: {severity.lower()} CCS{code}: {message}', flush=True)
        else:
            print(line, flush=True)
    (directory/'compile.log').write_text(result.stdout, encoding='utf-8')
    if result.returncode or not output.exists() or not re.search(r'\b0 Errors\b', result.stdout):
        raise RuntimeError('Compilacion fallida o sin resultado verificable; no se cargara')
    listing = (directory/(source.stem+'.lst')).read_text(errors='replace')
    if 'Version 5.081' not in listing:
        raise RuntimeError('Se requiere CCS PCH 5.081; revisar compilador configurado')
    metadata = {'profile': config['profile'], 'source': str(source),
                'hex_sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
                'compiler': 'CCS PCH 5.081', 'dependencies': dependency_status(config)}
    (directory/'build.json').write_text(json.dumps(metadata, indent=2)+'\n', encoding='utf-8')
    return output
