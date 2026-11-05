"""Pinned, per-application Git dependencies in <project>/galio.

Existing checkouts are read-only to this tool: never pull/reset/stash/clean.
Only a new temporary clone is checked out, verified, then renamed into place.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


def manifest(root):
    data = json.loads((Path(root)/'config/dependencies.json').read_text(encoding='utf-8'))
    if data.get('schema_version') != 1 or data.get('directory') != 'galio':
        raise ValueError('Manifiesto de dependencias no compatible')
    for name, entry in data['libraries'].items():
        if not re.fullmatch(r'[A-Za-z0-9_-]+', name):
            raise ValueError('Nombre de dependencia invalido')
        if not re.fullmatch(r'https://github\.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+\.git', entry['url']):
            raise ValueError(f'{name}: URL GitHub HTTPS invalida (sin tokens)')
        if not re.fullmatch(r'[0-9a-f]{40}', entry['commit']):
            raise ValueError(f'{name}: se requiere commit Git completo')
        for file, digest in entry['sha256'].items():
            if Path(file).is_absolute() or '..' in Path(file).parts or '\\' in file or ':' in file:
                raise ValueError(f'{name}: archivo fuera de la dependencia')
            if not re.fullmatch(r'[0-9a-f]{64}', digest):
                raise ValueError(f'{name}: SHA256 invalido')
    for names in data.get('applications', {}).values():
        if any(name not in data['libraries'] for name in names):
            raise ValueError('Aplicacion referencia una dependencia desconocida')
    return data


def selected(data, profile, all_libraries=False):
    if all_libraries or 'applications' not in data:
        return list(data['libraries'])
    if profile not in data['applications']:
        raise ValueError(f'Falta declarar dependencias de {profile} en config/dependencies.json')
    return data['applications'][profile]


def git(*args, cwd=None):
    env = dict(os.environ, GIT_TERMINAL_PROMPT='0', GCM_INTERACTIVE='Never')
    try:
        result = subprocess.run(['git', *args], cwd=cwd, env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                text=True, errors='replace', timeout=180)
    except FileNotFoundError as exc:
        raise RuntimeError('Instalar Git para Windows y volver a abrir VS Code.') from exc
    except subprocess.TimeoutExpired as exc:
        raise RuntimeError('Git excedio 180 s; revisar conexion/autenticacion y reintentar.') from exc
    if result.returncode:
        raise RuntimeError(f"Git fallo ({args[0]}): {result.stderr.strip()}\n"
                           'Para repos privados, iniciar sesion en Git Credential Manager antes de compilar.')
    return result.stdout.strip()


def destination(root, name):
    root = Path(root).resolve()
    base = root/'galio'
    path = base/name
    if base.resolve() != base or path.resolve() != path:
        raise RuntimeError('galio y sus repositorios no deben ser enlaces/junctions externos.')
    return path


def inspect(path, entry):
    if not path.exists():
        return ['ausente']
    if not (path/'.git').is_dir():
        return ['carpeta existente sin repositorio Git; no se modifica']
    issues = []
    # Avoid accidentally accepting an enclosing repository or another origin.
    if Path(git('rev-parse', '--show-toplevel', cwd=path)).resolve() != path.resolve():
        return ['raiz Git inesperada']
    if git('remote', 'get-url', 'origin', cwd=path) != entry['url']:
        issues.append('origin distinto del manifiesto')
    if git('rev-parse', 'HEAD', cwd=path) != entry['commit']:
        issues.append('commit distinto del fijado')
    if git('status', '--porcelain', '--untracked-files=all', cwd=path):
        issues.append('cambios locales; se conservan, compilacion detenida')
    for file, expected in entry['sha256'].items():
        target = path/file
        if not target.resolve().is_relative_to(path.resolve()):
            issues.append(f'archivo fuera del repositorio: {file}')
        elif not target.is_file():
            issues.append(f'ausente: {file}')
        elif hashlib.sha256(target.read_bytes().replace(b'\r\n', b'\n')).hexdigest() != expected:
            issues.append(f'huella distinta: {file}')
    return issues


def status(root, profile, all_libraries=False):
    data = manifest(root)
    return {name: inspect(destination(root, name), data['libraries'][name])
            for name in selected(data, profile, all_libraries)}


def ensure(root, profile, all_libraries=False):
    data = manifest(root)
    names = selected(data, profile, all_libraries)
    for name in names:
        entry = data['libraries'][name]
        path = destination(root, name)
        if not path.exists():
            path.parent.mkdir(parents=True, exist_ok=True)
            print(f"DEPENDENCIA {name}: descargando {entry['url']} @ {entry['commit'][:12]}", flush=True)
            # Keep failed clones for diagnosis; never recursively delete user data.
            staging = Path(tempfile.mkdtemp(prefix=f'.{name}-download-', dir=path.parent))
            try:
                # CCS 5.081 misparses some #if expressions in LF-only headers.
                # Git tracks normalized LF; Windows compiler receives CRLF.
                git('clone', '--no-checkout', '--config', 'core.autocrlf=true', entry['url'], str(staging))
                git('checkout', '--detach', entry['commit'], cwd=staging)
                issues = inspect(staging, entry)
                if issues:
                    raise RuntimeError('; '.join(issues))
                if path.exists():
                    raise RuntimeError('Otra ejecucion creo el destino; reintentar')
                staging.rename(path)
            except Exception as exc:
                raise RuntimeError(f'{name}: descarga incompleta en {staging}; {exc}') from exc
        issues = inspect(path, entry)
        if issues:
            raise RuntimeError(f"{name}: {'; '.join(issues)}. No se modifico el repositorio existente.")
        print(f'DEPENDENCIA {name}: OK @ {entry["commit"][:12]}', flush=True)
    return {name: [] for name in names}
