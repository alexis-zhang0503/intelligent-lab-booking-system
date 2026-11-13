import importlib.util
import json
import re
import tempfile
import unittest
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
        self.assertEqual(json.loads((self.out / 'version.json').read_text(encoding='utf-8'))['version'], 'test-release-123')
        text = (self.out / 'index.html').read_text(encoding='utf-8')
        self.assertIn('js/core/bootstrap.js?v=test-release-123', text)
        self.assertIn('data-app-version="test-release-123"', text)
        self.assertIn('id="updateNotice"', text)
    def test_local_assets_are_versioned_and_exist(self):
        text = (self.out / 'index.html').read_text(encoding='utf-8')
        for ref in re.findall(r'(?:src|href)="([^"<>]+)"', text):
            if ref.startswith(('https:', 'http:', '//', '#', 'data:')):
                continue
            self.assertIn('?v=test-release-123', ref)
            self.assertTrue((self.out / ref.split('?')[0]).is_file(), ref)
    def test_shared_attendance_module_and_sdk_are_versioned(self):
        self.assertIn('attendanceRules.js?v=test-release-123', (self.out / 'app.js').read_text(encoding='utf-8'))
        self.assertIn('../vendor/supabase.js?v=test-release-123', (self.out / 'js/core/supabaseClient.js').read_text(encoding='utf-8'))
    def test_desktop_and_database_sources_are_not_public_site_assets(self):
        for path in ['tests', 'scripts']:
            self.assertFalse((self.out / path).exists())
    def test_bad_versions_and_unsafe_output_are_rejected(self):
        with self.assertRaises(ValueError): builder.build(ROOT, 'safe')
        with self.assertRaises(ValueError): builder.build(self.out, '../bad')
        with self.assertRaises(ValueError): builder.build(Path(self.temp.name), 'safe')

if __name__ == '__main__': unittest.main()
