; ==============================================================================
;  Nova Vocal - asistente de instalacion para Windows (Inno Setup 6)
;  Pantallas: Bienvenida > Licencia > Componentes > Carpeta de instalacion > Listo > Instalando > Fin
; ==============================================================================
#define AppName "Nova Vocal"
#define AppVer  "1.0.0"
#define VstDir  "..\build\NovaVocal_artefacts\Release\VST3\Nova Vocal.vst3"
#define ExeFile "..\build\NovaVocal_artefacts\Release\Standalone\Nova Vocal.exe"
#define AaxDir  "..\build\NovaVocal_artefacts\Release\AAX\Nova Vocal.aaxplugin"

[Setup]
AppId={{8F3C2B1A-5D47-4C8E-9A61-3B7E2D0C4F19}
AppName={#AppName}
AppVersion={#AppVer}
AppPublisher=NovaAudio
; Carpeta que elige el usuario = carpeta donde se instala el plugin VST3
DefaultDirName={commoncf64}\VST3
AppendDefaultDirName=no
DirExistsWarning=no
UsePreviousAppDir=yes
DisableProgramGroupPage=yes
; El desinstalador va aparte para no ensuciar la carpeta de plugins
UninstallFilesDir={autopf}\NovaAudio\Nova Vocal
UninstallDisplayName={#AppName}
LicenseFile=licencia.txt
InfoAfterFile=leeme_fl_studio.txt
WizardStyle=modern
WizardImageFile=wizard_large.bmp
WizardSmallImageFile=wizard_small.bmp
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=NovaVocal-Windows-Installer
Compression=lzma2
SolidCompression=yes

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Messages]
WizardSelectDir=Carpeta de instalación del plugin
SelectDirDesc=¿Dónde querés instalar el plugin VST3?
SelectDirLabel3=Nova Vocal (VST3) se instalará en la siguiente carpeta.
SelectDirBrowseLabel=Hacé clic en Siguiente para continuar o en Examinar para elegir otra carpeta.%n%nFL Studio busca plugins en "C:\Program Files\Common Files\VST3". Si elegís otra ruta, agregala en FL Studio: Options > Manage plugins.

[Types]
Name: "full";    Description: "Instalación completa"
Name: "compact"; Description: "Solo el plugin VST3"
Name: "custom";  Description: "Personalizada"; Flags: iscustom

[Components]
Name: "vst3";       Description: "Plugin VST3 (FL Studio, Reaper, Cubase, Ableton...)"; Types: full compact custom; Flags: fixed
Name: "standalone"; Description: "Aplicación independiente (Standalone)"; Types: full
#if DirExists(AaxDir)
Name: "aax";        Description: "Plugin AAX (Pro Tools)"; Types: full
#endif

[Files]
Source: "{#VstDir}\*"; DestDir: "{app}\Nova Vocal.vst3"; Components: vst3; Flags: recursesubdirs ignoreversion
Source: "{#ExeFile}"; DestDir: "{autopf}\NovaAudio\Nova Vocal"; Components: standalone; Flags: ignoreversion
#if DirExists(AaxDir)
Source: "{#AaxDir}\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\Nova Vocal.aaxplugin"; Components: aax; Flags: recursesubdirs ignoreversion
#endif

[Icons]
Name: "{autoprograms}\Nova Vocal"; Filename: "{autopf}\NovaAudio\Nova Vocal\Nova Vocal.exe"; Components: standalone

[Run]
Filename: "{autopf}\NovaAudio\Nova Vocal\Nova Vocal.exe"; Description: "Abrir Nova Vocal (Standalone)"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{app}\Nova Vocal.vst3"
Type: filesandordirs; Name: "{commoncf64}\Avid\Audio\Plug-Ins\Nova Vocal.aaxplugin"
