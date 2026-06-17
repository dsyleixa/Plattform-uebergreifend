#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <map>
#include <vector>
#include <iostream>

// QT MULTIMEDIA FÜR DIE KAMERA
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>

// NATIVE TENSORFLOW LITE C++ API HEADER
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Vorhandene Slots für GUI und Dataset-Handling
    void on_btnLoadDataset_clicked();
    void on_btnExportCSV_clicked();
    void on_btnSaveKIpng_clicked();
    void on_vidScrshot_clicked();
    
    // Slots für KI-Steuerung (Jetzt nativ!)
    void on_btnKiTrain_clicked(); // Startet weiterhin asynchron das Python-Nachtraining
    void on_btnKiRun_clicked();   // Startet jetzt die blitzschnelle native C++ Erkennung
    void on_btnKiStop_clicked();  // Stoppt die native Erkennung
    void on_vidPause_clicked();
    void on_vidRun_clicked();
    
    // Verarbeitet die Live-Kamerabilder
    void handleFrame(const QVideoFrame &frame);

private:
    Ui::MainWindow *ui;

    // Kamera-Objekte
    QCamera *m_camera;
    QMediaCaptureSession *m_captureSession;
    QVideoSink *m_videoSink;

    // Status-Flags
    bool m_kiTrainingAktiv;
    bool m_kiErkennungAktiv;

    // Pfade und Konfiguration
    QString m_datasetPath;
    std::map<int, QString> m_klassenNamen;
    
    // Speicherstruktur für Dataset-Bilder
    std::vector<std::vector<float>> m_trainingInputs;

    // ==========================================
    // NATIVE KI-VARIABLEN (TFLITE C++)
    // ==========================================
    std::unique_ptr<tflite::FlatBufferModel> m_model;
    std::unique_ptr<tflite::Interpreter> m_interpreter;
    tflite::ops::builtin::BuiltinOpResolver m_resolver;
    
    // Hilfsfunktionen für die native Erkennung
    bool ladeNativesTfLiteModell();
    void fuehreNativeInferenzAus(const QImage &img);

    // ==========================================
    // MOTORSTEUERUNG (WIRINGPI)
    // ==========================================
    void initMotorGpio();
    void steuereMotoren(int klassenId);

    // GPIO-Pin Definitionen (wiringPi Nummerierung!)
    // Passen Sie diese Nummern an Ihre echte Verkabelung an
    const int PIN_MOTOR_LINKS_VOR  = 0;  // wPi Pin 0 (GPIO 17)
    const int PIN_MOTOR_LINKS_ZURUCK = 1; // wPi Pin 1 (GPIO 18)
    const int PIN_MOTOR_RECHTS_VOR = 2;  // wPi Pin 2 (GPIO 27)
    const int PIN_MOTOR_RECHTS_ZURUCK = 3; // wPi Pin 3 (GPIO 22)

    // Interne Kern-Funktionen aus Ihrem Windows-Code
    void ladeLabelsKonfiguration();
    int getNumClasses();
};

#endif // MAINWINDOW_H
