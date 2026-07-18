# QubitsDecyph0021.pro

QT += widgets multimedia multimediawidgets

CONFIG += c++17 console

# Aktiviert zusaetzlich zur GUI das echte, klassische Konsolenfenster
win32: CONFIG += console

# Zwingt das Modul printsupport erst beim echten Kompilieren unter Windows rein
win32: QT += printsupport

# SHADOW-BUILD AUSHEBELN: Erzwingt die Generierung der ui_*.h Dateien direkt im Hauptordner
UI_DIR = .

# Alle lokalen und externen Quellcodedateien des Projekts
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    "D:/Akten/Programmierung/Windows_progs/cpp_libs/qcustomplot/qcustomplot.cpp"

# Alle lokalen und externen Headerdateien des Projekts
HEADERS += \
    mainwindow.h \
    "D:/Akten/Programmierung/Windows_progs/cpp_libs/qcustomplot/qcustomplot.h"

# BEIDE Designer-Formulare sind hier vollstaendig verankert
FORMS += \
    mainwindow.ui

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Schaltet veraltete Warnungen stumm und schuetzt vor dem "Datei zu gross"-Absturz
win32-g++: QMAKE_CXXFLAGS += -Wno-deprecated-copy -Wno-deprecated-declarations -O2 # -O1 vs. -O2

# Windows-spezifische Pfade fuer die Bibliotheken und Python
win32 {
    INCLUDEPATH += "D:/Akten/Programmierung/Windows_progs/cpp_libs/qcustomplot"
    INCLUDEPATH += "D:/Akten/Programmierung/Windows_progs/cpp_libs"
    INCLUDEPATH += C:/Users/helmu/AppData/Local/Programs/Python/Python313/include
    LIBS += -LC:/Users/helmu/AppData/Local/Programs/Python/Python313/libs -lpython313
}

# Linux-spezifische Pfade und Bibliotheken fuer den Raspberry Pi
unix:!macx:!android: {
    INCLUDEPATH += /usr/local/include
    LIBS += -L"/usr/local/lib"
    LIBS += -lwiringPi
    LIBS += -lpthread
}

# Zwingt Windows, das Konsolen-Subsystem fuer das Terminal scharfzuschalten
win32-g++: QMAKE_LFLAGS += -Wl,-subsystem,console
win32-msvc*: QMAKE_LFLAGS += /SUBSYSTEM:CONSOLE
