@echo off
rem Compila Nova Vocal (VST3, y AAX si definis AAX_SDK_PATH) y genera el instalador .exe
rem Requisitos: Visual Studio 2022 (carga de trabajo C++), CMake e Inno Setup 6
cd /d "%~dp0\.."
set "EXTRA="
if defined AAX_SDK_PATH set "EXTRA=-DAAX_SDK_PATH=%AAX_SDK_PATH%"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 %EXTRA% || goto :err
cmake --build build --config Release --target NovaVocal_VST3 NovaVocal_Standalone || goto :err
if defined AAX_SDK_PATH cmake --build build --config Release --target NovaVocal_AAX || goto :err
if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" (
  "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" Installer\NovaVocal.iss || goto :err
) else (
  echo No se encontro Inno Setup 6. Instalalo y volve a correr este script.
  goto :err
)
echo Listo: dist\NovaVocal-Windows-Installer.exe
exit /b 0
:err
echo Hubo un error.
exit /b 1
