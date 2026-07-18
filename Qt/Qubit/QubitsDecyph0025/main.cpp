// main.cpp

#include "mainwindow.h"
#include <QApplication>
#include <windows.h> // Holt die echten Windows-Systembefehle
#include <iostream>
#include <cstdio>    // Wichtig für std::freopen

int main(int argc, char *argv[])
{
    // 1. Zwingt Windows direkt im Betriebssystem-Kern, JETZT ein Terminal zu öffnen!
    AllocConsole();

    // NEU: Zwingt das Windows-Terminal im Kernel auf echte UTF-8 Codierung (Codepage 65001)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 2. unblockierbare Windows-Schnittstelle für C++ Ströme
    // Das biegt std::cout so fest an das Fenster, dass keine IDE es unterdrücken kann!
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);

    // 3. Pufferung komplett abschalten, damit der Text SOFORT erscheint
    std::setvbuf(stdout, NULL, _IONBF, 0);
    std::setvbuf(stderr, NULL, _IONBF, 0);
    std::ios_base::sync_with_stdio(true);

    std::cout << "TEST: Wenn du das siehst, funktioniert die Konsole mit Ö, Ä, Ü und ß!" << std::endl;

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return QApplication::exec();
} // main.cpp
