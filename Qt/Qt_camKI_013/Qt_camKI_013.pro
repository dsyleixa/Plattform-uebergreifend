# (c)  dsyleixa 2026

QT += widgets multimedia multimediawidgets

CONFIG += c++17 console

# Aktiviert zusätzlich zur GUI das echte, klassische Konsolenfenster
win32: CONFIG += console

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Schaltet veraltete Compiler-Warnungen stumm
win32-g++: QMAKE_CXXFLAGS += -Wno-deprecated-copy -Wno-deprecated-declarations

# Diese Linux-spezifischen Pfade und Bibliotheken werden
# NUR auf dem Raspberry Pi ausgeführt. Windows ignoriert sie komplett:

win32 {
    INCLUDEPATH += "D:/Akten/Programmierung/Windows-progs/cpp_libs"
    INCLUDEPATH += C:/Users/helmu/AppData/Local/Programs/Python/Python313/include
    LIBS += -LC:/Users/helmu/AppData/Local/Programs/Python/Python313/libs -lpython313
}

unix:!macx:!android: {
    INCLUDEPATH += /usr/local/include    

    LIBS += -L"/usr/local/lib"
    LIBS += -lwiringPi
    LIBS += -lpthread
}

win32-g++: QMAKE_LFLAGS += -Wl,-subsystem,console
win32-msvc*: QMAKE_LFLAGS += /SUBSYSTEM:CONSOLE
