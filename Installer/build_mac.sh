#!/usr/bin/env bash
# Compila Nova Vocal y genera el instalador con asistente (.pkg) para macOS:
#   Bienvenida > Licencia > Instalacion personalizada (VST3 / AU / Standalone / AAX) > Disco > Instalar > Fin
# Requisitos: Xcode o Command Line Tools (xcode-select --install) y cmake (brew install cmake)
#
# Uso:                ./Installer/build_mac.sh
# Con firma real:     SIGN_ID="Developer ID Application: Tu Nombre (TEAMID)" \
#                     INSTALLER_SIGN_ID="Developer ID Installer: Tu Nombre (TEAMID)" ./Installer/build_mac.sh
# Con AAX (opcional): AAX_SDK_PATH=/ruta/al/AAX_SDK ./Installer/build_mac.sh
set -euo pipefail
cd "$(dirname "$0")/.."

VERSION="1.0.0"
ID="com.novaaudio.novavocal"
BUILD_DIR="build"
DIST="$(pwd)/dist"

command -v cmake >/dev/null || { echo "Falta cmake. Instalalo con: brew install cmake"; exit 1; }

echo "==> Configurando (se descarga JUCE la primera vez)..."
EXTRA=()
[ -n "${AAX_SDK_PATH:-}" ] && EXTRA+=("-DAAX_SDK_PATH=$AAX_SDK_PATH")
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" ${EXTRA[@]+"${EXTRA[@]}"}

echo "==> Compilando..."
TARGETS=(NovaVocal_VST3 NovaVocal_AU NovaVocal_Standalone)
[ -n "${AAX_SDK_PATH:-}" ] && TARGETS+=(NovaVocal_AAX)
cmake --build "$BUILD_DIR" --config Release --target "${TARGETS[@]}" -j "$(sysctl -n hw.ncpu)"

VST3="$(find "$BUILD_DIR" -type d -name 'Nova Vocal.vst3' | head -n1)"
AU="$(find "$BUILD_DIR" -type d -name 'Nova Vocal.component' | head -n1)"
APP="$(find "$BUILD_DIR" -type d -name 'Nova Vocal.app' | head -n1)"
AAX="$(find "$BUILD_DIR" -type d -name 'Nova Vocal.aaxplugin' | head -n1)"
[ -n "$VST3" ] && [ -n "$AU" ] && [ -n "$APP" ] || { echo "No se encontraron los bundles compilados."; exit 1; }

echo "==> Firmando..."
for bundle in "$VST3" "$AU" "$APP" "$AAX"; do
  [ -n "$bundle" ] || continue
  if [ -n "${SIGN_ID:-}" ]; then
    codesign --force --deep --options runtime --timestamp --sign "$SIGN_ID" "$bundle"
  else
    codesign --force --deep --sign - "$bundle"      # firma ad-hoc (necesaria en Apple Silicon)
  fi
done

echo "==> Armando paquetes por componente..."
rm -rf "$DIST"; mkdir -p "$DIST/root"
mkpkg() {  # nombre  carpeta-destino  bundle
  local name="$1" dest="$2" bundle="$3" stage="$DIST/root/$1"
  mkdir -p "$stage$dest"; cp -R "$bundle" "$stage$dest/"
  pkgbuild --root "$stage" --identifier "$ID.$name" --version "$VERSION" --install-location / \
           ${4:+--scripts "$4"} "$DIST/NovaVocal-$name.pkg"
}
mkpkg vst3 "/Library/Audio/Plug-Ins/VST3" "$VST3"
mkpkg au   "/Library/Audio/Plug-Ins/Components" "$AU" Installer/scripts
mkpkg app  "/Applications" "$APP"
[ -n "$AAX" ] && mkpkg aax "/Library/Application Support/Avid/Audio/Plug-Ins" "$AAX"

echo "==> Armando el asistente..."
AAX_CHOICE=""; AAX_OUTLINE=""; AAX_REF=""
if [ -n "$AAX" ]; then
  AAX_OUTLINE='<line choice="aax"/>'
  AAX_CHOICE="<choice id=\"aax\" title=\"Plugin AAX (Pro Tools)\" description=\"Se instala en /Library/Application Support/Avid/Audio/Plug-Ins\" start_selected=\"false\"><pkg-ref id=\"$ID.aax\"/></choice>"
  AAX_REF="<pkg-ref id=\"$ID.aax\" version=\"$VERSION\" onConclusion=\"none\">NovaVocal-aax.pkg</pkg-ref>"
fi
cat > "$DIST/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Nova Vocal</title>
  <welcome file="welcome.html" mime-type="text/html"/>
  <license file="license.html" mime-type="text/html"/>
  <conclusion file="conclusion.html" mime-type="text/html"/>
  <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <domains enable_localSystem="true"/>
  <choices-outline>
    <line choice="vst3"/><line choice="au"/><line choice="app"/>$AAX_OUTLINE
  </choices-outline>
  <choice id="vst3" title="Plugin VST3" description="FL Studio, Reaper, Cubase, Ableton... Se instala en /Library/Audio/Plug-Ins/VST3" start_selected="true"><pkg-ref id="$ID.vst3"/></choice>
  <choice id="au" title="Audio Unit (AU)" description="Logic Pro, GarageBand, MainStage. Se instala en /Library/Audio/Plug-Ins/Components" start_selected="true"><pkg-ref id="$ID.au"/></choice>
  <choice id="app" title="Aplicación independiente" description="Se instala en /Applications" start_selected="false"><pkg-ref id="$ID.app"/></choice>
  $AAX_CHOICE
  <pkg-ref id="$ID.vst3" version="$VERSION" onConclusion="none">NovaVocal-vst3.pkg</pkg-ref>
  <pkg-ref id="$ID.au" version="$VERSION" onConclusion="none">NovaVocal-au.pkg</pkg-ref>
  <pkg-ref id="$ID.app" version="$VERSION" onConclusion="none">NovaVocal-app.pkg</pkg-ref>
  $AAX_REF
</installer-gui-script>
XML
productbuild --distribution "$DIST/distribution.xml" --resources Installer/resources \
             --package-path "$DIST" "$DIST/NovaVocal-Installer.pkg"

if [ -n "${INSTALLER_SIGN_ID:-}" ]; then
  productsign --sign "$INSTALLER_SIGN_ID" "$DIST/NovaVocal-Installer.pkg" "$DIST/NovaVocal-Installer-signed.pkg"
  echo "Firmado: $DIST/NovaVocal-Installer-signed.pkg"
fi
echo ""
echo "Listo: $DIST/NovaVocal-Installer.pkg"
