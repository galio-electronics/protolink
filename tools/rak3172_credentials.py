"""Create local OTAA credentials once; never rotate an existing device silently."""
import json
from pathlib import Path
import secrets

root = Path(__file__).resolve().parents[1] / 'firmware/apps/station-rak3172'
header = root / 'credentials.local.h'
registration = root / 'registration.local.json'
if header.exists() or registration.exists():
    raise SystemExit('Credenciales existentes: se conservan sin cambios.')
join_eui = '02' + secrets.token_hex(7).upper()
app_key = secrets.token_hex(16).upper()
header.write_text(f'#define DEMO_JOIN_EUI "{join_eui}"\n'
                  f'#define DEMO_APP_KEY "{app_key}"\n', encoding='ascii')
registration.write_text(json.dumps({
    'name': 'station-rak3172', 'dev_eui': 'READ_FROM_LORA_QUERY_2',
    'join_eui': join_eui, 'app_key': app_key, 'activation': 'OTAA',
    'region': 'US915', 'channels': '0-7', 'mask': '0001'
}, indent=2) + '\n', encoding='ascii')
print('Credenciales creadas en firmware/apps/station-rak3172/registration.local.json')
