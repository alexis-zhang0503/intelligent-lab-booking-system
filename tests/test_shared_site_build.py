import importlib.util
import json
import re
import tempfile
import unittest
import zipfile
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('build_site', ROOT / 'scripts/build_site.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)

class SharedSiteBuildTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.out = builder.build(Path(cls.temp.name) / 'site', 'test-release-123')
    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()
    def test_version_manifest_and_entry(self):
        self.assertEqual(json.loads((self.out / 'version.json').read_text())['version'], 'test-release-123')
        text = (self.out / 'index.html').read_text()
        self.assertIn('web/core/bootstrap.js?v=test-release-123', text)
        self.assertIn('data-app-version="test-release-123"', text)
        self.assertIn('id="updateNotice"', text)
    def test_local_assets_are_versioned_and_exist(self):
        text = (self.out / 'index.html').read_text()
        for ref in re.findall(r'(?:src|href)="([^"<>]+)"', text):
            if ref.startswith(('https:', 'http:', '//', '#', 'data:')):
                continue
            self.assertIn('?v=test-release-123', ref)
            self.assertTrue((self.out / ref.split('?')[0]).is_file(), ref)
    def test_shared_attendance_module_and_sdk_are_versioned(self):
        self.assertIn('attendanceRules.js?v=test-release-123', (self.out / 'app.js').read_text())
        self.assertIn('../vendor/supabase.js?v=test-release-123', (self.out / 'web/core/supabaseClient.js').read_text())
    def test_desktop_and_database_sources_are_not_public_site_assets(self):
        for path in ['desktop-c', 'qt-client', 'backend', 'tests', '.git', '.github']:
            self.assertFalse((self.out / path).exists())
    def test_bad_versions_and_unsafe_output_are_rejected(self):
        with self.assertRaises(ValueError): builder.build(ROOT, 'safe')
        with self.assertRaises(ValueError): builder.build(self.out, '../bad')
        with self.assertRaises(ValueError): builder.build(Path(self.temp.name), 'safe')



class NativeDownloadTests(unittest.TestCase):
    def test_native_manifest_and_download(self):
        spec = importlib.util.spec_from_file_location('package_native', ROOT / 'scripts/package_native.py')
        packager = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(packager)
        with tempfile.TemporaryDirectory() as temp:
            site = builder.build(Path(temp) / 'site', 'native-test')
            archive = Path(temp) / 'desktop.zip'
            with zipfile.ZipFile(archive, 'w') as output:
                output.writestr('LabReservation.exe', b'MZ-test-fixture')
            manifest = packager.attach(site, archive)
            self.assertRegex(manifest['version'], r'^\d+\.\d+\.\d+$')
            self.assertEqual(manifest['sha256'], hashlib.sha256(archive.read_bytes()).hexdigest())
            self.assertEqual((site / manifest['download']).read_bytes(), archive.read_bytes())
            self.assertEqual(json.loads((site / 'native-desktop.json').read_text()), manifest)
            with self.assertRaises(ValueError):
                packager.attach(Path(temp), archive)
            with zipfile.ZipFile(archive, 'w') as output:
                output.writestr('LabReservation.exe', b'MZ-test-fixture')
                output.writestr('WebView2Loader.dll', b'unwanted')
            with self.assertRaises(ValueError):
                packager.attach(site, archive)

if __name__ == '__main__': unittest.main()
