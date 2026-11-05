"""Real local Git repositories: no network or user credentials required."""
import copy
import hashlib
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import dependencies as deps


class DependenciesTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)/'project'
        (self.root/'config').mkdir(parents=True)
        self.remote = Path(self.temp.name)/'remote'
        self.remote.mkdir()
        self.real_git = deps.git
        self.real_git('init', cwd=self.remote)
        self.real_git('config', 'user.email', 'test@example.invalid', cwd=self.remote)
        self.real_git('config', 'user.name', 'Dependency test', cwd=self.remote)
        self.real_git('config', 'core.autocrlf', 'false', cwd=self.remote)
        self.content = b'#define TEST_VERSION 1\n'
        (self.remote/'driver.h').write_bytes(self.content)
        self.real_git('add', '.', cwd=self.remote)
        self.real_git('commit', '-m', 'Pinned version', cwd=self.remote)
        self.commit = self.real_git('rev-parse', 'HEAD', cwd=self.remote)
        (self.remote/'driver.h').write_text('#define TEST_VERSION 2\n')
        self.real_git('commit', '-am', 'Newer upstream version', cwd=self.remote)
        self.entry = {'url':'https://github.com/example/driver.git', 'commit':self.commit,
                      'sha256':{'driver.h':hashlib.sha256(self.content).hexdigest()}}
        self.data = {'schema_version':1, 'directory':'galio',
                     'libraries':{'driver':self.entry, 'unused':copy.deepcopy(self.entry)},
                     'applications':{'demo':['driver'], 'minimal':[]}}
        self.save()
        self.calls = []
        def route(*args, cwd=None):
            self.calls.append(args)
            if args[0]=='clone':
                rewritten = list(args)
                rewritten[-2] = str(self.remote)
                result = self.real_git(*rewritten, cwd=cwd)
                self.real_git('remote', 'set-url', 'origin', self.entry['url'], cwd=args[-1])
                return result
            return self.real_git(*args, cwd=cwd)
        self.addCleanup(patch.stopall)
        patch('dependencies.git', side_effect=route).start()
        patch('sys.stdout', new=io.StringIO()).start()

    def save(self):
        (self.root/'config/dependencies.json').write_text(json.dumps(self.data))

    def test_clone_pin_and_offline_reuse(self):
        deps.ensure(self.root, 'demo')
        target = self.root/'galio/driver'
        self.assertEqual((target/'driver.h').read_bytes().replace(b'\r\n',b'\n'), self.content)
        self.assertEqual(self.real_git('rev-parse', 'HEAD', cwd=target), self.commit)
        self.assertFalse((self.root/'galio/unused').exists())
        self.calls.clear()
        deps.ensure(self.root, 'demo')
        self.assertFalse(any(c[0] in ('clone','fetch','pull','checkout') for c in self.calls))

    def test_app_manifests_are_isolated(self):
        self.data.pop('applications')
        self.data['libraries'].pop('unused')
        self.save()
        other = self.root/'other-app'
        (other/'config').mkdir(parents=True)
        (other/'config/dependencies.json').write_text(json.dumps({
            'schema_version': 1, 'directory': 'galio', 'libraries': {}}))
        deps.ensure(self.root, 'demo')
        self.assertEqual(deps.ensure(other, 'demo'), {})
        self.assertFalse((other/'galio').exists())
        self.assertTrue((self.root/'galio/driver/driver.h').is_file())

    def test_modified_and_untracked_files_preserved(self):
        deps.ensure(self.root, 'demo')
        target = self.root/'galio/driver'
        (target/'driver.h').write_text('local edit')
        (target/'notes.txt').write_text('my notes')
        with self.assertRaisesRegex(RuntimeError, 'cambios locales'):
            deps.ensure(self.root, 'demo')
        self.assertEqual((target/'driver.h').read_text(), 'local edit')
        self.assertEqual((target/'notes.txt').read_text(), 'my notes')

    def test_wrong_origin_and_version_preserved(self):
        deps.ensure(self.root, 'demo')
        target = self.root/'galio/driver'
        newest = self.real_git('rev-parse', 'HEAD', cwd=self.remote)
        self.real_git('checkout', '--detach', newest, cwd=target)
        self.real_git('remote', 'set-url', 'origin', 'https://github.com/example/other.git', cwd=target)
        with self.assertRaisesRegex(RuntimeError, 'origin distinto.*commit distinto'):
            deps.ensure(self.root, 'demo')
        self.assertEqual(self.real_git('rev-parse','HEAD',cwd=target), newest)

    def test_bad_hash_never_installs(self):
        self.entry['sha256']['driver.h']='0'*64
        self.save()
        with self.assertRaisesRegex(RuntimeError, 'huella distinta'):
            deps.ensure(self.root, 'demo')
        self.assertFalse((self.root/'galio/driver').exists())

    def test_existing_nonrepo_untouched(self):
        target = self.root/'galio/driver'
        target.mkdir(parents=True)
        (target/'keep.txt').write_text('keep')
        with self.assertRaisesRegex(RuntimeError, 'sin repositorio'):
            deps.ensure(self.root, 'demo')
        self.assertEqual((target/'keep.txt').read_text(), 'keep')

    def test_empty_profile_and_unknown_profile(self):
        deps.ensure(self.root, 'minimal')
        self.assertEqual(self.calls, [])
        self.assertFalse((self.root/'galio').exists())
        with self.assertRaisesRegex(ValueError, 'Falta declarar'):
            deps.ensure(self.root, 'typo')

    def test_path_traversal_rejected(self):
        self.data['libraries']['../outside']=self.entry
        self.save()
        with self.assertRaises(ValueError):
            deps.ensure(self.root, 'demo')
        self.assertFalse((self.root/'galio').exists())

    def test_auth_or_network_failure_leaves_no_final_repo(self):
        with patch('dependencies.git', side_effect=RuntimeError('authentication failed')):
            with self.assertRaisesRegex(RuntimeError, 'descarga incompleta'):
                deps.ensure(self.root, 'demo')
        self.assertFalse((self.root/'galio/driver').exists())

    def windows_lock(self, code=32):
        error = PermissionError('Directory is in use')
        error.winerror = code
        return error

    def test_windows_lock_retries_without_cloning_again(self):
        real_rename = Path.rename
        failures = [self.windows_lock(32), self.windows_lock(5), self.windows_lock(33)]

        def rename(source, target):
            if failures:
                raise failures.pop(0)
            return real_rename(source, target)

        with patch.object(Path, 'rename', autospec=True, side_effect=rename) as move, \
             patch('dependencies.time.sleep') as sleep:
            deps.ensure(self.root, 'demo')
        self.assertEqual(move.call_count, 4)
        self.assertEqual(sleep.call_count, 3)
        self.assertEqual(sum(c[0] == 'clone' for c in self.calls), 1)
        self.assertEqual(deps.status(self.root, 'demo'), {'driver': []})

    def leave_locked_download(self):
        with patch.object(Path, 'rename', side_effect=self.windows_lock()) as move, \
             patch('dependencies.time.sleep') as sleep:
            with self.assertRaisesRegex(RuntimeError, 'Windows mantiene bloqueada'):
                deps.ensure(self.root, 'demo')
        self.assertEqual(move.call_count, 8)
        self.assertEqual(sleep.call_count, 7)
        self.assertFalse((self.root/'galio/driver').exists())
        staging, = (self.root/'galio').glob('.driver-download-*')
        self.assertEqual(deps.inspect(staging, self.entry), [])
        return staging

    def test_persistent_lock_keeps_verified_download_for_offline_retry(self):
        staging = self.leave_locked_download()
        self.calls.clear()
        deps.ensure(self.root, 'demo')
        self.assertFalse(staging.exists())
        self.assertEqual(deps.status(self.root, 'demo'), {'driver': []})
        self.assertFalse(any(c[0] in ('clone', 'fetch', 'checkout', 'pull') for c in self.calls))

    def test_recovery_preserves_modified_staging(self):
        staging = self.leave_locked_download()
        (staging/'notes.txt').write_text('keep my notes')
        self.calls.clear()
        deps.ensure(self.root, 'demo')
        self.assertEqual((staging/'notes.txt').read_text(), 'keep my notes')
        self.assertEqual(sum(c[0] == 'clone' for c in self.calls), 1)
        self.assertEqual(deps.status(self.root, 'demo'), {'driver': []})

    def test_recovery_rechecks_manifest_hashes(self):
        staging = self.leave_locked_download()
        self.entry['sha256']['driver.h'] = '0'*64
        self.save()
        with self.assertRaisesRegex(RuntimeError, 'huella distinta'):
            deps.ensure(self.root, 'demo')
        self.assertTrue(staging.is_dir())
        self.assertFalse((self.root/'galio/driver').exists())

    def test_incomplete_staging_is_preserved(self):
        staging = self.root/'galio/.driver-download-incomplete'
        staging.mkdir(parents=True)
        (staging/'notes.txt').write_text('keep')
        deps.ensure(self.root, 'demo')
        self.assertEqual((staging/'notes.txt').read_text(), 'keep')
        self.assertEqual(deps.status(self.root, 'demo'), {'driver': []})

    def test_unrelated_rename_error_is_not_retried(self):
        with patch.object(Path, 'rename', side_effect=OSError('disk full')) as move, \
             patch('dependencies.time.sleep') as sleep:
            with self.assertRaisesRegex(RuntimeError, 'disk full'):
                deps.ensure(self.root, 'demo')
        move.assert_called_once()
        sleep.assert_not_called()

    def test_destination_appearing_during_retry_is_not_replaced(self):
        target = self.root/'galio/driver'

        def competing_process(_delay):
            target.mkdir()
            (target/'keep.txt').write_text('other process')

        with patch.object(Path, 'rename', side_effect=self.windows_lock()) as move, \
             patch('dependencies.time.sleep', side_effect=competing_process):
            with self.assertRaisesRegex(RuntimeError, 'Otra ejecucion'):
                deps.ensure(self.root, 'demo')
        move.assert_called_once()
        self.assertEqual((target/'keep.txt').read_text(), 'other process')
