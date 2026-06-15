@echo off
setlocal enabledelayedexpansion

:: Aktuellen Ordnernamen als Standardwert ermitteln
for %%I in ("%CD%") do set "CURR_DIR=%%~nxI"

:ASK_NAME
:: Trick: Schreibt den aktuellen Ordnernamen unsichtbar in den Tastaturpuffer
echo set WshShell = WScript.CreateObject^("WScript.Shell"^)^:WshShell.SendKeys "%CURR_DIR%" > "%temp%\suggest.vbs"
cscript //nologo "%temp%\suggest.vbs"
del "%temp%\suggest.vbs"

:: Die Eingabeaufforderung startet jetzt mit dem bereits eingetippten Namen
set "xxxx="
set /p "xxxx=Enter new project name: "

:: Falls die Eingabe komplett gelöscht wurde, Standardwert nehmen
if "%xxxx%"=="" set "xxxx=%CURR_DIR%"

echo your new project name: %xxxx%

:: Prüfen, ob der Zielordner bereits existiert
if exist "..\%xxxx%\" (
    echo name already exists
    goto ASK_NAME
)

:: Neuen Zielordner erstellen
mkdir "..\%xxxx%"

:: Nur die reinen Quell- und Projektdateien kopieren
copy *.cpp "..\%xxxx%\" >nul 2>&1
copy *.h "..\%xxxx%\" >nul 2>&1
copy *.ui "..\%xxxx%\" >nul 2>&1
copy *.pro "..\%xxxx%\" >nul 2>&1

:: kopiert ALLE .bat-Dateien in den neuen Ordner
copy *.bat "..\%xxxx%\" >nul 2>&1

:: kopiert ALLE .py-Dateien in den neuen Ordner
copy *.py  "..\%xxxx%\" >nul 2>&1

:: kopiert ALLE .cfg-Dateien in den neuen Ordner
copy *.cfg "..\%xxxx%\" >nul 2>&1

:: .pro.user Datei im Zielordner löschen
if exist "..\%xxxx%\*.pro.user" del "..\%xxxx%\*.pro.user"

:: Die .pro Datei passend umbenennen
for %%F in ("..\%xxxx%\*.pro") do (
    ren "%%F" "%xxxx%.pro"
)

:: ERZWINGT das Öffnen des neuen Ordners im Windows-Explorer
explorer.exe "..\%xxxx%"

echo.
echo === Kopieren erfolgreich! Neuer Ordner wurde geoeffnet. ===
timeout /t 2 >nul
