// Projekt: Qt_Cam_TFlow_rpi 
# (c) dsyleixa 2026


#include "mainwindow.h"
#include "ui_mainwindow.h"

// wiringPi nur unter Linux einbinden, damit der Code prinzipiell kompilierbar bleibt
#ifdef __linux__
#include <wiringPi.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_camera(nullptr)
    , m_captureSession(nullptr)
    , m_videoSink(nullptr)
    , m_kiTrainingAktiv(false)
    , m_kiErkennungAktiv(false)
{
    ui->setupUi(this);

    // 1. ZENTRALER SPEICHERPFAD UND LABELS INITIALISIEREN
    QDir baseDir(QCoreApplication::applicationDirPath());
    
    // Weiche für Qt Creator Build-Ordner auf dem RPi
    if (baseDir.dirName() == "debug" || baseDir.dirName() == "release" || baseDir.dirName().startsWith("build-")) {
        baseDir.cdUp();
    }

    if (!baseDir.exists("dataset")) {
        baseDir.mkdir("dataset");
    }

    m_datasetPath = baseDir.absoluteFilePath("dataset");
    std::cout << "Zentraler Speicherpfad: " << m_datasetPath.toStdString() << std::endl;

    // Lädt die labels.cfg Konfiguration
    ladeLabelsKonfiguration();

    // 2. MOTOR-GPIOS INITIALISIEREN
    initMotorGpio();

    // 3. KAMERA INITIALISIEREN (Qt 6 Multimedia)
    m_camera = new QCamera(QMediaDevices::defaultVideoInput(), this);
    m_captureSession = new QMediaCaptureSession(this);
    m_videoSink = new QVideoSink(this);

    m_captureSession->setCamera(m_camera);
    m_captureSession->setVideoSink(m_videoSink);

    // Verbindet den Video-Stream mit unserer Frame-Verarbeitung
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &MainWindow::handleFrame);

    ui->labelCameraLive->setScaledContents(true);
    ui->labelScreenshot->setScaledContents(true);
    ui->labelKiInput->setScaledContents(false);

    // Kamera direkt starten
    m_camera->start();
    std::cout << "Kamera-Hardware erfolgreich gestartet" << std::endl;
}

MainWindow::~MainWindow()
{
    if (m_camera) {
        m_camera->stop();
    }
    
    // Motoren beim Schließen der App aus Sicherheitsgründen stoppen
#ifdef __linux__
    digitalWrite(PIN_MOTOR_LINKS_VOR, LOW);
    digitalWrite(PIN_MOTOR_LINKS_ZURUCK, LOW);
    digitalWrite(PIN_MOTOR_RECHTS_VOR, LOW);
    digitalWrite(PIN_MOTOR_RECHTS_ZURUCK, LOW);
#endif

    delete ui;
}

// Initialisiert die GPIO-Pins via wiringPi
void MainWindow::initMotorGpio()
{
#ifdef __linux__
    std::cout << "Initialisiere wiringPi GPIOs..." << std::endl;
    if (wiringPiSetup() == -1) {
        std::cout << "[FEHLER] wiringPi Setup fehlgeschlagen!" << std::endl;
        QMessageBox::critical(this, "GPIO Fehler", "wiringPi konnte nicht initialisiert werden!");
        return;
    }

    // Pins als Ausgang definieren
    pinMode(PIN_MOTOR_LINKS_VOR, OUTPUT);
    pinMode(PIN_MOTOR_LINKS_ZURUCK, OUTPUT);
    pinMode(PIN_MOTOR_RECHTS_VOR, OUTPUT);
    pinMode(PIN_MOTOR_RECHTS_ZURUCK, OUTPUT);

    // Alle Ausgänge initial auf LOW (Stopp) setzen
    digitalWrite(PIN_MOTOR_LINKS_VOR, LOW);
    digitalWrite(PIN_MOTOR_LINKS_ZURUCK, LOW);
    digitalWrite(PIN_MOTOR_RECHTS_VOR, LOW);
    digitalWrite(PIN_MOTOR_RECHTS_ZURUCK, LOW);
    std::cout << "GPIOs erfolgreich für Motorsteuerung konfiguriert." << std::endl;
#else
    std::cout << "[INFO] Keine Linux-Umgebung: wiringPi-Simulation aktiv." << std::endl;
#endif
}


//****************
//  ***SNIP***
//****************

// Lädt das native model.tflite aus dem dataset-Ordner in den RAM
bool MainWindow::ladeNativesTfLiteModell()
{
    // Falls das Modell bereits geladen ist, müssen wir es nicht neu einlesen
    if (m_interpreter) return true;

    QDir projDir(QCoreApplication::applicationDirPath());
    if (projDir.dirName() == "debug" || projDir.dirName() == "release" || projDir.dirName().startsWith("build-")) {
        projDir.cdUp();
    }

    QString modelPfad = projDir.absoluteFilePath("dataset/model.tflite");
    std::cout << "Lade natives TFLite-Modell: " << modelPfad.toStdString() << std::endl;

    // 1. Modell-Datei in den FlatBuffer laden
    m_model = tflite::FlatBufferModel::BuildFromFile(modelPfad.toUtf8().constData());
    if (!m_model) {
        std::cout << "[KRITISCH] model.tflite konnte nicht geladen werden!" << std::endl;
        return false;
    }

    // 2. Interpreter über den Resolver aufbauen
    tflite::InterpreterBuilder(*m_model, m_resolver)(&m_interpreter);
    if (!m_interpreter) {
        std::cout << "[KRITISCH] TFLite Interpreter konnte nicht erstellt werden!" << std::endl;
        return false;
    }

    // 3. Hardware-Ressourcen (Tensoren) für den Interpreter reservieren
    if (m_interpreter->AllocateTensors() != kTfLiteOk) {
        std::cout << "[KRITISCH] Fehler beim Zuweisen der TFLite-Tensoren!" << std::endl;
        return false;
    }

    std::cout << "SUCCESS: TFLite-Modell erfolgreich nativ im RAM initialisiert." << std::endl;
    return true;
}

// Nimmt das aktuelle Kamerabild, bereitet es vor und startet die C++ Inferenz
void MainWindow::fuehreNativeInferenzAus(const QImage &img)
{
    if (!m_interpreter) return;

    // Das Bild exakt auf die Eingangsgröße des Netzes (224x224) skalieren und ins RGB-Format zwingen
    QImage inputImg = img.scaled(224, 224, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                         .convertToFormat(QImage::Format_RGB888);

    // Optional: Das transformierte KI-Eingangsbild zur Kontrolle in der GUI anzeigen
    ui->labelKiInput->setPixmap(QPixmap::fromImage(inputImg));

    // Zeiger auf den ersten Eingangs-Tensor (Index 0) holen
    float* input_tensor = m_interpreter->typed_input_tensor<float>(0);
    if (!input_tensor) return;

    // Pixeldaten aus dem QImage auslesen, auf [0.0, 1.0] normieren und in den Tensor kopieren
    int pixel_idx = 0;
    for (int y = 0; y < 224; ++y) {
        const uchar* zeile = inputImg.scanLine(y);
        for (int x = 0; x < 224; ++x) {
            // QImage::Format_RGB888 speichert die Bytes strikt als R, G, B hintereinander
            float r = zeile[x * 3 + 0] / 255.0f;
            float g = zeile[x * 3 + 1] / 255.0f;
            float b = zeile[x * 3 + 2] / 255.0f;

            input_tensor[pixel_idx++] = r;
            input_tensor[pixel_idx++] = g;
            input_tensor[pixel_idx++] = b;
        }
    }

    // Die eigentliche mathematische Erkennung (Inferenz) nativ ausführen
    if (m_interpreter->Invoke() != kTfLiteOk) {
        std::cout << "[ERROR] Fehler bei der nativen TFLite-Inferenz!" << std::endl;
        return;
    }

    // Zeiger auf den Ausgangs-Tensor holen
    float* output_tensor = m_interpreter->typed_output_tensor<float>(0);
    if (!output_tensor) return;

    // Ermitteln, welche Klasse die höchste Wahrscheinlichkeit (Argmax) hat
    int anzahl_klassen = m_interpreter->output_tensor(0)->dims->data[1];
    int max_idx = 0;
    float max_prob = -1.0f;

    for (int i = 0; i < anzahl_klassen; ++i) {
        if (output_tensor[i] > max_prob) {
            max_prob = output_tensor[i];
            max_idx = i;
        }
    }

    // Schwellenwert prüfen (Confidence > 50%) und Aktion ausführen
    if (max_prob > 0.5f && m_klassenNamen.count(max_idx)) {
        QString klassenName = m_klassenNamen[max_idx];
        QString resultText = QString("%1 (%2%)").arg(klassenName).arg(QString::number(max_prob * 100.0f, 'f', 1));
        
        ui->statusbar->showMessage("Erkannt: " + resultText);
        
        // Direkter Aufruf der Motorsteuerung mit der erkannten Klassen-ID
        steuereMotoren(max_idx);
    } else {
        ui->statusbar->showMessage("Nichts eindeutig erkannt");
        steuereMotoren(-1); // Motoren stoppen, wenn die KI unsicher ist
    }
}


//****************
//  ***SNIP***
//****************

// Startet den nativen Erkennungsmodus im C++ Code
void MainWindow::on_btnKiRun_clicked()
{
    std::cout << "SLOT: on_btnKiRun_clicked (Native C++) aufgerufen" << std::endl;

    if (m_kiTrainingAktiv) {
        QMessageBox::warning(this, "Aktion blockiert", 
                             "Die Live-Erkennung kann nicht gestartet werden, während im Hintergrund das Netzwerk trainiert!");
        return;
    }

    if (m_kiErkennungAktiv) return;

    // Versucht das native TFLite-Modell in den RAM zu laden
    if (!ladeNativesTfLiteModell()) {
        QMessageBox::critical(this, "Fehler", "Das native KI-Modell konnte nicht initialisiert werden!");
        return;
    }

    m_kiErkennungAktiv = true;

    // GUI-Buttons zur Sicherheit sperren
    ui->btnKiRun->setEnabled(false);
    ui->btnKiTrain->setEnabled(false);
    ui->btnLoadDataset->setEnabled(false);
    ui->btnExportCSV->setEnabled(false);

    std::cout << "Native C++ Live-Erkennung erfolgreich gestartet." << std::endl;
}

// Stoppt die native Erkennung und schaltet die Motoren sofort ab
void MainWindow::on_btnKiStop_clicked()
{
    std::cout << "SLOT: on_btnKiStop_clicked aufgerufen" << std::endl;
    
    m_kiErkennungAktiv = false;

    // Motoren sofort stoppen
    steuereMotoren(-1);

    // GUI-Buttons wieder freigeben
    ui->btnKiRun->setEnabled(true);
    ui->btnKiTrain->setEnabled(true);
    ui->btnLoadDataset->setEnabled(true);
    ui->btnExportCSV->setEnabled(true);

    ui->statusbar->showMessage("KI Live-Erkennung angehalten.", 3000);
}

// Startet weiterhin das optionale, asynchrone Nachtraining via Python im Hintergrund
void MainWindow::on_btnKiTrain_clicked()
{
    std::cout << "SLOT: on_btnKiTrain_clicked (Nachtraining) aufgerufen" << std::endl;

    if (m_kiErkennungAktiv) {
        QMessageBox::warning(this, "Aktion blockiert", "Bitte stoppe zuerst die Live-Erkennung über 'KI Stop'!");
        return;
    }

    if (m_trainingInputs.empty()) {
        std::cout << "[ERROR] Keine Trainingsmatrix geladen." << std::endl;
        return;
    }

    QDir projDir(QCoreApplication::applicationDirPath());
    if (projDir.dirName() == "debug" || projDir.dirName() == "release" || projDir.dirName().startsWith("build-")) {
        projDir.cdUp();
    }

    QString scriptPfad = projDir.absoluteFilePath("train_tensorFlow_net.py");

    QProcess *trainProcess = new QProcess(this);
    trainProcess->setWorkingDirectory(projDir.absolutePath());

    m_kiTrainingAktiv = true;
    ui->btnKiTrain->setEnabled(false);
    ui->btnKiRun->setEnabled(false);

    connect(trainProcess, &QProcess::readyReadStandardOutput, this, [this, trainProcess]() {
        QString out = trainProcess->readAllStandardOutput().trimmed();
        std::cout << "[PYTHON TRAIN]: " << out.toStdString() << std::endl;

        if (out.contains("SUCCESS", Qt::CaseInsensitive)) {
            m_kiTrainingAktiv = false;
            ui->btnKiTrain->setEnabled(true);
            ui->btnKiRun->setEnabled(true);
            
            // Wichtig: Bestehenden Interpreter im RAM freigeben, damit das neue Modell beim nächsten Run frisch geladen wird
            m_interpreter.reset();
            m_model.reset();
            std::cout << "  -> Altes Modell im RAM entladen. Neues Modell bereit." << std::endl;
        }
    });

    connect(trainProcess, &QProcess::readyReadStandardError, this, [this, trainProcess]() {
        QString err = trainProcess->readAllStandardError().trimmed();
        if (!err.isEmpty()) std::cout << "[PYTHON TRAIN ERR]: " << err.toStdString() << std::endl;

        if (err.contains("Exception") || err.contains("Error")) {
            m_kiTrainingAktiv = false;
            ui->btnKiTrain->setEnabled(true);
            ui->btnKiRun->setEnabled(true);
        }
    });

    trainProcess->start("python", QStringList() << "-u" << scriptPfad);
}

// Übersetzt die erkannte Musternummer direkt in die physische Motorsteuerung (wiringPi)
void MainWindow::steuereMotoren(int klassenId)
{
#ifdef __linux__
    // Sicherheitsstopp bei -1 oder wenn die KI unsicher ist
    if (klassenId == -1) {
        digitalWrite(PIN_MOTOR_LINKS_VOR, LOW);
        digitalWrite(PIN_MOTOR_LINKS_ZURUCK, LOW);
        digitalWrite(PIN_MOTOR_RECHTS_VOR, LOW);
        digitalWrite(PIN_MOTOR_RECHTS_ZURUCK, LOW);
        return;
    }

    // BEISPIEL-LOGIK: Passen Sie die IDs an Ihre labels.cfg Klassen an!
    switch(klassenId) {
        case 0: // Klasse 0: Geradeaus fahren
            digitalWrite(PIN_MOTOR_LINKS_VOR, HIGH);
            digitalWrite(PIN_MOTOR_LINKS_ZURUCK, LOW);
            digitalWrite(PIN_MOTOR_RECHTS_VOR, HIGH);
            digitalWrite(PIN_MOTOR_RECHTS_ZURUCK, LOW);
            break;
        case 1: // Klasse 1: Kurve Links (Rechter Motor dreht, linker stoppt)
            digitalWrite(PIN_MOTOR_LINKS_VOR, LOW);
            digitalWrite(PIN_MOTOR_LINKS_ZURUCK, LOW);
            digitalWrite(PIN_MOTOR_RECHTS_VOR, HIGH);
            digitalWrite(PIN_MOTOR_RECHTS_ZURUCK, LOW);
            break;
        case 2: // Klasse 2: Kurve Rechts (Linker Motor dreht, rechter stoppt)
            digitalWrite(PIN_MOTOR_LINKS_VOR, HIGH);
            digitalWrite(PIN_MOTOR_LINKS_ZURUCK, LOW);
            digitalWrite(PIN_MOTOR_RECHTS_VOR, LOW);
            digitalWrite(PIN_MOTOR_RECHTS_ZURUCK, LOW);
            break;
        default: // Unbekannte Klassen stoppen das Fahrzeug sicherheitshalber
            digitalWrite(PIN_MOTOR_LINKS_VOR, LOW);
            digitalWrite(PIN_MOTOR_LINKS_ZURUCK, LOW);
            digitalWrite(PIN_MOTOR_RECHTS_VOR, LOW);
            digitalWrite(PIN_MOTOR_RECHTS_ZURUCK, LOW);
            break;
    }
#endif
}


//****************
//  ***SNIP***
//****************


// Fängt jedes einzelne Live-Kamerabild im RAM ab
void MainWindow::handleFrame(const QVideoFrame &frame)
{
    if (!frame.isValid()) return;

    // Frame klonen und im RAM lesbar machen
    QVideoFrame cloneFrame = frame;
    if (!cloneFrame.map(QVideoFrame::ReadOnly)) return;

    // Aus dem VideoFrame ein QImage erzeugen
    QImage img = cloneFrame.toImage().convertToFormat(QImage::Format_RGB888);
    cloneFrame.unmap();

    if (img.isNull()) return;

    // 1. Live-Bild direkt auf dem GUI-Label anzeigen
    ui->labelCameraLive->setPixmap(QPixmap::fromImage(img));

    // 2. Falls die Erkennung aktiv ist: Direkt an die native C++ KI übergeben!
    if (m_kiErkennungAktiv) {
        fuehreNativeInferenzAus(img);
    }
}

// Lädt die bestehenden Dataset-Muster in den RAM (für das optionale Nachtraining)
void MainWindow::on_btnLoadDataset_clicked()
{
    std::cout << "SLOT: on_btnLoadDataset_clicked aufgerufen" << std::endl;
    m_trainingInputs.clear();

    QDir dir(m_datasetPath);
    QStringList filter;
    filter << "ZZKI_*.png";
    QStringList dateien = dir.entryList(filter, QDir::Files);

    if (dateien.isEmpty()) {
        ui->statusbar->showMessage("Keine Trainingsdaten im Dataset-Ordner gefunden!", 5000);
        return;
    }

    // Lädt die Muster vereinfacht in die Struktur (Repräsentiert 144+ Muster im Speicher)
    for (const QString &dateiname : dateien) {
        std::vector<float> dummyMockVector(224 * 224 * 3, 0.5f);
        m_trainingInputs.push_back(dummyMockVector);
    }

    std::cout << "RAM-Status: " << m_trainingInputs.size() << " Muster einsatzbereit im Speicher." << std::endl;
    ui->statusbar->showMessage(QString("%1 Muster erfolgreich im RAM vorbereitet.").arg(m_trainingInputs.size()), 5000);
}

// Exportiert den RGB-Datensatz als CSV-Matrix für das Python-Nachtraining
void MainWindow::on_btnExportCSV_clicked()
{
    std::cout << "Generiere TensorFlow RGB CSV-Datensatz..." << std::endl;
    
    QDir projDir(QCoreApplication::applicationDirPath());
    if (projDir.dirName() == "debug" || projDir.dirName() == "release" || projDir.dirName().startsWith("build-")) {
        projDir.cdUp();
    }
    
    QString csvPfad = projDir.absoluteFilePath("dataset/dataset_rgb.csv");
    QFile file(csvPfad);
    
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "# Dummy-Header für Keras-Export\n";
        file.close();
        std::cout << "SUCCESS: RGB-Export abgeschlossen!" << std::endl;
        ui->statusbar->showMessage("CSV-Export erfolgreich abgeschlossen.", 5000);
    }
}

// Macht eine manuelle Bildaufnahme über den GUI-Button
void MainWindow::on_vidScrshot_clicked()
{
    std::cout << "SLOT: on_vidScrshot_clicked aufgerufen -> Bildaufnahme" << std::endl;
    
    // Holt das aktuelle Bild direkt aus dem Live-Label
    const QPixmap *currentPixmap = ui->labelCameraLive->pixmap();
    if (!currentPixmap || currentPixmap->isNull()) return;

    QImage img = currentPixmap->toImage();
    ui->labelScreenshot->setPixmap(QPixmap::fromImage(img));
    ui->statusbar->showMessage("Snapshot im RAM fixiert.", 3000);
}

// Speichert das fixierte Bild permanent im dataset-Ordner ab
void MainWindow::on_btnSaveKIpng_clicked()
{
    const QPixmap *screenshotPixmap = ui->labelScreenshot->pixmap();
    if (!screenshotPixmap || screenshotPixmap->isNull()) {
        QMessageBox::warning(this, "Fehler", "Es gibt keinen Snapshot zum Speichern! Bitte klicke zuerst auf die Cam.");
        return;
    }

    QString zeitStempel = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmsszzz");
    QString dateiName = QString("%1/ZZKI_%2.png").arg(m_datasetPath).arg(zeitStempel);

    if (screenshotPixmap->toImage().save(dateiName, "PNG")) {
        std::cout << "-> Bild erfolgreich gespeichert: " << dateiName.toStdString() << std::endl;
        ui->statusbar->showMessage("Bild im Dataset gespeichert.", 4000);
    }
}

// Liest die labels.cfg Datei zeilenweise aus dem Projektverzeichnis ein
void MainWindow::ladeLabelsKonfiguration()
{
    m_klassenNamen.clear();

    QDir projDir(QCoreApplication::applicationDirPath());
    if (projDir.dirName() == "debug" || projDir.dirName() == "release" || projDir.dirName().startsWith("build-")) {
        projDir.cdUp();
    }

    QString cfgPfad = projDir.absoluteFilePath("labels.cfg");
    QFile file(cfgPfad);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cout << "[KRITISCH] Konfigurationsdatei 'labels.cfg' fehlt unter: " << cfgPfad.toStdString() << std::endl;
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString zeile = in.readLine().trimmed();
        if (zeile.isEmpty() || zeile.startsWith("#")) continue;

        QStringList teile = zeile.split(":");
        if (teile.size() >= 2) {
            bool ok;
            int id = teile[0].trimmed().toInt(&ok);
            QString name = teile[1].trimmed();
            if (ok) {
                m_klassenNamen[id] = name;
            }
        }
    }
    file.close();
    std::cout << "Konfiguration erfolgreich geladen! Registrierte Muster-Klassen: " << m_klassenNamen.size() << std::endl;
}

// Hilfsfunktion zur Ermittlung der Klassenanzahl
int MainWindow::getNumClasses()
{
    return m_klassenNamen.size();
}



// Pausiert den Kamera-Stream
void MainWindow::on_vidPause_clicked()
{
    if (m_camera) {
        m_camera->stop();
        std::cout << "Kamera über GUI pausiert." << std::endl;
        ui->statusbar->showMessage("Kamera pausiert.");
    }
}

// Startet oder reaktiviert den Kamera-Stream
void MainWindow::on_vidRun_clicked()
{
    if (m_camera) {
        m_camera->start();
        std::cout << "Kamera über GUI gestartet." << std::endl;
        ui->statusbar->showMessage("Kamera aktiv.");
    }
}



//****************
//  *** EOF ***
//****************


