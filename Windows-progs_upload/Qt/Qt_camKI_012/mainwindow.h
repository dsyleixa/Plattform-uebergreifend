#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QImageCapture>
#include <QVideoSink>
#include <QImage>
#include <QProcess>

#include <vector>
#include <map>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Ermittelt die dynamische Slot-Anzahl anhand des höchsten Keys in der geladenen Map + 1
    int getNumClasses() const;

private slots:
    void on_vidPause_clicked();
    void on_vidScrshot_clicked();
    void on_vidRun_clicked();
    void on_quitButton_released();
    void on_btnSaveKIpng_clicked();
    void on_btnLoadDataset_clicked();
    void on_btnExportCSV_clicked();

    void on_btnKiRun_clicked();
    void on_btnKiStop_clicked();
    void on_btnKiTrain_clicked();

    // Asynchrone Signal-Slots aus der .cpp-Pipeline
    void on_timer_snapshot();
    void readKiOutput();
    void readKiError();
    void readTrainOutput();

private:
    // Hilfsfunktion zum Einlesen der Konfigurationsdatei beim Start
    void ladeLabelsKonfiguration();

    // Private Klassenvariablen der GUI und Hardware-Steuerung
    Ui::MainWindow *ui;
    QCamera *m_camera = nullptr;
    QImageCapture *m_imageCapture = nullptr;
    QMediaCaptureSession m_captureSession;
    QVideoSink *m_sink = nullptr;
    QImage m_currentImage;

    QString m_datasetPath;

    // Dynamische ID-Tabelle (wird aus labels.cfg geladen)
    std::map<int, QString> m_klassenNamen;

    std::vector<std::vector<float>> m_trainingInputs;
    std::vector<std::vector<float>> m_trainingTargets;

    // Für die asynchrone Live-Erkennung über das RAM-Skript
    QProcess *m_kiProzess = nullptr;
    bool m_kiErkennungAktiv = false;

    // Status-Merker für das laufende Training
    bool m_kiTrainingAktiv = false;
};

#endif // MAINWINDOW_H
