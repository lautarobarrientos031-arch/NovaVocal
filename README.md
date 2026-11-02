# Nova Vocal

Plugin de cadena vocal (AU + VST3 + Standalone) con interfaz de tres pestañas:

- **Vocal Assist**: escuchás una toma de voz, el plugin la analiza y propone toda la mezcla.
- **Simple**: cuatro perillas grandes (Comp, Magic, Reverb, Delay) con selectores y cortes.
- **Advanced**: una tarjeta por sección; cada una abre **su propia ventana** con todos los controles.

Secciones: EQ · Compresor · De-esser · Magic (color/saturación) · Delay · Reverb · Autotune.

## Compilar e instalar en Mac (para Logic Pro)

> Logic Pro **solo carga Audio Units (AU)**, no VST3. El instalador incluye los dos formatos.

1. Instalá las herramientas (una sola vez):
   ```bash
   xcode-select --install
   brew install cmake
   ```
2. Desde la carpeta del proyecto:
   ```bash
   ./Installer/build_mac.sh
   ```
   La primera vez descarga JUCE (necesita internet). Al terminar queda
   `dist/NovaVocal-Installer.pkg`.
3. Abrí el `.pkg` y seguí el instalador. En Logic: **Audio FX → NovaAudio → Nova Vocal**.
   Si no aparece, reiniciá Logic y volvé a analizar los plug-ins desde su Gestor de plug-ins.

Firma para distribuir a otras personas (opcional):
`SIGN_ID="Developer ID Application: Tu Nombre (TEAMID)" ./Installer/build_mac.sh`
y luego notarizar el `.pkg` con `xcrun notarytool`. Sin firma, el plugin funciona en tu Mac
(firma ad-hoc), pero en otras Macs Gatekeeper pedirá aprobación manual.

## El instalador (asistente "Siguiente")

**Windows (`NovaVocal-Windows-Installer.exe`)**: Bienvenida, Licencia, Componentes (VST3 / Standalone / AAX si existe),
**Carpeta de instalación** (por defecto `C:\Program Files\Common Files\VST3`; podés elegir otra con *Examinar*),
Listo para instalar, Instalando y Finalizar. Incluye desinstalador en *Aplicaciones* de Windows.
Si instalás el VST3 en otra carpeta, agregala en FL Studio: *Options > Manage plugins*.

**Mac (`NovaVocal-Installer.pkg`)**: Bienvenida, Licencia, *Personalizar* (VST3, AU, Standalone, AAX si existe),
elección de disco e Instalar. macOS fija las carpetas de plugins (`/Library/Audio/Plug-Ins/...`); solo se puede elegir el disco.

Antes de distribuir, reemplazá el texto de licencia: `Installer/licencia.txt` (Windows) y `Installer/resources/license.html` (Mac).

## FL Studio (Windows o Mac)

FL Studio carga **VST3**, y el instalador lo deja en la carpeta estandar
(`C:\Program Files\Common Files\VST3` en Windows, `/Library/Audio/Plug-Ins/VST3` en Mac).
Despues: *Options > Manage plugins > Start scan*; el plugin aparece en un slot de efecto del Mixer.
**FL Studio no carga AAX** (es solo de Pro Tools). Si igual lo necesitas, compila con el AAX SDK de
Avid: `AAX_SDK_PATH=/ruta/al/SDK ./Installer/build_mac.sh` (en Windows: `set AAX_SDK_PATH=...` antes del .bat).
Para distribuirlo en Pro Tools hace falta ademas firmarlo con las herramientas de PACE/Avid.

### Generar el .exe sin tener Windows (GitHub Actions)

1. Crea un repositorio privado en GitHub y subi esta carpeta completa (incluida `.github`).
2. En la pestana **Actions > Build installers > Run workflow**.
3. Al terminar (10-20 min) descarga `NovaVocal-Windows-Installer.exe` y `NovaVocal-macOS-Installer.pkg`
   desde *Artifacts*.

## Compilar en Windows

Requisitos: Visual Studio 2022 (C++), CMake e Inno Setup 6. Ejecutá `Installer\build_windows.bat`;
genera `dist\NovaVocal-Windows-Installer.exe` (instala el VST3 en `C:\Program Files\Common Files\VST3`).

## Cómo funciona Vocal Assist

1. Tocá el círculo (**Escuchar**) mientras suena la voz sola (hasta 12 s) y tocalo de nuevo (**Detener**),
   o usá **Cargar** para analizar un archivo de audio.
2. Se mide: nivel, rango dinámico, balance espectral (graves, medios, presencia, aire),
   frecuencia y fuerza de la sibilancia, registro de la voz y tonalidad.
3. **Aplicar** escribe los valores en todos los módulos (EQ, compresor con make-up automático,
   de-esser sobre la sibilancia real de esa voz, tonalidad/escala del autotune, Magic, Reverb y Delay).
   Todo queda editable; **Deshacer** vuelve al estado previo al último Aplicar.
4. El selector **Estilo** (Natural / Rap-Trap / Pop) sesga el resultado (más presencia y aire,
   más compresión, afinación más dura, más efectos).

Es un análisis de señal con reglas de mezcla, no una red neuronal. Los objetivos espectrales y
las reglas están en `Source/VocalAssist.h` (sección "decisiones") y se pueden ajustar o reemplazar
por un modelo entrenado (por ejemplo con ONNX Runtime) sin tocar el resto del plugin.

## Estructura

```
Source/DSP.h            Núcleo DSP sin JUCE: biquads, EQ, compresor, de-esser, magic, delay, YIN, autotune
Source/Reverb.h         Reverb con pre-delay y cortes
Source/VocalAssist.h    Captura + análisis en hilo aparte + generación de ajustes
Source/Params.h         Todos los parámetros (automatizables desde el DAW)
Source/PluginProcessor  Cadena de audio: Tune → EQ → Comp → De-esser → Magic → Delay → Reverb
Source/UI.h             Tema, perillas, medidores, selectores
Source/Panels.h         Ventanas por sección y las tres páginas
Source/PluginEditor     Cabecera, presets, A/B, deshacer
Installer/              Scripts de instalador (.pkg macOS, .exe Windows)
Tests/test_core.cpp     Test del núcleo DSP: g++ -std=c++17 -O2 Tests/test_core.cpp -o t && ./t
```

## Limitaciones conocidas

- El autotune es un pitch shifter de línea de retardo sincronizado al período de la voz: suena bien
  para correcciones moderadas y efecto "duro" tipo trap; para calidad de estudio con cambios grandes
  conviene reemplazarlo por PSOLA/phase vocoder (p. ej. Signalsmith Stretch o Rubber Band).
- El autotune no informa latencia al DAW (compensá a mano ~10 ms si lo usás en pistas ya grabadas).
- Detecta una sola voz (monofónica) y trabaja en mono/estéreo.
- JUCE se distribuye bajo AGPLv3 o licencia comercial: para vender el plugin sin abrir el código
  necesitás una licencia de JUCE.

## Personalizar

Nombre, códigos AU/VST3 y empresa: `CMakeLists.txt` (`PRODUCT_NAME`, `COMPANY_NAME`, `PLUGIN_CODE`).
Colores: `nv::theme` al principio de `Source/UI.h`. Logo: `NovaVocalAudioProcessorEditor::paint`.
