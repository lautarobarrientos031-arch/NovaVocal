# Cómo obtener el instalador .exe (sin instalar nada en tu PC)

El `.exe` hay que compilarlo en una máquina Windows. GitHub te presta una gratis. Son 5 pasos:

1. Creá una cuenta en https://github.com (gratis) y un repositorio nuevo **privado**: botón **New**, nombre `nova-vocal`.
2. En el repositorio: **Add file → Upload files**. Descomprimí `NovaVocal.zip` y arrastrá **todo el contenido**
   de la carpeta `NovaVocal` (Source, Installer, Tests, CMakeLists.txt, README.md, etc.). Tocá **Commit changes**.
3. Si no aparece una pestaña con el workflow en **Actions**, es porque la carpeta oculta `.github` no se subió:
   **Add file → Create new file**, escribí como nombre `.github/workflows/build.yml`, pegá el contenido de
   `CI_build.yml` y tocá **Commit changes**.
4. Entrá a **Actions → Build installers** (arranca solo; si no, **Run workflow**). Tarda 10 a 20 minutos.
   Si sale una cruz roja, abrí el paso que falló y mandame el texto del error: lo corrijo.
5. Cuando termine con tilde verde, abrí la ejecución y bajá de **Artifacts**:
   - `NovaVocal-Windows-Installer` → el asistente `.exe` (Siguiente, ruta de instalación, etc.)
   - `NovaVocal-macOS-Installer` → el `.pkg` para Mac

Windows puede mostrar "SmartScreen: editor desconocido" porque el instalador no está firmado:
tocá **Más información → Ejecutar de todas formas**.
