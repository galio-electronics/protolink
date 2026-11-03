import io
import json
import tempfile
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import serial_ports
import project


class SerialPortsTests(unittest.TestCase):
    def setUp(self):
        self.ports = [SimpleNamespace(device='COM8', description='USB Serial'),
                      SimpleNamespace(device='COM26', description='CP210x')]

    def test_save_preserves_other_settings_and_loads_for_all_profiles(self):
        test_root = project.ROOT / 'build'
        test_root.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=test_root, prefix='port-test-') as folder:
            root = Path(folder)
            (root/'config').mkdir()
            (root/'main.c').touch()
            settings = root/'config/local.json'
            settings.write_text(json.dumps({'compiler': 'custom', 'other': 42, 'port': 'COM1'}))
            serial_ports.save_port('COM26', settings)
            self.assertEqual(json.loads(settings.read_text()),
                             {'compiler': 'custom', 'other': 42, 'port': 'COM26'})
            config = root/'config/project.json'
            config.write_text(json.dumps({'active_profile': 'a', 'port': 'COM1',
                'profiles': {'a': {'source': 'main.c'}, 'b': {'source': 'main.c'}}}))
            with patch.object(project, 'ROOT', root):
                for name in ('a', 'b'):
                    self.assertEqual(project.load_config(config, name)['port'], 'COM26')
                self.assertEqual(project.load_config(config, 'a', 'COM8')['port'], 'COM8')

    def test_interactive_selection_with_invalid_input(self):
        with patch('serial_ports.detected_ports', return_value=self.ports), \
             patch('builtins.input', side_effect=['99', '2']), \
             patch('serial_ports.save_port') as save, patch('sys.stdout', new=io.StringIO()):
            serial_ports.select_port('COM8')
        save.assert_called_once_with('COM26')

    def test_refresh_after_connect(self):
        with patch('serial_ports.detected_ports', side_effect=[[], self.ports, self.ports]), \
             patch('builtins.input', side_effect=['r', 'com8']), \
             patch('serial_ports.save_port') as save, patch('sys.stdout', new=io.StringIO()):
            serial_ports.select_port('COM26')
        save.assert_called_once_with('COM8')

    def test_cancel_does_not_write(self):
        with patch('serial_ports.detected_ports', return_value=[]), \
             patch('builtins.input', return_value=''), patch('serial_ports.save_port') as save, \
             patch('sys.stdout', new=io.StringIO()):
            serial_ports.select_port('COM26')
        save.assert_not_called()

    def test_disconnected_selection_does_not_write(self):
        with patch('serial_ports.detected_ports', side_effect=[self.ports, []]), \
             patch('serial_ports.save_port') as save, patch('sys.stdout', new=io.StringIO()):
            with self.assertRaisesRegex(ValueError, 'desconecto'):
                serial_ports.select_port('COM8', 'COM26')
        save.assert_not_called()

    def test_missing_port_explains_how_to_select(self):
        with patch('serial_ports.detected_ports', return_value=self.ports):
            with self.assertRaisesRegex(ValueError, 'Seleccionar puerto COM'):
                serial_ports.require_port({'port': 'COM99', 'profile': 'test'})

    def test_task_available_in_every_app_workspace(self):
        paths = [project.ROOT/'.vscode/tasks.json', *project.ROOT.glob('firmware/**/*.code-workspace')]
        for path in paths:
            data = json.loads(path.read_text())
            tasks = data['tasks'] if path.suffix == '.json' else data['tasks']['tasks']
            matches = [t for t in tasks if t['label'] == 'Protolink: Seleccionar puerto COM']
            self.assertEqual(len(matches), 1, str(path))
            self.assertIn('select-port', matches[0]['args'])
            self.assertTrue(matches[0]['presentation']['focus'])
