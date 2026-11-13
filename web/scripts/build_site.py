"""Build the GitHub Pages site from web/, stamping one release version on every local asset."""
import argparse, json, re, shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {'.html', '.js', '.css', '.json', '.svg', '.png', '.jpg', '.jpeg', '.webp', '.ico', '.woff', '.woff2', '.ttf', '.txt'}
SKIP = {'tests', 'scripts', 'node_modules', '_site'}

def versioned(value, version):
    if value.startswith(('https:', 'http:', '//', 'data:', '#', 'blob:')):
        return value
    return value.split('?', 1)[0] + '?v=' + version

def build(out, version):
    if not re.fullmatch(r'[A-Za-z0-9._-]{1,80}', version):
        raise ValueError('invalid release version')
    out = out.resolve()
    if out == ROOT or ROOT.is_relative_to(out):
        raise ValueError('output must not contain the source repository')
    if out.exists():
        # Only delete a previous generated site, never an arbitrary destination.
        if not (out / '.lab-site-build').is_file():
            raise ValueError('output exists and is not a generated lab site')
        shutil.rmtree(out)
    out.mkdir(parents=True)
    (out / '.lab-site-build').write_text('generated\n')
    paths = [p for p in ROOT.rglob('*') if p.is_file() and
             not any(part in SKIP for part in p.relative_to(ROOT).parts) and
             p.suffix.lower() in FILES and out not in p.parents]
    for source in paths:
        rel = source.relative_to(ROOT)
        target = out / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        if source.suffix.lower() not in {'.html', '.js', '.css'}:
            shutil.copyfile(source, target)
            continue
        text = source.read_text(encoding='utf-8')
        if source.suffix == '.html':
            text = re.sub(r'((?:src|href)=")([^"<>]+)(")', lambda m: m[1] + versioned(m[2], version) + m[3], text)
            text = re.sub(r'data-app-version="[^"]*"', 'data-app-version="' + version + '"', text)
        elif source.suffix == '.js' and 'vendor' not in rel.parts:
            text = re.sub(r'((?:\bfrom\s+|\bimport\s*\()\s*["\'])(\.{1,2}/[^"\']+)(["\'])',
                          lambda m: m[1] + versioned(m[2], version) + m[3], text)
        elif source.suffix == '.css':
            text = re.sub(r'url\(("|\')?(\.{0,2}/?[^)"\']+)\1\)',
                          lambda m: 'url("' + versioned(m[2], version) + '")', text)
        target.write_text(text, encoding='utf-8')
    (out / 'version.json').write_text(json.dumps({'version': version}, ensure_ascii=False), encoding='utf-8')
    (out / '.nojekyll').touch()
    return out

if __name__ == '__main__':
    args = argparse.ArgumentParser()
    args.add_argument('--out', type=Path, default=ROOT / '_site')
    args.add_argument('--version', required=True)
    opts = args.parse_args()
    print(build(opts.out, opts.version))
