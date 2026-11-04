"""Add a separately compiled native-C Windows download to a generated web site."""
import argparse
import hashlib
import json
import re
import shutil
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def attach(site, archive):
    site, archive = site.resolve(), archive.resolve()
    if not (site / '.lab-site-build').is_file():
        raise ValueError('destination must be a generated site')
    with zipfile.ZipFile(archive) as source:
        executables = [n for n in source.namelist() if Path(n).name == 'LabReservation.exe']
        if len(executables) != 1 or source.read(executables[0])[:2] != b'MZ':
            raise ValueError('archive must contain one Windows LabReservation.exe')
        if any('webview' in n.lower() for n in source.namelist()):
            raise ValueError('browser runtime is not part of the native desktop')
    header = (ROOT / 'desktop-c/src/client.h').read_text()
    version = re.search(r'#define LAB_NATIVE_VERSION "(\d+\.\d+\.\d+)"', header)[1]
    download = site / 'downloads/LabReservation-Windows-x64.zip'
    download.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(archive, download)
    manifest = {'version': version, 'platform': 'windows-x64',
                'download': 'downloads/LabReservation-Windows-x64.zip',
                'sha256': hashlib.sha256(download.read_bytes()).hexdigest()}
    (site / 'native-desktop.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--site', required=True, type=Path)
    parser.add_argument('--zip', required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(attach(args.site, args.zip)))
