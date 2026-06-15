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

:: 2. Nach der .exe im debug- oder release-Ordner suchen
set "EXE_QUELLE="
if exist "debug\*.exe" (
    for %%E in (debug\*.exe) do set "EXE_QUELLE=%%E"
) else if exist "release\*.exe" (
    for %%E in (release\*.exe) do set "EXE_QUELLE=%%E"
)

:: 3. Kopieren und umbenennen
if not "%EXE_QUELLE%"=="" (
    echo Kopiere !EXE_QUELLE! nach %PRO_NAME%.exe...
    copy /y "!EXE_QUELLE!" "%PRO_NAME%.exe"
    
    echo.
    echo Starte automatische Qt-DLL-Sammlung fuer %PRO_NAME%.exe...
    if exist "!QT_BIN_PATH!\windeployqt.exe" (
        "!QT_BIN_PATH!\windeployqt.exe" --no-translations --no-compiler-runtime "%PRO_NAME%.exe"
        echo.
        echo FERTIG! Alle Qt6-DLLs wurden erfolgreich in diesen Ordner kopiert.
    ) else (
        echo.
        echo FEHLER: windeployqt.exe wurde unter !QT_BIN_PATH! nicht gefunden.
        echo Bitte pruefe den Pfad QT_BIN_PATH ganz oben in dieser Batch-Datei!
    )
) else (
    echo FEHLER: Keine .exe im debug- oder release-Ordner gefunden. Bitte zuerst in Qt kompilieren!
)

pause
