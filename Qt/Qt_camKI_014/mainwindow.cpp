// Qt_camKI_
// (c)  dsyleixa  2026
// Multi-Label-Erkennung
//
// für Python-Funktionen:
// pip install tensorflow pandas numpy


// changelog
// 014 neue zusätzlich Pfade für .exe
// 013 Farbkanäle RGB
// 012 ok mit run_inference code intern, train_tensorFlow_net.py ext
// 009 ok scripts: train_tensorFlow_net.py + run_inference.py
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



QImage prozessiereKiBild(const QImage &quellBild)  //  (RGB)
{
    if (quellBild.isNull()) return QImage();

    int w = quellBild.width();
    int h = quellBild.height();
    int minDim = std::min(w, h);
    int startX = (w - minDim) / 2;
    int startY = (h - minDim) / 2;

    // 1. Quadratischer Zuschnitt aus der Mitte
    QImage quadrat = quellBild.copy(startX, startY, minDim, minDim);

    // 2. Skalierung auf TensorFlow-Standardgröße (RGB32 Format erzwingen!)
    QImage kiBild = quadrat.scaled(224, 224, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    kiBild = kiBild.convertToFormat(QImage::Format_RGB32);

    // 3. Autokontrast für alle 3 Farbkanäle getrennt (Min/Max-Normalisierung)
    int minR = 255, maxR = 0;
    int minG = 255, maxG = 0;
    int minB = 255, maxB = 0;

    for (int y = 0; y < kiBild.height(); ++y) {
        const QRgb *row = reinterpret_cast<const QRgb*>(kiBild.scanLine(y));
        for (int x = 0; x < kiBild.width(); ++x) {
            // extrahiert aus 32bit Farbe (αRGB) die Einzel-R,G,B
            int r = qRed(row[x]);
            int g = qGreen(row[x]);
            int b = qBlue(row[x]);
            //  ersetzen von minX, maxX durch evt noch dunklere/hellere Werte
            if (r < minR) minR = r; if (r > maxR) maxR = r;
            if (g < minG) minG = g; if (g > maxG) maxG = g;
            if (b < minB) minB = b; if (b > maxB) maxB = b;
        }
    }

    // Normalisierung zurückschreiben
    for (int y = 0; y < kiBild.height(); ++y) {
        QRgb *row = reinterpret_cast<QRgb*>(kiBild.scanLine(y));
        for (int x = 0; x < kiBild.width(); ++x) {
            int r = qRed(row[x]);
            int g = qGreen(row[x]);
            int b = qBlue(row[x]);

            // Autokontrast
            // dunkelster Wert->schwarz; Rest *255/echte Breite des Farbspektrums (maxR - minR)
            // => hellste Pixel (maxR/G/B) landen exakt auf Maximalwert 255 (=> Rein-Weiß).
            if (maxR > minR) r = ((r - minR) * 255) / (maxR - minR);
            if (maxG > minG) g = ((g - minG) * 255) / (maxG - minG);
            if (maxB > minB) b = ((b - minB) * 255) / (maxB - minB);

            // setze R,G,B wieder zu 32bit zusammen
            row[x] = qRgb(r, g, b);
        }
    }
    return kiBild;
} // prozessiereKiBild //  (RGB)





/* alte Version 13 mit Pfaden nur für Qt-GUI-Umgebung

void erstelleInferenzSkriptDatei(const QString &datasetPath)
{
    QDir projDir(datasetPath);
    projDir.cdUp();
    projDir.mkdir("tmp");

    QString absoluterScriptPfad = projDir.absoluteFilePath("tmp/generated_inference.py");
    QFile::remove(absoluterScriptPfad);

    cout << "  -> Globale Funktion: Generiere Inferenz-Skript unter: " << absoluterScriptPfad.toStdString() << endl;

    QFile kiFile(absoluterScriptPfad);
    if (kiFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&kiFile);
        out << "import sys\n"
               "import base64\n"
               "import numpy as np\n"
               "import tensorflow as tf\n"
               "import os\n"
               "import logging\n\n"

               "def main():\n"
               "    tf.get_logger().setLevel(logging.WARNING)\n"
               "    logging.basicConfig(stream=sys.stdout, level=logging.WARNING)\n\n"

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
               "            if ':' in line: line = line.split(':', 1)[-1].strip()\n"
               "            elif '=' in line: line = line.split('=', 1)[-1].strip()\n"
               "            labels.append(str(line))\n\n"
               "    interpreter = tf.lite.Interpreter(model_path=model_path)\n"
               "    interpreter.allocate_tensors()\n"
               "    \n"
               "    in_details = interpreter.get_input_details()\n"
               "    out_details = interpreter.get_output_details()\n"
               "    \n"
               "    if isinstance(in_details, list):\n"
               "        input_index = in_details[0]['index']\n"
               "    else:\n"
               "        first_key = list(in_details.keys())[0]\n"
               "        input_index = in_details[first_key]['index']\n"
               "        \n"
               "    if isinstance(out_details, list):\n"
               "        output_index = out_details[0]['index']\n"
               "    else:\n"
               "        first_key = list(out_details.keys())[0]\n"
               "        output_index = out_details[first_key]['index']\n\n"
               "    while True:\n"
               "        raw_line = sys.stdin.buffer.readline()\n"
               "        if not raw_line: break\n"
               "        try:\n"
               "            line = raw_line.decode('utf-8', errors='ignore').strip()\n"
               "        except Exception:\n"
               "            continue\n"
               "            \n"
               "        if line.startswith('DATA:'):\n"
               "            try:\n"
               "                b64_data = line[5:]\n"
               "                raw_bytes = base64.b64decode(b64_data)\n"
               "                input_data = np.frombuffer(raw_bytes, dtype=np.uint8).astype(np.float32)\n"
               "                \n"
               "                # RGB-ANPASSUNG: Ändert die letzte Dimension von 1 auf 3 Farbkanäle (R, G, B)\n"
               "                input_data = input_data.reshape(1, 224, 224, 3) / 255.0\n"
               "                \n"
               "                interpreter.set_tensor(input_index, input_data)\n"
               "                interpreter.invoke()\n"
               "                output_data = interpreter.get_tensor(output_index)\n"
               "                \n"
               "                flat_output = output_data.flatten()\n"
               "                max_idx = int(np.argmax(flat_output))\n"
               "                prob = float(flat_output[max_idx])\n"
               "                \n"
               "                if prob > 0.5 and 0 <= max_idx < len(labels):\n"
               "                    label_text = str(labels[max_idx])\n";

        // Prozentzeichen isoliert wegschreiben
        out << "                    print('RESULT: ' + label_text + ' (' + str(round(prob*100, 1)) + '" << "%" << ")', flush=True)\n";

        out << "                else:\n"
               "                    print('RESULT: Nichts erkannt', flush=True)\n"
               "            except Exception as e:\n"
               "                print('RESULT: Fehler bei Inferenz (' + str(e) + ')', flush=True)\n\n"
               "if __name__ == '__main__':\n"
               "    main()\n";
        kiFile.close();
    }
}
//  erstelleInferenzSkriptDatei

*/



    //  neue Version mit angepassten Pfaden für GUI und per .exe
    void erstelleInferenzSkriptDatei(const QString &datasetPath)
{
    Q_UNUSED(datasetPath); // <-- Verhindert die Compiler-Warnung vollständig  // neue Pfad-Erstellung

    // Nutzen der intelligenten Weiche direkt über das Anwendungs-Verzeichnis
    QDir projDir(QCoreApplication::applicationDirPath());
    if (projDir.dirName() == "debug" || projDir.dirName() == "release") {
        projDir.cdUp();
    }

    // Erstellt den tmp-Ordner im Hauptverzeichnis, falls er noch fehlt
    if (!projDir.exists("tmp")) {
        projDir.mkdir("tmp");
    }

    QString absoluterScriptPfad = projDir.absoluteFilePath("tmp/generated_inference.py");
    QFile::remove(absoluterScriptPfad);

    cout << "  -> Globale Funktion: Generiere Inferenz-Skript unter: " << absoluterScriptPfad.toStdString() << endl;

    QFile kiFile(absoluterScriptPfad);
    if (kiFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&kiFile);
        out << "import sys\n"
               "import base64\n"
               "import numpy as np\n"
               "import tensorflow as tf\n"
               "import os\n"
               "import logging\n\n"

               "def main():\n"
               "    tf.get_logger().setLevel(logging.WARNING)\n"
               "    logging.basicConfig(stream=sys.stdout, level=logging.WARNING)\n\n"

               "    base_dir = os.getcwd()\n"
               "    # KORREKTUR: Da Python direkt im dataset-Ordner startet, liegen die Dateien direkt im base_dir\n"
               "    model_path = os.path.join(base_dir, 'model.tflite')\n"
               "    labels_path = os.path.join(base_dir, 'labels.txt')\n\n"
               "    if not os.path.exists(model_path) or not os.path.exists(labels_path):\n"
               "        print('RESULT: Fehler - model.tflite oder labels.txt fehlt!', flush=True)\n"
               "        return\n\n"
               "    labels = []\n"
               "    with open(labels_path, 'r', encoding='utf-8') as f:\n"
               "        for line in f:\n"
               "            line = line.strip()\n"
               "            if not line: continue\n"
               "            if ':' in line: line = line.split(':', 1)[-1].strip()\n"
               "            elif '=' in line: line = line.split('=', 1)[-1].strip()\n"
               "            labels.append(str(line))\n\n"
               "    interpreter = tf.lite.Interpreter(model_path=model_path)\n"
               "    interpreter.allocate_tensors()\n"
               "    \n"
               "    in_details = interpreter.get_input_details()\n"
               "    out_details = interpreter.get_output_details()\n"
               "    \n"
               "    if isinstance(in_details, list):\n"
               "        input_index = in_details[0]['index']\n"
               "    else:\n"
               "        first_key = list(in_details.keys())[0]\n"
               "        input_index = in_details[first_key]['index']\n"
               "        \n"
               "    if isinstance(out_details, list):\n"
               "        output_index = out_details[0]['index']\n"
               "    else:\n"
               "        first_key = list(out_details.keys())[0]\n"
               "        output_index = out_details[first_key]['index']\n\n"
               "    while True:\n"
               "        raw_line = sys.stdin.buffer.readline()\n"
               "        if not raw_line: break\n"
               "        try:\n"
               "            line = raw_line.decode('utf-8', errors='ignore').strip()\n"
               "        except Exception:\n"
               "            continue\n"
               "            \n"
               "        if line.startswith('DATA:'):\n"
               "            try:\n"
               "                b64_data = line[5:]\n"
               "                raw_bytes = base64.b64decode(b64_data)\n"
               "                input_data = np.frombuffer(raw_bytes, dtype=np.uint8).astype(np.float32)\n"
               "                \n"
               "                # RGB-ANPASSUNG: Ändert die letzte Dimension von 1 auf 3 Farbkanäle (R, G, B)\n"
               "                input_data = input_data.reshape(1, 224, 224, 3) / 255.0\n"
               "                \n"
               "                interpreter.set_tensor(input_index, input_data)\n"
               "                interpreter.invoke()\n"
               "                output_data = interpreter.get_tensor(output_index)\n"
               "                \n"
               "                flat_output = output_data.flatten()\n"
               "                max_idx = int(np.argmax(flat_output))\n"
               "                prob = float(flat_output[max_idx])\n"
               "                \n"
               "                if prob > 0.5 and 0 <= max_idx < len(labels):\n"
               "                    label_text = str(labels[max_idx])\n";

        // Prozentzeichen isoliert wegschreiben
        out << "                    print('RESULT: ' + label_text + ' (' + str(round(prob*100, 1)) + '" << "%" << ")', flush=True)\n";

        out << "                else:\n"
               "                    print('RESULT: Nichts erkannt', flush=True)\n"
               "            except Exception as e:\n"
               "                print('RESULT: Fehler bei Inferenz (' + str(e) + ')', flush=True)\n\n"
               "if __name__ == '__main__':\n"
               "    main()\n";
        kiFile.close();
    }
}
// erstelleInferenzSkriptDatei




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
    /*
    // bisheriger Speicherpfad für Qt-Creator-Umgebung
    QDir baseDir(QCoreApplication::applicationDirPath());
    baseDir.cdUp();
    baseDir.mkdir("dataset");
    m_datasetPath = baseDir.absoluteFilePath("dataset");
    cout << "Zentraler Speicherpfad: " << m_datasetPath.toStdString() << endl;
    */
    
    // Speicherpfad-Fix, neu!  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    QDir baseDir(QCoreApplication::applicationDirPath());

    // Wenn wir im Qt Creator Shadow-Build (debug/release) sind, gehen wir hoch.
    // Wenn wir als fertige .exe im Projektordner liegen, bleiben wir dort!
    if (baseDir.dirName() == "debug" || baseDir.dirName() == "release") {
        baseDir.cdUp();
    }

    // Ordner "dataset" prüfen/erstellen
    if (!baseDir.exists("dataset")) {
        baseDir.mkdir("dataset");
    }

    m_datasetPath = baseDir.absoluteFilePath("dataset");
    cout << "Zentraler Speicherpfad: " << m_datasetPath.toStdString() << endl;

    // 


    // Lädt die labels.cfg direkt beim Starten des Programms in den RAM
    ladeLabelsKonfiguration();

    ui->labelCameraLive->setScaledContents(true);
    ui->labelScreenshot->setScaledContents(true);
    ui->labelKiInput->setScaledContents(false);

    // AUFRUF DER AUSGELAGERTEN KAMERA- UND BELICHTUNGS-PIPELINE
    setupKameraPipeline();

    ui->progressBar->setValue(0);

    // Aufruf der unbeschränkten, globalen Funktion
    erstelleInferenzSkriptDatei(m_datasetPath);

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

    on_btnLoadDataset_clicked();  // lädt Dataset beim Start

} // MainWindow::MainWindow




MainWindow::~MainWindow()
{
    if (m_kiProzess) {
        m_kiProzess->terminate();
        m_kiProzess->waitForFinished(3000);
    }
    if (m_camera) {
        m_camera->stop();
    }
    delete ui;
} // MainWindow::~MainWindow




void MainWindow::setupKameraPipeline()
{
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

            // DYNAMISCHE SOFTWARE-BELICHTUNGSAUTOMATIK (SOFTWARE AEC)
            int w = liveBild.width();
            int h = liveBild.height();

            long long helligkeitSumme = 0;
            int gemessenePixel = 0;

            int zentStartX = std::max(0, w / 2 - 150);
            int zentStartY = std::max(0, h / 2 - 150);
            int zentEndX = std::min(w, w / 2 + 150);
            int zentEndY = std::min(h, h / 2 + 150);

            for (int y = zentStartY; y < zentEndY; ++y) {
                const QRgb *row = reinterpret_cast<const QRgb*>(liveBild.scanLine(y));
                for (int x = zentStartX; x < zentEndX; ++x) {
                    helligkeitSumme += (qRed(row[x]) * 299 + qGreen(row[x]) * 587 + qBlue(row[x]) * 114) / 1000;
                    gemessenePixel++;
                }
            }

            int durchschnittsHelligkeit = (gemessenePixel > 0) ? (helligkeitSumme / gemessenePixel) : 128;
            int zielHelligkeit = 120;
            double anpassungsFaktor = 1.0;

            if (durchschnittsHelligkeit > zielHelligkeit) {
                anpassungsFaktor = static_cast<double>(zielHelligkeit) / static_cast<double>(durchschnittsHelligkeit);
                if (anpassungsFaktor < 0.35) anpassungsFaktor = 0.35;
            }

            if (anpassungsFaktor < 0.98) {
                double kontrastFaktor = 1.0 + (1.0 - anpassungsFaktor) * 0.5;

                for (int y = 0; y < h; ++y) {
                    QRgb *row = reinterpret_cast<QRgb*>(liveBild.scanLine(y));
                    for (int x = 0; x < w; ++x) {
                        int r = static_cast<int>(qRed(row[x]) * anpassungsFaktor);
                        int g = static_cast<int>(qGreen(row[x]) * anpassungsFaktor);
                        int b = static_cast<int>(qBlue(row[x]) * anpassungsFaktor);

                        r = static_cast<int>((r - 128) * kontrastFaktor + 128);
                        g = static_cast<int>((g - 128) * kontrastFaktor + 128);
                        b = static_cast<int>((b - 128) * kontrastFaktor + 128);

                        row[x] = qRgb(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
                    }
                }
            }

            ui->labelCameraLive->setPixmap(QPixmap::fromImage(liveBild));

            if (m_kiErkennungAktiv && m_kiProzess && m_kiProzess->state() == QProcess::Running) {
                if (kiIntervallTimer.elapsed() >= 100) {
                    kiIntervallTimer.restart();

                    QImage kiBild = prozessiereKiBild(liveBild);
                    QByteArray rgbBytes;
                    rgbBytes.reserve(224 * 224 * 3);

                    for (int y = 0; y < 224; ++y) {
                        const QRgb *row = reinterpret_cast<const QRgb*>(kiBild.scanLine(y));
                        for (int x = 0; x < 224; ++x) {
                            rgbBytes.append(static_cast<char>(qRed(row[x])));
                            rgbBytes.append(static_cast<char>(qGreen(row[x])));
                            rgbBytes.append(static_cast<char>(qBlue(row[x])));
                        }
                    }

                    QByteArray b64Text = rgbBytes.toBase64();
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
} // MainWindow::setupKameraPipeline



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
    if (ui->labelScreenshot->pixmap().isNull()) {
        QMessageBox::warning(this, "Fehler", "Kein Standbild im 2. Fenster vorhanden! Bitte zuerst Screenshot auslösen.");
        return;
    }

    if (m_klassenNamen.empty()) {
        QMessageBox::critical(this, "Fehler", "Keine Muster-Klassen geladen! Bitte labels.cfg überprüfen.");
        return;
    }

    // 1. Spickzettel dynamisch aus der geladenen m_klassenNamen Map generieren
    QString spickzettel = "<b>Verfügbare Muster-IDs:</b><br><br><table border='0' cellspacing='2' cellpadding='1'>";
    int spaltenZaehler = 0;

    for (auto const& [id, name] : m_klassenNamen) {
        if (spaltenZaehler == 0) spickzettel += "<tr>";
        spickzettel += QString("<td><b>%1:</b> %2</td>").arg(id).arg(name);
        spaltenZaehler++;

        if (spaltenZaehler >= 2) {
            spickzettel += "</tr>";
            spaltenZaehler = 0;
        } else {
            spickzettel += "<td>&nbsp;&nbsp;&nbsp;&nbsp;</td>";
        }
    }
    if (spaltenZaehler != 0) spickzettel += "</tr>";
    spickzettel += "</table><br>Gib eine oder mehrere IDs ein (getrennt durch Leerzeichen, z.B. 1 5 14):";

    // 2. Maßgeschneidertes Dialogfenster aufbauen
    QDialog dialog(this);
    dialog.setWindowTitle("Multi-Muster Klassifizierung");
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    QLabel *textLabel = new QLabel(spickzettel, &dialog);
    textLabel->setTextFormat(Qt::RichText);
    mainLayout->addWidget(textLabel);

    QLineEdit *customInput = new QLineEdit(&dialog);
    customInput->setPlaceholderText(" Hier IDs eintragen, z.B.: 1 5 14");
    customInput->setMinimumHeight(35);
    customInput->setStyleSheet(
        "QLineEdit { font-size: 14px; border: 2px solid #828282; border-radius: 5px; padding: 5px; background-color: #ffffff; }"
        "QLineEdit:focus { border: 2px solid #0078d4; }"
        );
    mainLayout->addWidget(customInput);
    mainLayout->addSpacing(10);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK", &dialog);
    QPushButton *cancelButton = new QPushButton("Abbrechen", &dialog);
    okButton->setMinimumHeight(30);
    cancelButton->setMinimumHeight(30);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        cout << "Speichervorgang abgebrochen." << endl;
        return;
    }

    QString classInput = customInput->text().trimmed();
    if (classInput.isEmpty()) return;

    // 3. IDs extrahieren und gegen geladene Map validieren
    QStringList idList = classInput.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    QString idSuffix = "";
    std::vector<int> validIds;

    for (const QString &idStr : std::as_const(idList)) {
        bool isNumber;
        int classId = idStr.toInt(&isNumber);

        if (!isNumber) {
            QMessageBox::critical(this, "Eingabefehler", QString("'%1' ist keine gültige Zahl!").arg(idStr));
            return;
        }

        // PRÜFUNG: Existiert die ID in der dynamisch geladenen Dateiliste?
        if (m_klassenNamen.find(classId) == m_klassenNamen.end()) {
            QMessageBox::critical(this, "Bereichsfehler",
                                  QString("Die ID %1 ist in Deiner 'labels.cfg' nicht definiert!").arg(classId));
            return;
        }

        if (std::find(validIds.begin(), validIds.end(), classId) == validIds.end()) {
            validIds.push_back(classId);
            idSuffix += QString("_ID%1").arg(classId);
        }
    }

    if (validIds.empty()) return;

    // 4. Bildprozessierung und Speichern
    QImage rawImage = ui->labelScreenshot->pixmap().toImage();
    QImage kiBild = prozessiereKiBild(rawImage);

    QString zeitstempel = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    // Optimierte Multi-Arg Zuweisung zur Vermeidung von Clazy-Warnungen
    QString standardName = QString("%1/ZZKI%2_%3.png").arg(m_datasetPath, idSuffix, zeitstempel);

    QString selectedFile = QFileDialog::getSaveFileName(
        this, "Multi-Label KI-Bild abspeichern", standardName, "PNG-Bilder (*.png)"
        );

    if (!selectedFile.isEmpty()) {
        if (!selectedFile.contains("_ID")) {
            QMessageBox::critical(this, "Abbruch", "Fehler: Die '_IDx' Kennung darf nicht gelöscht werden!");
            return;
        }

        if (kiBild.save(selectedFile, "PNG")) {
            cout << "Multi-Muster Bild erfolgreich indexiert: " << QFileInfo(selectedFile).fileName().toStdString() << endl;
            ui->statusbar->showMessage(QString("%1 Muster im Datensatz archiviert!").arg(validIds.size()), 3000);
        }
    }
}
// on_btnSaveKIpng_clicked// MainWindow::on_btnSaveKIpng_clicked










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


/*
 * // alte Pfade nur für Qt-Umbeung
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
*/

// Liest die labels.cfg Datei zeilenweise aus dem Quellcode-Verzeichnis ein
void MainWindow::ladeLabelsKonfiguration()
{
    m_klassenNamen.clear();

    // Ermittelt den Pfad, wo die EXE liegt
    QDir projDir(QCoreApplication::applicationDirPath());

    // NEU: Nur hochgehen, wenn wir uns im Unterordner debug oder release befinden (z.B. im Qt Creator)
    if (projDir.dirName() == "debug" || projDir.dirName() == "release") {
        projDir.cdUp();
    }

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


/*
// alte starre Pfade für Qt-Startumgebung
void MainWindow::on_btnKiTrain_clicked()
{
    std::cout << "SLOT: on_btnKiTrain_clicked aufgerufen" << std::endl;
    ui->statusbar->showMessage("on_btnKiTrain_clicked aufgerufen", 5000);

    ui->progressBar->setValue(0);

    // 1. SCHUTZ-SPERRE: Verhindert den Start des Trainings, wenn die Live-Erkennung aktiv ist
    if (m_kiErkennungAktiv) {
        std::cout << "  -> ABBRUCH: Live-Erkennung ist noch aktiv!" << std::endl;
        QMessageBox::warning(this, "Aktion blockiert",
                             "Das KI-Modell-Training kann nicht gestartet werden, während die Live-Erkennung aktiv ist!\n\n"
                             "Bitte halte zuerst die Erkennung über 'KI Stop' an.");
        return;
    }

    if (m_trainingInputs.empty()) {
        std::cout << "[ERROR] Trainings-Matrix ist leer! Bitte zuerst 'Export CSV' ausführen." << std::endl;
        return;
    }

    QDir projDir(QCoreApplication::applicationDirPath());
    projDir.cdUp();
    QString scriptPfad = projDir.absoluteFilePath("train_tensorFlow_net.py");

    QProcess *trainProcess = new QProcess(this);
    trainProcess->setWorkingDirectory(projDir.absolutePath());

    // Status-Merker auf aktiv setzen
    m_kiTrainingAktiv = true;

    // JETZT KOMPLETT: Graut ausnahmslos alle Tasten während des Keras-Trainings aus!
    ui->btnKiTrain->setEnabled(false);     // "KI Training" (sich selbst) ausgrauen
    ui->btnKiRun->setEnabled(false);       // "KI Run" / Start ausgrauen
    ui->btnKiStop->setEnabled(false);      // "KI Stop" ausgrauen
    ui->btnLoadDataset->setEnabled(false); // Lade Daten ausgrauen
    ui->btnExportCSV->setEnabled(false);   // Export CSV ausgrauen
    ui->btnSaveKIpng->setEnabled(false);   // SaveKIpng ausgrauen

    connect(trainProcess, &QProcess::readyReadStandardOutput, this, [this, trainProcess]() {
        QString out = trainProcess->readAllStandardOutput().trimmed();
        std::cout << "[PYTHON TRAIN]: " << out.toStdString() << std::endl;

        // Sucht fehlertolerant nach "SUCCESS", egal wie es in Python geschrieben steht
        if (out.contains("SUCCESS", Qt::CaseInsensitive)) {
            m_kiTrainingAktiv = false;

            // Schaltet nach dem Trainings-Erfolg ALLE Tasten wieder zurück in dein beiges Wunschdesign!
            ui->btnKiTrain->setEnabled(true);
            ui->btnKiRun->setEnabled(true);
            ui->btnKiStop->setEnabled(true);
            ui->btnLoadDataset->setEnabled(true);
            ui->btnExportCSV->setEnabled(true);
            ui->btnSaveKIpng->setEnabled(true);

            std::cout << "  -> Alle GUI-Buttons erfolgreich über CaseInsensitive reaktiviert." << std::endl;
        }
    });

    connect(trainProcess, &QProcess::readyReadStandardError, this, [this, trainProcess]() {
        QString err = trainProcess->readAllStandardError().trimmed();
        if (!err.isEmpty()) {
            std::cout << "[PYTHON TRAIN ERR]: " << err.toStdString() << std::endl;
        }

        // Falls das Skript abstürzt, müssen alle Tasten ebenfalls wieder freigegeben und beige gefärbt werden!
        if (err.contains("Exception") || err.contains("Error") || err.contains("Absturz")) {
            m_kiTrainingAktiv = false;

            ui->btnKiTrain->setEnabled(true);
            ui->btnKiRun->setEnabled(true);
            ui->btnKiStop->setEnabled(true);
            ui->btnLoadDataset->setEnabled(true);
            ui->btnExportCSV->setEnabled(true);
            ui->btnSaveKIpng->setEnabled(true);
        }
    });

    trainProcess->start("python", QStringList() << "-u" << scriptPfad);
    std::cout << "Trainings-Skript wurde asynchron gestartet..." << std::endl;
    ui->statusbar->showMessage("Trainings-Skript wurde asynchron gestartet...", 5000);

} // MainWindow::on_btnKiTrain_clicked
*/


// intelligente Pfad-Abfrage eingessetzt,
// Zusätzlich Arbeitsverzeichnis für den QProcess (setWorkingDirectory) angepasst,
// damit Python seine Ausgabedateien (wie das trainierte Modell) ebenfalls im richtigen Projektordner ablegt

void MainWindow::on_btnKiTrain_clicked()
{
    std::cout << "SLOT: on_btnKiTrain_clicked aufgerufen" << std::endl;
    ui->statusbar->showMessage("on_btnKiTrain_clicked aufgerufen", 5000);

    ui->progressBar->setValue(0);

    // 1. SCHUTZ-SPERRE: Verhindert den Start des Trainings, wenn die Live-Erkennung aktiv ist
    if (m_kiErkennungAktiv) {
        std::cout << "  -> ABBRUCH: Live-Erkennung ist noch aktiv!" << std::endl;
        QMessageBox::warning(this, "Aktion blockiert",
                             "Das KI-Modell-Training kann nicht gestartet werden, während die Live-Erkennung aktiv ist!\n\n"
                             "Bitte halte zuerst die Erkennung über 'KI Stop' an.");
        return;
    }

    if (m_trainingInputs.empty()) {
        std::cout << "[ERROR] Trainings-Matrix ist leer! Bitte zuerst 'Export CSV' ausführen." << std::endl;
        return;
    }

    // Ermittelt den Pfad, wo die EXE liegt
    QDir projDir(QCoreApplication::applicationDirPath());

    // NEU: Nur hochgehen, wenn wir uns im Unterordner debug oder release befinden (z.B. im Qt Creator)
    if (projDir.dirName() == "debug" || projDir.dirName() == "release") {
        projDir.cdUp();
    }

    QString scriptPfad = projDir.absoluteFilePath("train_tensorFlow_net.py");

    QProcess *trainProcess = new QProcess(this);
    // Setzt das Arbeitsverzeichnis für Python auf den korrigierten Projektordner
    trainProcess->setWorkingDirectory(projDir.absolutePath());

    // ==============================================================================
    // DYNAMISCHER FIX: System-Umgebung kopieren & DLL-Sperren hinzufügen
    // Keine Hardcoded-Pfade, liest live die Umgebung des aktuellen PCs aus!
    // ==============================================================================
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("KMP_DUPLICATE_LIB_OK", "TRUE");
    env.insert("TF_ENABLE_ONEDNN_OPTS", "0");
    trainProcess->setProcessEnvironment(env);

    // Status-Merker auf aktiv setzen
    m_kiTrainingAktiv = true;

    // JETZT KOMPLETT: Graut ausnahmslos alle Tasten während des Keras-Trainings aus!
    ui->btnKiTrain->setEnabled(false);     // "KI Training" (sich selbst) ausgrauen
    ui->btnKiRun->setEnabled(false);       // "KI Run" / Start ausgrauen
    ui->btnKiStop->setEnabled(false);      // "KI Stop" ausgrauen
    ui->btnLoadDataset->setEnabled(false); // Lade Daten ausgrauen
    ui->btnExportCSV->setEnabled(false);   // Export CSV ausgrauen
    ui->btnSaveKIpng->setEnabled(false);   // SaveKIpng ausgrauen

    connect(trainProcess, &QProcess::readyReadStandardOutput, this, [this, trainProcess]() {
        QString out = trainProcess->readAllStandardOutput().trimmed();
        std::cout << "[PYTHON TRAIN]: " << out.toStdString() << std::endl;

        // Sucht fehlertolerant nach "SUCCESS", egal wie es in Python geschrieben steht
        if (out.contains("SUCCESS", Qt::CaseInsensitive)) {
            m_kiTrainingAktiv = false;

            // Schaltet nach dem Trainings-Erfolg ALLE Tasten wieder zurück in dein beiges Wunschdesign!
            ui->btnKiTrain->setEnabled(true);
            ui->btnKiRun->setEnabled(true);
            ui->btnKiStop->setEnabled(true);
            ui->btnLoadDataset->setEnabled(true);
            ui->btnExportCSV->setEnabled(true);
            ui->btnSaveKIpng->setEnabled(true);

            std::cout << "  -> Alle GUI-Buttons erfolgreich über CaseInsensitive reaktiviert." << std::endl;
        }
    });

    connect(trainProcess, &QProcess::readyReadStandardError, this, [this, trainProcess]() {
        QString err = trainProcess->readAllStandardError().trimmed();
        if (!err.isEmpty()) {
            std::cout << "[PYTHON TRAIN ERR]: " << err.toStdString() << std::endl;
        }

        // Falls das Skript abstürzt, müssen alle Tasten ebenfalls wieder freigegeben und beige gefärbt werden!
        if (err.contains("Exception") || err.contains("Error") || err.contains("Absturz")) {
            m_kiTrainingAktiv = false;

            ui->btnKiTrain->setEnabled(true);
            ui->btnKiRun->setEnabled(true);
            ui->btnKiStop->setEnabled(true);
            ui->btnLoadDataset->setEnabled(true);
            ui->btnExportCSV->setEnabled(true);
            ui->btnSaveKIpng->setEnabled(true);
        }
    });

    // Startet die globale python.exe des Systems mit der kopierten Betriebssystem-Umgebung
    trainProcess->start("python", QStringList() << "-u" << scriptPfad);
    std::cout << "Trainings-Skript wurde asynchron gestartet..." << std::endl;
    ui->statusbar->showMessage("Trainings-Skript wurde asynchron gestartet...", 5000);

} // MainWindow::on_btnKiTrain_clicked




/*
// alt
void MainWindow::on_btnKiRun_clicked()
{
    std::cout << "SLOT: on_btnKiRun_clicked aufgerufen" << std::endl;
    ui->progressBar->setValue(0);

    // SCHUTZ-SPERRE: Verhindert den Start der Erkennung, wenn das Keras-Training läuft
    if (m_kiTrainingAktiv) {
        std::cout << "  -> ABBRUCH: KI-Modell-Training läuft im Hintergrund!" << std::endl;
        QMessageBox::warning(this, "Aktion blockiert",
                             "Die Live-Erkennung kann nicht gestartet werden, während im Hintergrund das Netzwerk trainiert!\n\n"
                             "Bitte warte, bis das Training im Konsolenfenster vollständig abgeschlossen ist.");
        return;
    }

    if (m_kiErkennungAktiv) return;

    if (m_kiProzess->state() != QProcess::Running) {
        QDir projDir(QCoreApplication::applicationDirPath());
        projDir.cdUp();
        m_kiProzess->setWorkingDirectory(projDir.absolutePath());

        QString scriptPfad = projDir.absoluteFilePath("tmp/generated_inference.py");
        QStringList args;
        args << "-u" << scriptPfad;

        m_kiProzess->start("python", args);

        if (!m_kiProzess->waitForStarted()) {
            std::cout << "[ERROR] KI-Prozess konnte nicht gestartet werden!" << std::endl;
            return;
        }
    }

    m_kiErkennungAktiv = true;

    // VISUELLE SPERREN: Graut den eigenen Start-Button und den Trainings-Button sofort aus!
    ui->btnKiRun->setEnabled(false);      // "KI Run" / Start sperren
    ui->btnKiTrain->setEnabled(false);    // "KI Training" sperren

    std::cout << "KI Live-Erkennung erfolgreich gestartet." << std::endl;
} // MainWindow::on_btnKiRun_clicked
*/

// neue Pfade, dynamisch:



void MainWindow::on_btnKiRun_clicked()
{
    std::cout << "SLOT: on_btnKiRun_clicked aufgerufen" << std::endl;
    ui->progressBar->setValue(0);

    // SCHUTZ-SPERRE: Verhindert den Start der Erkennung, wenn das Keras-Training läuft
    if (m_kiTrainingAktiv) {
        std::cout << "  -> ABBRUCH: KI-Modell-Training läuft im Hintergrund!" << std::endl;
        QMessageBox::warning(this, "Aktion blockiert",
                             "Die Live-Erkennung kann nicht gestartet werden, während im Hintergrund das Netzwerk trainiert!\n\n"
                             "Bitte warte, bis das Training im Konsolenfenster vollständig abgeschlossen ist.");
        return;
    }

    if (m_kiErkennungAktiv) return;

    if (m_kiProzess->state() != QProcess::Running) {
        // Ermittelt den Pfad, wo die EXE liegt
        QDir projDir(QCoreApplication::applicationDirPath());

        // NEU: Nur hochgehen, wenn wir uns im Unterordner debug oder release befinden (z.B. im Qt Creator)
        if (projDir.dirName() == "debug" || projDir.dirName() == "release") {
            projDir.cdUp();
        }

        // WICHTIG: Das Arbeitsverzeichnis für Python wird direkt in den dataset-Ordner gelegt!
        // Dadurch findet das Python-Skript 'model.tflite' und 'labels.txt' direkt vor Ort.
        QString datasetPfad = projDir.absoluteFilePath("dataset");
        m_kiProzess->setWorkingDirectory(datasetPfad);

        // Das Inferenz-Skript selbst liegt weiterhin im tmp-Ordner des Hauptverzeichnisses
        QString scriptPfad = projDir.absoluteFilePath("tmp/generated_inference.py");
        QStringList args;
        args << "-u" << scriptPfad;

        m_kiProzess->start("python", args);

        if (!m_kiProzess->waitForStarted()) {
            std::cout << "[ERROR] KI-Prozess konnte nicht gestartet werden!" << std::endl;
            return;
        }
    }

    m_kiErkennungAktiv = true;

    // VISUELLE SPERREN: Graut den eigenen Start-Button und den Trainings-Button sofort aus!
    ui->btnKiRun->setEnabled(false);      // "KI Run" / Start sperren
    ui->btnKiTrain->setEnabled(false);    // "KI Training" sperren

    std::cout << "KI Live-Erkennung erfolgreich gestartet." << std::endl;
} // MainWindow::on_btnKiRun_clicked




void MainWindow::on_btnKiStop_clicked()
{
    std::cout << "SLOT: on_btnKiStop_clicked aufgerufen" << std::endl;
    ui->progressBar->setValue(0);

    if (m_kiProzess && m_kiProzess->state() == QProcess::Running) {
        m_kiProzess->terminate();
        m_kiProzess->waitForFinished(2000);
    }
    m_kiErkennungAktiv = false;

    // REPARATUR: Schaltet beide Buttons nach dem Stop farblich wieder aktiv frei!
    ui->btnKiRun->setEnabled(true);   // "KI Run" wieder freigeben
    ui->btnKiTrain->setEnabled(true); // "KI Training" wieder freigeben

    std::cout << "KI Live-Erkennung angehalten. GUI reaktiviert." << std::endl;
} // MainWindow::on_btnKiStop_clicked







void MainWindow::on_btnLoadDataset_clicked() //  (RGB)
{
    ui->progressBar->setValue(0);
    m_trainingInputs.clear();
    m_trainingTargets.clear();

    int maxSlots = getNumClasses();
    QDir datasetDir(m_datasetPath);
    QStringList filter;
    filter << "ZZKI_*.png";
    QStringList fileList = datasetDir.entryList(filter, QDir::Files);

    if (fileList.isEmpty()) {
        cout << "Fehler: Keine Trainingsdaten mit Maske 'ZZKI_*.png' im Dataset-Ordner gefunden!" << endl;
        return;
    }

    cout << "Lese " << fileList.size() << " Multi-Label RGB-Bilder in den RAM ein..." << endl;
    QRegularExpression re("_ID(\\d+)(?=[_\\.])");

    for (const QString &fileName : std::as_const(fileList)) {
        std::vector<float> targetVector(maxSlots, 0.0f);
        bool idGefunden = false;

        QRegularExpressionMatchIterator it = re.globalMatch(fileName);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int classId = match.captured(1).toInt();
            if (classId >= 0 && classId < maxSlots) {
                targetVector[classId] = 1.0f;
                idGefunden = true;
            }
        }

        if (!idGefunden) continue;

        QImage img(datasetDir.absoluteFilePath(fileName));
        if (img.isNull()) continue;
        img = img.convertToFormat(QImage::Format_RGB32);

        std::vector<float> inputVector;
        inputVector.reserve(224 * 224 * 3); // Verdreifacht für R, G, B

        for (int y = 0; y < 224; ++y) {
            const QRgb *row = reinterpret_cast<const QRgb*>(img.scanLine(y));
            for (int x = 0; x < 224; ++x) {
                inputVector.push_back(static_cast<float>(qRed(row[x])) / 255.0f);
                inputVector.push_back(static_cast<float>(qGreen(row[x])) / 255.0f);
                inputVector.push_back(static_cast<float>(qBlue(row[x])) / 255.0f);
            }
        }

        m_trainingInputs.push_back(inputVector);
        m_trainingTargets.push_back(targetVector);
    }

    QString msg = "RAM-Status: " + QString::number(m_trainingInputs.size()) +
                  " Muster einsatzbereit im Speicher (Slots: " + QString::number(maxSlots) + ").";


    // neues log in cout und statusbar
    std::cout << msg.toStdString() << std::endl;
    ui->statusbar->showMessage(QString("Erfolgreich geladen: %1 RGB-Bilder im RAM").arg(m_trainingInputs.size()), 4000);
    ui->statusbar->showMessage(msg, 4000);


}
//  on_btnLoadDataset_clicked (RGB)



void MainWindow::on_btnExportCSV_clicked()   //  (RGB)
{
    ui->progressBar->setValue(0);
    /*
    ui->btnKiTrain->setEnabled(false);    // "KI Training" sperren
    ui->btnKiRun->setEnabled(false);       // "KI RUN" sperren
    */

    int maxSlots = getNumClasses();
    if (m_trainingInputs.empty() || m_trainingTargets.empty() || (m_trainingInputs.size() != m_trainingTargets.size())) {
        cout << "Fehler: Datensatz asynchron oder leer! Export abgebrochen." << endl;
        return;
    }

    cout << "Generiere TensorFlow RGB CSV-Datensatz..." << endl;
    QString exportPath = m_datasetPath + "/tensorflow_dataset.csv";
    QFile file(exportPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    int totalFiles = m_trainingInputs.size();

    // RGB CSV Header schreiben (pixel_0_r, pixel_0_g, pixel_0_b...)
    for (int i = 0; i < 224 * 224; ++i) {
        out << "pixel_" << i << "_r,pixel_" << i << "_g,pixel_" << i << "_b,";
    }
    for (int i = 0; i < maxSlots; ++i) {
        out << "target_" << i << (i == maxSlots - 1 ? "" : ",");
    }
    out << "\n";

    for (int i = 0; i < totalFiles; ++i) {
        const std::vector<float>& currentInput = m_trainingInputs[i];
        for (float val : currentInput) {
            out << val << ",";
        }

        const std::vector<float>& currentTarget = m_trainingTargets[i];
        for (int t = 0; t < maxSlots; ++t) {
            out << currentTarget[t] << (t == maxSlots - 1 ? "" : ",");
        }
        out << "\n";

        ui->progressBar->setValue(static_cast<int>(((i + 1) * 100) / totalFiles));
        QCoreApplication::processEvents();
    }
    file.close();

    /*
    ui->btnKiTrain->setEnabled(true);    // "KI Training" freigeben
    ui->btnKiRun->setEnabled(true);      // "KI RUN" freigeben
    */
    cout << "SUCCESS: RGB-Export abgeschlossen! " << totalFiles << " Bilder geschrieben." << endl;
    ui->statusbar->showMessage(QString("SUCCESS: RGB-Export abgeschlossen! %1 Bilder geschrieben.").arg(totalFiles), 5000);
}
//  on_btnExportCSV_clicked //  (RGB)



/*
void MainWindow::timer_snapshot()
{
    // Snapshot-Intervall-Triggerslot aus dem Header
}
// MainWindow::timer_snapshot, deprecated
*/




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



//*************************
// EOF
//*************************




