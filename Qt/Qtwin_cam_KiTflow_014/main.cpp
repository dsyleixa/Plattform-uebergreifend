#include "mainwindow.h"
#include <QApplication>
#include <windows.h> // Holt die echten Windows-Systembefehle
#include <iostream>

int main(int argc, char *argv[])
{
    // Zwingt Windows direkt im Betriebssystem-Kern, JETZT ein Terminal zu öffnen!
    AllocConsole();

    // Biegt die C++ Ströme hardcodiert auf das neue Fenster um
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    std::cout << "TEST: Wenn du das siehst, funktioniert die Konsole!" << std::endl;

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return QApplication::exec();
} // main
