# Project Qt_Cam_Tflow_rpi


# Project Qt_Cam_Tflow_rpi

QT += core gui widgets multimedia

# TARGET = GUI_app   #historisch, deprecated

CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS

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

# RASPBERRY PI HARDWARE- & KI-PFADE
unix:!macx:!android {
    INCLUDEPATH += /usr/local/include
    
    # TensorFlow Header-Pfade (Zwingend nötig für C++)
    INCLUDEPATH += /home/pi/tensorflow_src
    INCLUDEPATH += /home/pi/tensorflow_src/tensorflow/lite/tools/make/downloads/flatbuffers/include
    INCLUDEPATH += /home/pi/tensorflow_src/tensorflow/lite/tools/make/downloads/absl

    LIBS += -L"/usr/local/lib"
    LIBS += -lwiringPi
    LIBS += -lpthread
    
    # Ersetzt das einfache -ltensorflow-lite durch den echten Pfad der kompilierten Library:
    LIBS += /home/pi/tensorflow_src/tflite_build/libtensorflow-lite.a
}


