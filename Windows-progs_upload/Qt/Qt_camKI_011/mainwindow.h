// changelog: Python Embedded KI Integration

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

// ========================================================
// Python Vorwärtsdeklaration
// (Python.h NICHT hier einbinden!)
// ========================================================
struct _object;
typedef _object PyObject;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

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

private:
    // =========================
    // Basis / Setup
    // =========================
    void ladeLabelsKonfiguration();

    // =========================
    // UI + Kamera
    // =========================
    Ui::MainWindow *ui = nullptr;

    QCamera *m_camera = nullptr;
    QImageCapture *m_imageCapture = nullptr;
    QMediaCaptureSession m_captureSession;
    QVideoSink *m_sink = nullptr;

    QImage m_currentImage;

    QString m_datasetPath;

    // =========================
    // Labels / Dataset
    // =========================
    std::map<int, QString> m_klassenNamen;

    std::vector<std::vector<float>> m_trainingInputs;
    std::vector<std::vector<float>> m_trainingTargets;

    // =========================
    // Training Status
    // =========================
    bool m_kiTrainingAktiv = false;

    // =========================
    // Python Embedded KI
    // =========================
    bool m_pythonInit = false;
    bool m_pythonReady = false;

    PyObject* m_pyModule = nullptr;
    PyObject* m_pyInferFunc = nullptr;

    // Python Lifecycle
    bool initPython();
    void shutdownPython();

    // Inference
    void runPythonInference(const std::vector<float>& input);
    void runPythonInferenceFromImage(const QImage &img);

    // =========================
    // Altbestand
    // =========================
    QProcess *m_kiProzess = nullptr;

    bool m_kiErkennungAktiv = false;
};

#endif // MAINWINDOW_H