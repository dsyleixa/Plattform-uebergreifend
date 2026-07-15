::  qt_create_proName_exe.bat
:: erstellt .exe file mit Namen des akt.Projekts
:: inkl. aller eingebundener Laufzeit-Bibliotheken

@echo off
setlocal enabledelayedexpansion

:: ==============================================================================
:: DEIN QT-PFAD IST HIER JETZT FEST INTEGRIERT:
:: ==============================================================================
set "QT_BIN_PATH=D:\Qt\6.11.1\mingw_64\bin"
:: ==============================================================================

:: 1. .pro-Dateinamen im aktuellen Ordner ermitteln
set "PRO_NAME="
for %%F in (*.pro) do (
    set "PRO_NAME=%%~nF"
)

if "%PRO_NAME%"=="" (
    for %%I in ("%cd%") do set "PRO_NAME=%%~nxI"
)

echo Projektname erkannt: %PRO_NAME%

:: ==============================================================================
:: 2. RADIKALE VEREINFACHUNG: Keine verschachtelten Klammern mehr
:: ==============================================================================
set "EXE_QUELLE="

:: Erst in debug schauen (Falls da was ist, wird es als Standard gesetzt)
if exist "debug\*.exe" (
    for %%E in (debug\*.exe) do set "EXE_QUELLE=%%E"
)

:: Danach in release schauen (Falls hier eine EXE ist, ueberschreibt sie die Debug-Version!)
if exist "release\*.exe" (
    for %%E in (release\*.exe) do set "EXE_QUELLE=%%E"
)

:: ==============================================================================
:: 3. Kopieren und umbenennen
:: ==============================================================================
if "%EXE_QUELLE%"=="" (
    echo FEHLER Keine .exe im release- oder debug-Ordner gefunden. Bitte zuerst in Qt kompilieren
    pause
    exit /b
)

echo Kopiere !EXE_QUELLE! nach %PRO_NAME%.exe
copy /y "!EXE_QUELLE!" "%PRO_NAME%.exe"

echo.
echo Starte automatische Qt-DLL-Sammlung fuer %PRO_NAME%.exe
if not exist "!QT_BIN_PATH!\windeployqt.exe" (
    echo FEHLER windeployqt.exe wurde unter !QT_BIN_PATH! nicht gefunden.
    echo Bitte pruefe den Pfad QT_BIN_PATH ganz oben in dieser Batch-Datei
    pause
    exit /b
)

"!QT_BIN_PATH!\windeployqt.exe" --no-translations --no-compiler-runtime "%PRO_NAME%.exe"
echo.
echo FERTIG Alle Qt6-DLLs wurden erfolgreich in diesen Ordner kopiert.

:: ==============================================================================
:: UEBERGABE AN DAS POWERSHELL-SKRIPT (KORRIGIERTER NAME)
:: ==============================================================================
echo.
echo Starte PowerShell-Skript zum Zusammenschmelzen (Enigma)
powershell -NoProfile -ExecutionPolicy Bypass -File "Enigma_build_boxed-exe.ps1"

pause
