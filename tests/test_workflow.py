import io
import json
from pathlib import Path
import unittest
from unittest.mock import patch, Mock
import project
from station_upload import upload, record
from autoboot_probe import crossed_sequence


class WorkflowTests(unittest.TestCase):
    def test_crossed_sequence(self):
        events = []
        crossed_sequence(lambda line,v: events.append((line,v)),
                         lambda ms: events.append(('wait',ms)),100,200,500)
        self.assertEqual(events,[('dtr',False),('rts',False),('wait',100),
            ('rts',True),('wait',200),('dtr',True),('rts',False),('wait',500),('dtr',False)])

    def test_profile_selects_main_not_open_file(self):
        config = project.load_config(profile='station-client')
        self.assertEqual(config['source'],project.ROOT/'firmware/examples/station-client/main.c')
        self.assertEqual(config['hex_path'],project.ROOT/'firmware/examples/station-client/build/station-client/main.hex')

    def test_every_app_owns_its_dependencies_and_outputs(self):
        for path in project.ROOT.glob('firmware/**/config/project.json'):
            config = project.load_config(path)
            app = path.parent.parent
            self.assertEqual(config['library_root'], app/'galio')
            self.assertTrue(config['output_dir'].is_relative_to(app))
            self.assertTrue((app/'config/dependencies.json').is_file())

    def test_unknown_profile_rejected(self):
        with self.assertRaises(ValueError): project.load_config(profile='not-a-profile')

    def test_upload_keeps_port_open_through_monitor(self):
        ser = Mock()
        ser.is_open = True
        ser.port = 'FAKE'
        ser.in_waiting = 0
        # Esperas simuladas; banner completo seguido del ACK/XON final.
        ser.read.side_effect = [b'name=Protolink-Station-Monitor,version=BOOT:1.2,x\n',b'\x06\x11']
        lines = [record(1)]
        ser.write.side_effect = lambda line: len(line)
        def sequence(setter, wait, *args): wait(1)
        def show_monitor(*args): ser.close.assert_not_called()
        # monotonico: esperar una lectura de banner, no bloquear.
        ticks = iter([0,0,2,3,4,4,4])
        with patch('station_upload.open_serial',return_value=ser), \
             patch('station_upload.crossed_sequence',side_effect=sequence), \
             patch('station_upload.time.sleep'), \
             patch('station_upload.time.monotonic',side_effect=lambda: next(ticks,4)), \
             patch('station_upload.await_ack') as ack, \
             patch('station_upload.monitor',side_effect=show_monitor) as monitor, \
             patch('sys.stdout',new=io.StringIO()):
            upload({'port':'FAKE','baud':115200,'profile':'test'},lines,5)
        ack.assert_called_once_with(ser)
        monitor.assert_called_once()
        ser.close.assert_called_once()

    def test_task_matcher_matches_ccs(self):
        import re
        tasks=json.loads((project.ROOT/'firmware/apps/station-demo/station-demo.code-workspace').read_text())
        pattern=tasks['tasks']['tasks'][0]['problemMatcher'][0]['pattern']['regexp']
        match=re.match(pattern,r'G:\path\main.c:12:3: error CCS90: Bad expression')
        self.assertEqual(match.group(2),'12')
        self.assertEqual(match.group(4),'error')


if __name__ == '__main__': unittest.main()
