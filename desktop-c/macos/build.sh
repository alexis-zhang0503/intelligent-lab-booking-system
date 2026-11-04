#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
export MACOSX_DEPLOYMENT_TARGET=13.0
app="$PWD/desktop-c/dist/LabReservation.app"
mkdir -p "$app/Contents/"{MacOS,Resources,Frameworks}
cc -std=c11 -Wall -Wextra desktop-c/tests/client_test.c desktop-c/src/client.c desktop-c/vendor/cJSON.c -o /tmp/lab-client-test
/tmp/lab-client-test
# GTK is a C toolkit; all application code and shared business logic are C11.
cc -std=c11 -Wall -Wextra $(pkg-config --cflags gtk+-3.0) desktop-c/macos/main.c desktop-c/macos/http.c desktop-c/src/client.c desktop-c/vendor/cJSON.c $(pkg-config --libs gtk+-3.0) -lcurl -o "$app/Contents/MacOS/lab-native"
dylibbundler -od -b -x "$app/Contents/MacOS/lab-native" -d "$app/Contents/Frameworks" -p @executable_path/../Frameworks/
cat > "$app/Contents/MacOS/LabReservation" <<'LAUNCH'
#!/bin/bash
base="$(cd "$(dirname "$0")/.." && pwd)"
export GSETTINGS_SCHEMA_DIR="$base/Resources/glib-2.0/schemas"
export GTK_DATA_PREFIX="$base/Resources"
export GTK_THEME=Adwaita
export GDK_BACKEND=quartz
exec "$base/MacOS/lab-native" "$@"
LAUNCH
chmod +x "$app/Contents/MacOS/LabReservation"
mkdir -p "$app/Contents/Resources/glib-2.0/schemas"
cp "$(brew --prefix gtk+3)/share/glib-2.0/schemas/"*.xml "$app/Contents/Resources/glib-2.0/schemas/"
glib-compile-schemas "$app/Contents/Resources/glib-2.0/schemas"
mkdir -p /tmp/lab.iconset
for size in 16 32 128 256 512; do
 sips -z "$size" "$size" assets/brand/njupt-lab-reservation-logo.png --out "/tmp/lab.iconset/icon_${size}x${size}.png" >/dev/null
 double=$((size*2));sips -z "$double" "$double" assets/brand/njupt-lab-reservation-logo.png --out "/tmp/lab.iconset/icon_${size}x${size}@2x.png" >/dev/null
done
iconutil -c icns /tmp/lab.iconset -o "$app/Contents/Resources/lab.icns"
cat > "$app/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>LabReservation</string>
<key>CFBundleIdentifier</key><string>cn.njupt.labreservation</string>
<key>CFBundleName</key><string>LabReservation</string>
<key>CFBundleDisplayName</key><string>实验室预约</string>
<key>CFBundleVersion</key><string>3.0.0</string>
<key>CFBundleShortVersionString</key><string>3.0.0</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleIconFile</key><string>lab.icns</string>
<key>LSMinimumSystemVersion</key><string>13.0</string>
<key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
# Relocation check: every non-system linked library must resolve inside the bundle.
python3 - "$app" <<'PY'
import pathlib,subprocess,sys
app=pathlib.Path(sys.argv[1]);bad=[]
for p in [app/'Contents/MacOS/lab-native',*(app/'Contents/Frameworks').glob('*')]:
 if not p.is_file():continue
 for line in subprocess.check_output(['otool','-L',str(p)],text=True).splitlines()[1:]:
  dep=line.strip().split(' (')[0]
  if dep.startswith(('/opt/homebrew','/usr/local')):bad.append((str(p),dep))
if bad:raise SystemExit(str(bad))
PY
find "$app/Contents/Frameworks" -type f -name '*.dylib' -exec codesign --force --sign - {} \;
codesign --force --sign - "$app/Contents/MacOS/lab-native"
codesign --force --sign - "$app"
codesign --verify --deep --strict "$app"
