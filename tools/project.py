"""Perfiles y compilacion CCS reproducible desde el repositorio."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = ROOT / 'config/project.json'


def load_config(path=DEFAULT_CONFIG, profile=None, port=None):
    path = Path(path).resolve()
    config = json.loads(path.read_text(encoding='utf-8-sig'))
    local = ROOT / 'config/local.json'
    if local.exists():
        overrides = json.loads(local.read_text(encoding='utf-8-sig'))
        for key in ('compiler', 'ccs_root', 'library_root', 'port'):
            if key in overrides:
                config[key] = overrides[key]
    chosen = profile or config['active_profile']
    if chosen not in config['profiles'] or not re.fullmatch(r'[a-zA-Z0-9_-]+', chosen):
        raise ValueError(f'Perfil no valido: {chosen}')
    source = (ROOT / config['profiles'][chosen]['source']).resolve()
    if not source.is_relative_to(ROOT) or not source.is_file():
        raise ValueError(f'Fuente inexistente o fuera del repositorio: {source}')
    config.update(profile=chosen, source=source, output_dir=ROOT/'build'/chosen)
    config['hex_path'] = config['output_dir'] / (source.stem + '.hex')
    config['config_path'] = path
    if port:
        config['port'] = port
    return config


def dependency_status(config):
    lock = json.loads((ROOT/'config/dependencies.lock.json').read_text(encoding='utf-8'))
    result = {}
    for name, entry in lock['libraries'].items():
        issues = []
        for file, expected in entry['sha256'].items():
            path = Path(config['library_root'])/name/file
            if not path.exists():
                issues.append(f'ausente: {file}')
            elif hashlib.sha256(path.read_bytes()).hexdigest() != expected:
                issues.append(f'modificado: {file}')
        result[name] = issues
    return result


def compile_project(config):
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
