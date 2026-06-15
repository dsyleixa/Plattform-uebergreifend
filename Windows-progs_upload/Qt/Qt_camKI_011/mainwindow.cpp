// Qt_camKI_
//
// für Python:
// pip install tensorflow pandas numpy


// changelog
// 011
// 009  funktioniert mit beiden py scripts
// 008 KI Muster-Labels auslagern in labels.cfg
// 007 vereinfachte Speicher/Klassifizierungsmethode
// 006 (Zentraler globaler Pfad als 1 Datei), prepareTensorFlowInput,
// 005 Qt_camKI_005 Basis
// 005 Qt_cam005 als Sicherung für Qt_cam004, ab jetzt jetzt Qt_camKI_005
// 004 224x224 speichern unter "ZZ_"+alias
// 003 3.Fenster 224x224, KI-Graustufen + Autokontrast
// 002 3.Fenster für 224x224, code optim.
// 001 3.Fenster für 224x224
// 000 2 Fenster für 640x480

//*************************
// ***SNIP***
//*************************
// Qt_camKI_
//
// für Python:
// pip install tensorflow pandas numpy


// changelog
// 008 KI Muster-Labels auslagern in labels.cfg
// 007 vereinfachte Speicher/Klassifizierungsmethode
// 006 (Zentraler globaler Pfad als 1 Datei), prepareTensorFlowInput,
// 005 Qt_camKI_005 Basis
// 005 Qt_cam005 als Sicherung für Qt_cam004, ab jetzt jetzt Qt_camKI_005
// 004 224x224 speichern unter "ZZ_"+alias
// 003 3.Fenster 224x224, KI-Graustufen + Autokontrast
// 002 3.Fenster für 224x224, code optim.
// 001 3.Fenster für 224x224
// 000 2 Fenster für 640x480

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <windows.h>
#include <cstdio>
#include <algorithm>
#include <vector>

#include <QRegularExpression>
#include <QTimer>
#include <QElapsedTimer>

#include <QMediaDevices>
#include <QPixmap>
#include <QVideoFrame>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QInputDialog>
#include <QLayout>
#include <QTextStream>

using namespace std;









MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_camera(nullptr)
    , m_imageCapture(nullptr)
    , m_sink(nullptr)
    , m_kiProzess(nullptr)
{
    FreeConsole();
    AllocConsole();
    std::freopen("CONOUT$", "w", stdout);
    std::freopen("CONOUT$", "w", stderr);

    ui->setupUi(this);

    // Speicherpfad im Projektverzeichnis initialisieren
    QDir baseDir(QCoreApplication::applicationDirPath());
    baseDir.cdUp();
    baseDir.mkdir("dataset");
    m_datasetPath = baseDir.absoluteFilePath("dataset");
    cout << "Zentraler Speicherpfad: " << m_datasetPath.toStdString() << endl;

    // Lädt die labels.cfg direkt beim Starten des Programms in den RAM
    ladeLabelsKonfiguration();

    ui->labelCameraLive->setScaledContents(true);
    ui->labelScreenshot->setScaledContents(true);
    ui->labelKiInput->setScaledContents(false);

    // Kamera-Setup
    QCameraDevice standardCam = QMediaDevices::defaultVideoInput();
    if (standardCam.isNull()) {
        m_camera = new QCamera(this);
    } else {
        m_camera = new QCamera(standardCam, this);
    }

    m_imageCapture = new QImageCapture(this);

    // Format auf 640x480 erzwingen, falls verfügbar
    const QList<QCameraFormat> formats = standardCam.videoFormats();
    if (!formats.isEmpty()) {
        QCameraFormat selectedFormat = formats.first();
        for (const QCameraFormat &format : formats) {
            if (format.resolution().width() == 640 && format.resolution().height() == 480) {
                selectedFormat = format;
                break;
            }
        }
        m_camera->setCameraFormat(selectedFormat);
    }

    m_captureSession.setCamera(m_camera);
    m_captureSession.setImageCapture(m_imageCapture);
    m_sink = new QVideoSink(this);
    m_captureSession.setVideoOutput(m_sink);

    static QElapsedTimer kiIntervallTimer;
    kiIntervallTimer.start();

    connect(m_sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        QImage liveBild = frame.toImage().convertToFormat(QImage::Format_RGB32);
        if (!liveBild.isNull()) {
            ui->labelCameraLive->setPixmap(QPixmap::fromImage(liveBild));

            if (m_kiErkennungAktiv && m_kiProzess && m_kiProzess->state() == QProcess::Running) {
                if (kiIntervallTimer.elapsed() >= 100) {
                    kiIntervallTimer.restart();

                    QImage kiBild = prozessiereKiBild(liveBild);
                    QByteArray rawBytes(reinterpret_cast<const char*>(kiBild.bits()), 224 * 224);
                    QByteArray b64Text = rawBytes.toBase64();

                    m_kiProzess->write("DATA:" + b64Text + "\n");
                    m_kiProzess->waitForBytesWritten(1);
                }
            }
        }
    });

    connect(m_imageCapture, &QImageCapture::imageCaptured, this, [this](int id, const QImage &preview) {
        Q_UNUSED(id);
        if (preview.isNull()) return;

        m_currentImage = preview;
        ui->labelScreenshot->setPixmap(QPixmap::fromImage(m_currentImage));

        QImage kiBild = prozessiereKiBild(m_currentImage);
        ui->labelKiInput->setPixmap(QPixmap::fromImage(kiBild));
    });

    // Kamera leicht verzögert starten
    QTimer::singleShot(300, this, [this]() {
        if (m_camera) {
            m_camera->start();
            cout << "Kamera-Hardware erfolgreich gestartet: "
                 << m_camera->cameraFormat().resolution().width() << "x"
                 << m_camera->cameraFormat().resolution().height() << endl;
        }
    });

    ui->progressBar->setValue(0);

    // Verzeichnis erzwingen
    QDir projDir(m_datasetPath);
    projDir.cdUp();
    projDir.mkdir("tmp");

    QString absoluterScriptPfad = projDir.absoluteFilePath("tmp/generated_inference.py");
    QFile::remove(absoluterScriptPfad);

    cout << "  -> Schreibe Inferenz-Skript frisch unter: " << absoluterScriptPfad.toStdString() << endl;

    QFile kiFile(absoluterScriptPfad);
    if (kiFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&kiFile);
        out << "import sys\n"
               "import base64\n"
               "import numpy as np\n"
               "import tensorflow as tf\n"
               "import os\n\n"
               "def main():\n"
               "    base_dir = os.getcwd()\n"
               "    model_path = os.path.join(base_dir, 'dataset', 'model.tflite')\n"
               "    labels_path = os.path.join(base_dir, 'dataset', 'labels.txt')\n\n"
               "    if not os.path.exists(model_path) or not os.path.exists(labels_path):\n"
               "        print('RESULT: Fehler - model.tflite oder labels.txt fehlt!', flush=True)\n"
               "        return\n\n"
               "    labels = []\n"
               "    with open(labels_path, 'r', encoding='utf-8') as f:\n"
               "        for line in f:\n"
               "            line = line.strip()\n"
               "            if not line: continue\n"
               "            # Korrektur: Holt das Element aus der Liste und wendet ERST DANNERST strip() an\n"
               "            if ':' in line: line = line.split(':', 1)[1].strip()\n"
               "            elif '=' in line: line = line.split('=', 1)[1].strip()\n"
               "            labels.append(str(line))\n\n"
               "    interpreter = tf.lite.Interpreter(model_path=model_path)\n"
               "    interpreter.allocate_tensors()\n"
               "    input_details = interpreter.get_input_details()\n"
               "    output_details = interpreter.get_output_details()\n\n"
               "    while True:\n"
               "        line = sys.stdin.readline()\n"
               "        if not line: break\n"
               "        line = line.strip()\n"
               "        if line.startswith('DATA:'):\n"
               "            try:\n"
               "                b64_data = line[5:]\n"
               "                raw_bytes = base64.b64decode(b64_data)\n"
               "                input_data = np.frombuffer(raw_bytes, dtype=np.uint8).astype(np.float32)\n"
               "                input_data = input_data.reshape(1, 224, 224, 1) / 255.0\n"
               "                interpreter.set_tensor(input_details['index'], input_data)\n"
               "                interpreter.invoke()\n"
               "                output_data = interpreter.get_tensor(output_details['index'])\n"
               "                \n"
               "                wahrscheinlichkeiten = np.array(output_data).flatten()\n"
               "                ergebnisse = []\n"
               "                \n"
               "                for i in range(len(labels)):\n"
               "                    if i < len(wahrscheinlichkeiten):\n"
               "                        prob = wahrscheinlichkeiten[i]\n"
               "                        if float(prob) > 0.5:\n";

        // Prozentzeichen trennen für C++ Streams
        out << "                            ergebnisse.append(str(labels[i]) + ' (' + str(round(float(prob)*100, 1)) + '" << "%" << "')\n";

        out << "                if not ergebnisse:\n"
               "                    print('RESULT: Nichts erkannt', flush=True)\n"
               "                else:\n"
               "                    print('RESULT: ' + ', '.join(ergebnisse), flush=True)\n"
               "            except Exception as e:\n"
               "                print('RESULT: Fehler beim Dekodieren (' + str(e) + ')', flush=True)\n\n"
               "if __name__ == '__main__':\n"
               "    main()\n";
        kiFile.close();
    }

    // Hintergrund-Prozess für die KI initialisieren
    m_kiProzess = new QProcess(this);

    connect(m_kiProzess, &QProcess::readyReadStandardOutput, this, [this]() {
        QString ausgabe = m_kiProzess->readAllStandardOutput().trimmed();
        QStringList zeilen = ausgabe.split("\n");
        for (const QString &zeile : std::as_const(zeilen)) {
            if (zeile.startsWith("RESULT:")) {
                QString reinesErgebnis = zeile.mid(7).trimmed();
                ui->statusbar->showMessage("KI Live: " + reinesErgebnis);
            } else {
                cout << "[PYTHON DEBUG]: " << zeile.toStdString() << endl;
            }
        }
    });

    connect(m_kiProzess, &QProcess::readyReadStandardError, this, [this]() {
        QString fehler = m_kiProzess->readAllStandardError().trimmed();
        cout << "[PYTHON KI FEHLER]: " << fehler.toStdString() << endl;
    });
} // MainWindow::MainWindow





















MainWindow::~MainWindow()
{
    cout << "PROGRAMM-ENDE: Schließe Anwendungen..." << endl;
    if (m_kiProzess) {
        m_kiProzess->terminate();
        m_kiProzess->waitForFinished(3000);
    }
    if (m_camera) {
        m_camera->stop();
    }
    delete ui;
} // MainWindow::~MainWindow






//*************************
// ***SNIP***
//*************************

//*************************
// ***SNIP***
//*************************

void MainWindow::on_vidPause_clicked()
{
    cout << "SLOT: on_vidPause_clicked aufgerufen" << endl;
    if (m_camera) {
        m_camera->stop();
        cout << "  -> Kamera angehalten." << endl;
    }
} // MainWindow::on_vidPause_clicked

void MainWindow::on_vidScrshot_clicked()
{
    cout << "SLOT: on_vidScrshot_clicked aufgerufen" << endl;
    if (m_imageCapture) {
        m_imageCapture->capture();
        cout << "  -> Bildaufnahme-Befehl an Hardware gesendet." << endl;
    }
} // MainWindow::on_vidScrshot_clicked

void MainWindow::on_vidRun_clicked()
{
    cout << "SLOT: on_vidRun_clicked aufgerufen" << endl;
    if (m_camera && !m_camera->isActive()) {
        m_camera->start();
        cout << "  -> Kamera gestartet." << endl;
    }
} // MainWindow::on_vidRun_clicked

void MainWindow::on_quitButton_released()
{
    cout << "SLOT: on_quitButton_released aufgerufen -> Schließe App" << endl;
    close();
} // MainWindow::on_quitButton_released



//*************************
// ***SNIP***
//*************************

//*************************
// ***SNIP***
//*************************

void MainWindow::on_btnSaveKIpng_clicked()
{
    cout << "SLOT: on_btnSaveKIpng_clicked aufgerufen" << endl;
    if (m_currentImage.isNull()) {
        cout << "  -> FEHLER: Kein Bild zum Speichern vorhanden!" << endl;
        return;
    }

    QDir().mkpath("dataset");
    QString fileName = QString("dataset/ki_input_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    QImage scaledImg = m_currentImage.scaled(224, 224, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage grayImg = scaledImg.convertToFormat(QImage::Format_Grayscale8);

    if (grayImg.save(fileName, "PNG")) {
        cout << "  -> KI-Bild erfolgreich gespeichert: " << fileName.toStdString() << endl;
        ui->statusbar->showMessage("KI-Bild erfolgreich als PNG gespeichert.", 3000);
    } else {
        cout << "  -> FEHLER beim Speichern des PNG-Bildes!" << endl;
    }
} // MainWindow::on_btnSaveKIpng_clicked











int MainWindow::getNumClasses() const
{
    cout << "FUNC: getNumClasses aufgerufen" << endl;
    if (m_klassenNamen.empty()) {
        cout << "  -> Klassen-Map ist leer (0 Klassen)." << endl;
        return 0;
    }
    int num = m_klassenNamen.rbegin()->first + 1;
    cout << "  -> Ermittelte Anzahl der Klassen: " << num << endl;
    return num;
} // MainWindow::getNumClasses



// Liest die labels.cfg Datei zeilenweise aus dem Quellcode-Verzeichnis ein
void MainWindow::ladeLabelsKonfiguration()
{
    m_klassenNamen.clear();

    // Ermittelt den Pfad, wo die EXE liegt, und sucht die labels.cfg im Projektordner darüber
    QDir projDir(QCoreApplication::applicationDirPath());
    projDir.cdUp(); // Aus debug/release raus in den Hauptordner

    QString cfgPfad = projDir.absoluteFilePath("labels.cfg");
    QFile file(cfgPfad);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cout << "[KRITISCH] Konfigurationsdatei 'labels.cfg' fehlt unter: "
                  << cfgPfad.toStdString() << std::endl;
        QMessageBox::critical(this, "Fehler", "Die Datei 'labels.cfg' wurde im Projektordner nicht gefunden!");
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString zeile = in.readLine().trimmed();

        // Ignoriere Leerzeilen oder Kommentare (falls du mal welche einfügst)
        if (zeile.isEmpty() || zeile.startsWith("#")) continue;

        // Splitte die Zeile am Doppelpunkt (z.B. "21:Karte Bube")
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
    std::cout << "Konfiguration erfolgreich geladen! Registrierte Muster-Klassen: "
              << m_klassenNamen.size() << " (Slots benötigt: " << getNumClasses() << ")" << std::endl;
} // MainWindow::ladeLabelsKonfiguration





//*************************
// ***SNIP***
//*************************


void MainWindow::on_btnKiRun_clicked()
{
    cout << "SLOT: on_btnKiRun_clicked aufgerufen" << endl;
    ui->progressBar->setValue(0);

    if (m_kiErkennungAktiv) return;

    if (m_kiProzess->state() != QProcess::Running) {
        // ZWINGEND ERFORDERLICH: Setzt den Pfad eine Ebene über 'dataset',
        // damit os.getcwd() in Python exakt im Hauptordner landet!
        QDir projDir(m_datasetPath);
        projDir.cdUp();
        m_kiProzess->setWorkingDirectory(projDir.absolutePath());

        cout << "  -> Setze KI-Arbeitsverzeichnis: " << projDir.absolutePath().toStdString() << endl;

        QStringList args;
        args << "-u" << "tmp/generated_inference.py";
        m_kiProzess->start("python", args);

        if (!m_kiProzess->waitForStarted()) {
            std::cout << "[ERROR] KI-Prozess konnte nicht gestartet werden!" << std::endl;
            return;
        }
    }

    m_kiErkennungAktiv = true;
    std::cout << "KI Live-Erkennung erfolgreich gestartet." << std::endl;
}
// MainWindow::on_btnKiRun_clicked






void MainWindow::on_btnKiStop_clicked()
{
    cout << "SLOT: on_btnKiStop_clicked aufgerufen" << endl;
    if (m_kiProzess && m_kiProzess->state() == QProcess::Running) {
        m_kiProzess->terminate();
        m_kiProzess->waitForFinished(2000);
    }
    m_kiErkennungAktiv = false;
    cout << "  -> KI Live-Erkennung angehalten." << endl;
    ui->statusbar->showMessage("KI-Inferenz angehalten.", 3000);
} // MainWindow::on_btnKiStop_clicked





void MainWindow::on_btnLoadDataset_clicked()
{

    ui->progressBar->setValue(0);


    m_trainingInputs.clear();
    m_trainingTargets.clear();

    ui->progressBar->setValue(0);

    int maxSlots = getNumClasses(); // Dynamische Anzahl ermitteln
    QDir datasetDir(m_datasetPath);
    QStringList filter;
    filter << "ZZKI_*.png";
    QStringList fileList = datasetDir.entryList(filter, QDir::Files);

    if (fileList.isEmpty()) {
        cout << "Fehler: Keine Trainingsdaten mit Maske 'ZZKI_*.png' im Dataset-Ordner gefunden!" << endl;
        return;
    }

    cout << "Lese " << fileList.size() << " Multi-Label Bilder in den RAM ein..." << endl;

    QRegularExpression re("_ID(\\d+)(?=[_\\.])");

    for (const QString &fileName : std::as_const(fileList)) {
        std::vector<float> targetVector(maxSlots, 0.0f); // Nutzt maxSlots statt NUM_CLASSES
        bool idGefunden = false;

        QRegularExpressionMatchIterator it = re.globalMatch(fileName);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int classId = match.captured(1).toInt();

            if (classId >= 0 && classId < maxSlots) { // nutzt maxSlots statt NUM_CLASSES
                targetVector[classId] = 1.0f;
                idGefunden = true;
            }
        }

        if (!idGefunden) {
            cout << "Uebersprungen (Keine gueltige ID): " << fileName.toStdString() << endl;
            continue;
        }

        QImage img(datasetDir.absoluteFilePath(fileName));
        if (img.isNull()) continue;

        if (img.format() != QImage::Format_Grayscale8) {
            img = img.convertToFormat(QImage::Format_Grayscale8);
        }

        std::vector<float> inputVector;
        inputVector.reserve(224 * 224);

        for (int y = 0; y < 224; ++y) {
            const uchar *row = img.scanLine(y);
            for (int x = 0; x < 224; ++x) {
                inputVector.push_back(static_cast<float>(row[x]) / 255.0f);
            }
        }

        m_trainingInputs.push_back(inputVector);
        m_trainingTargets.push_back(targetVector);
    }

    cout << "RAM-Status: " << m_trainingInputs.size() << " Muster einsatzbereit im Speicher (Slots: " << maxSlots << ")." << endl;
    // Zeigt den aktuellen RAM-Status für 4 Sekunden in der GUI an
    ui->statusbar->showMessage(QString("Erfolgreich geladen: %1 Bilder im RAM (Slots: %2)")
                                   .arg(m_trainingInputs.size()).arg(maxSlots), 4000);

}
//  on_btnLoadDataset_clicked



void MainWindow::on_btnExportCSV_clicked()
{

    ui->progressBar->setValue(0);

    int maxSlots = getNumClasses(); // Dynamische Anzahl ermitteln
    if (m_trainingInputs.empty() || m_trainingTargets.empty() || (m_trainingInputs.size() != m_trainingTargets.size())) {
        cout << "Fehler: Datensatz asynchron oder leer! Export abgebrochen." << endl;
        return;
    }

    cout << "Generiere TensorFlow CSV-Datensatz..." << endl;
    ui->progressBar->setValue(0);

    QString exportPath = m_datasetPath + "/tensorflow_dataset.csv";
    QFile file(exportPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        cout << "Schreibfehler auf Festplatte bei: " << exportPath.toStdString() << endl;
        return;
    }

    QTextStream out(&file);
    int totalFiles = m_trainingInputs.size();

    // CSV Header schreiben
    for (int i = 0; i < 224 * 224; ++i) {
        out << "pixel_" << i << ",";
    }
    for (int i = 0; i < maxSlots; ++i) { // Nutzt maxSlots statt NUM_CLASSES
        out << "target_" << i << (i == maxSlots - 1 ? "" : ",");
    }
    out << "\n";

    // Daten zeilenweise streamen
    for (int i = 0; i < totalFiles; ++i) {
        const std::vector<float>& currentInput = m_trainingInputs[i];
        for (float val : currentInput) {
            out << val << ",";
        }

        const std::vector<float>& currentTarget = m_trainingTargets[i];
        for (int t = 0; t < maxSlots; ++t) { // Nutzt maxSlots statt NUM_CLASSES
            out << currentTarget[t] << (t == maxSlots - 1 ? "" : ",");
        }
        out << "\n";

        ui->progressBar->setValue(static_cast<int>(((i + 1) * 100) / totalFiles));
        QCoreApplication::processEvents();
    }

    file.close();
    cout << "SUCCESS: Export abgeschlossen! " << totalFiles << " Bilder in CSV geschrieben." << endl;
    // Zeigt den erfolgreichen Export für 5 Sekunden in der GUI an
    ui->statusbar->showMessage(QString("SUCCESS: %1 Bilder erfolgreich in CSV exportiert!")
                                   .arg(totalFiles), 5000);

}
//  on_btnExportCSV_clicked





void MainWindow::on_btnKiTrain_clicked()
{
    std::cout << "SLOT: on_btnKiTrain_clicked aufgerufen" << std::endl;
    ui->progressBar->setValue(0);

    if (m_trainingInputs.empty()) {
        std::cout << "[ERROR] Trainings-Matrix ist leer! Bitte zuerst 'Export CSV' ausführen." << std::endl;
        return;
    }

    QDir projDir(QCoreApplication::applicationDirPath());
    projDir.cdUp();
    QString scriptPfad = projDir.absoluteFilePath("train_tensorFlow_net.py");

    QProcess *trainProcess = new QProcess(this);
    trainProcess->setWorkingDirectory(projDir.absolutePath());

    connect(trainProcess, &QProcess::readyReadStandardOutput, this, [this, trainProcess]() {
        QString out = trainProcess->readAllStandardOutput().trimmed();
        std::cout << "[PYTHON TRAIN]: " << out.toStdString() << std::endl;
    });

    connect(trainProcess, &QProcess::readyReadStandardError, this, [trainProcess]() {
        QString err = trainProcess->readAllStandardError().trimmed();
        if (!err.isEmpty()) {
            std::cout << "[PYTHON TRAIN ERR]: " << err.toStdString() << std::endl;
        }
    });

    trainProcess->start("python", QStringList() << "-u" << scriptPfad);
    std::cout << "Trainings-Skript wurde asynchron gestartet..." << std::endl;
} // MainWindow::on_btnKiTrain_clicked




void MainWindow::on_timer_snapshot()
{
    // Snapshot-Intervall-Triggerslot aus dem Header
} // MainWindow::on_timer_snapshot



void MainWindow::readKiOutput()
{
    // Erledigt über den Lambda-Slot im Konstruktor
} // MainWindow::readKiOutput

void MainWindow::readKiError()
{
    // Erledigt über den Lambda-Slot im Konstruktor
} // MainWindow::readKiError

void MainWindow::readTrainOutput()
{
    // Slot zum Mitlesen eines Trainingsprozesses falls gekoppelt
} // MainWindow::readTrainOutput

QImage MainWindow::prozessiereKiBild(const QImage &quellBild)
{
    // Wandelt das übergebene Bild in das exakte KI-Eingangsformat (224x224, Graustufen) um
    QImage scaledImg = quellBild.scaled(224, 224, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage grayImg = scaledImg.convertToFormat(QImage::Format_Grayscale8);
    return grayImg;
} // MainWindow::prozessiereKiBild

//*************************
// EOF
//*************************




