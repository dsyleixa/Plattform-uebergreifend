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



using namespace std;

// ZENTRALE HELFERFUNKTION: Verhindert Code-Duplikate und sichert identische Bildverarbeitung
QImage prozessiereKiBild(const QImage &quellBild)
{
    if (quellBild.isNull()) return QImage();

    int w = quellBild.width();
    int h = quellBild.height();
    int minDim = std::min(w, h);
    int startX = (w - minDim) / 2;
    int startY = (h - minDim) / 2;

    // 1. Quadratischer Zuschnitt aus der Mitte
    QImage quadrat = quellBild.copy(startX, startY, minDim, minDim);

    // 2. Skalierung auf TensorFlow-Standardgröße
    QImage kiBild = quadrat.scaled(224, 224, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // 3. Konvertierung in reine Graustufen (1 Byte pro Pixel)
    kiBild = kiBild.convertToFormat(QImage::Format_Grayscale8);

    // 4. Autokontrast (Min/Max-Normalisierung auf volles 8-Bit-Spektrum)
    int minPixel = 255, maxPixel = 0;
    for (int y = 0; y < kiBild.height(); ++y) {
        uchar *row = kiBild.scanLine(y);
        for (int x = 0; x < kiBild.width(); ++x) {
            if (row[x] < minPixel) minPixel = row[x];
            if (row[x] > maxPixel) maxPixel = row[x];
        }
    }

    if (maxPixel > minPixel) {
        for (int y = 0; y < kiBild.height(); ++y) {
            uchar *row = kiBild.scanLine(y);
            for (int x = 0; x < kiBild.width(); ++x) {
                row[x] = static_cast<uchar>(((row[x] - minPixel) * 255) / (maxPixel - minPixel));
            }
        }
    }
    return kiBild;
}





MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    FreeConsole();
    AllocConsole();
    std::freopen("CONOUT$", "w", stdout);
    std::freopen("CONOUT$", "w", stderr);

    ui->setupUi(this);

    // ==========================================================
    // Speicherpfad im Projektverzeichnis initialisieren
    QDir baseDir(QCoreApplication::applicationDirPath());
    baseDir.cdUp();
    baseDir.mkdir("dataset");
    m_datasetPath = baseDir.absoluteFilePath("dataset");
    cout << "Zentraler Speicherpfad: " << m_datasetPath.toStdString() << endl;

    // Lädt die labels.cfg direkt beim Starten des Programms in den RAM
    ladeLabelsKonfiguration();
    // ==========================================================

    ui->labelCameraLive->setScaledContents(true);
    ui->labelScreenshot->setScaledContents(true);
    ui->labelKiInput->setScaledContents(false);

    // ==========================================================
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

    // ==========================================================
    // Live-Stream Signal mit Text-Pipeline (Base64) und 100ms Drossel
    // ==========================================================
    static QElapsedTimer kiIntervallTimer;
    kiIntervallTimer.start();

    connect(m_sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        QImage liveBild = frame.toImage().convertToFormat(QImage::Format_RGB32);
        if (!liveBild.isNull()) {
            ui->labelCameraLive->setPixmap(QPixmap::fromImage(liveBild));

            // WENN DIE KI LÄUFT: Konvertiere das Bild zu Base64 und sende es als Textzeile
            if (m_kiErkennungAktiv && m_kiProzess && m_kiProzess->state() == QProcess::Running) {

                // Exakte Taktung: Nur alle 100ms ein Bild senden (10 Bilder/Sekunde)
                if (kiIntervallTimer.elapsed() >= 100) {
                    kiIntervallTimer.restart();

                    QImage kiBild = prozessiereKiBild(liveBild); // 224x224 Grayscale8

                    // Wandelt die rohen 50.176 Bytes im RAM in ein Qt-Bytearray um
                    QByteArray rawBytes(reinterpret_cast<const char*>(kiBild.bits()), 224 * 224);

                    // Zu sicherem Base64-Text kodieren (verhindert Windows-Binärfehler)
                    QByteArray b64Text = rawBytes.toBase64();

                    // Als saubere Textzeile mit "DATA:"-Präfix und Zeilenumbruch absenden
                    m_kiProzess->write("DATA:" + b64Text + "\n");

                    // ZWINGEND ERFORDERLICH: Drückt die Textzeile SOFORT physikalisch durch die Pipeline!
                    m_kiProzess->waitForBytesWritten(1);
                }
            }
        }
    });

    // ==========================================================
    // Hardware-Screenshot Signal (Asynchron)
    // ==========================================================
    connect(m_imageCapture, &QImageCapture::imageCaptured, this, [this](int id, const QImage &preview) {
        Q_UNUSED(id);
        if (preview.isNull()) return;

        m_currentImage = preview;
        ui->labelScreenshot->setPixmap(QPixmap::fromImage(m_currentImage));

        // Nutzung der zentralen Transformations-Funktion
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

    // Setzt den Fortschrittsbalken beim Programmstart sauber auf 0% zurück
    ui->progressBar->setValue(0);

    // ==========================================================
    // Hintergrund-Prozess für die KI initialisieren
    // ==========================================================
    m_kiProzess = new QProcess(this);

    // Signal: Wenn Python ein Ergebnis liefert, wird es sofort in der GUI-Statusleiste angezeigt
    connect(m_kiProzess, &QProcess::readyReadStandardOutput, this, [this]() {
        QString ausgabe = m_kiProzess->readAllStandardOutput().trimmed();
        QStringList zeilen = ausgabe.split("\n");
        for (const QString &zeile : std::as_const(zeilen)) {
            if (zeile.startsWith("RESULT:")) {
                QString reinesErgebnis = zeile.mid(7).trimmed();
                ui->statusbar->showMessage("KI Live: " + reinesErgebnis);
            }
        }
    });

    // Signal für Fehler (stderr) von Python abfangen und in die Konsole drucken
    connect(m_kiProzess, &QProcess::readyReadStandardError, this, [this]() {
        QString fehler = m_kiProzess->readAllStandardError().trimmed();
        cout << "[PYTHON KI FEHLER]: " << fehler.toStdString() << endl;
    });
}

// MainWindow()







MainWindow::~MainWindow()
{
    if (m_camera) m_camera->stop();
    delete ui;
}

void MainWindow::on_vidPause_clicked()
{
    if (m_camera) {
        m_camera->stop();
        cout << "Live-Stream pausiert." << endl;
    }
}

void MainWindow::on_vidRun_clicked()
{
    if (m_camera) {
        m_camera->start();
        cout << "Live-Stream läuft weiter." << endl;
    }
}

void MainWindow::on_quitButton_released()
{
    close();
}


void MainWindow::on_vidScrshot_clicked()
{
    // FALL 1: Live-Stream läuft aktiv
    if (m_camera && m_camera->isActive() && m_imageCapture && m_imageCapture->isReadyForCapture()) {
        m_imageCapture->capture();
        cout << "Hardware-Screenshot vom Live-Stream angefordert." << endl;

        QPixmap currentPixmap = ui->labelCameraLive->pixmap();
        if (!currentPixmap.isNull()) {
            QString fileName = m_datasetPath + "/screenshot_stream.png";
            currentPixmap.save(fileName, "PNG");
        }
    }
    // FALL 2: Standbild / Pausenmodus
    else {
        QPixmap currentPixmap = ui->labelCameraLive->pixmap();
        if (!currentPixmap.isNull()) {
            m_currentImage = currentPixmap.toImage();
            ui->labelScreenshot->setPixmap(currentPixmap);

            QString fileName = m_datasetPath + "/screenshot_pause.png";
            currentPixmap.save(fileName, "PNG");

            QImage kiBild = prozessiereKiBild(m_currentImage);
            ui->labelKiInput->setPixmap(QPixmap::fromImage(kiBild));
        } else {
            cout << "Fehler: Kein Standbild im Kamera-Label vorhanden." << endl;
        }
    }
}

// OPTIMIERT: Speichert das KI-Bild mit der für den Loader zwingend erforderlichen ID-Maske





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
// on_btnSaveKIpng_clicked







void MainWindow::on_btnLoadDataset_clicked()
{
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




void MainWindow::on_btnExportCSV_clicked()
{
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




// Gibt die dynamische Anzahl an Ausgängen zurück
int MainWindow::getNumClasses() const
{
    if (m_klassenNamen.empty()) return 0;
    return m_klassenNamen.rbegin()->first + 1;
}



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
}




void MainWindow::on_btnKiRun_clicked()
{
    ui->progressBar->setValue(0);

    if (m_kiErkennungAktiv) return;

    QDir projDir(QCoreApplication::applicationDirPath());
    projDir.cdUp();
    QString skriptPfad = projDir.absoluteFilePath("run_inference.py");

    if (!QFile::exists(skriptPfad)) {
        QMessageBox::critical(this, "Fehler", "Das Erkennungs-Skript 'run_inference.py' wurde nicht gefunden!");
        return;
    }

    cout << "Starte KI-Hintergrundprozess (Unbuffered)..." << endl;

    // Das "-u" Flag schaltet die Python-Pufferung komplett ab!
    QStringList argumente;
    argumente << "-u" << skriptPfad;

#ifdef Q_OS_WIN
    m_kiProzess->start("python", argumente);
#else
    m_kiProzess->start("python3", argumente);
#endif

    m_kiErkennungAktiv = true;
    ui->statusbar->showMessage("KI Live-Erkennung gestartet.", 3000);
    ui->btnKiRun->setEnabled(false);
    ui->btnKiStop->setEnabled(true);
}



// MainWindow::on_btnKiRun_clicked




void MainWindow::on_btnKiStop_clicked()
{
    ui->progressBar->setValue(0);

    if (!m_kiErkennungAktiv) return;

    cout << "Stoppe KI-Hintergrundprozess." << endl;
    m_kiErkennungAktiv = false;

    if (m_kiProzess) {
        m_kiProzess->close(); // Schließt das Python-Skript im Hintergrund sauber
    }

    ui->statusbar->showMessage("KI Live-Erkennung gestoppt.", 3000);
    ui->btnKiRun->setEnabled(true);
    ui->btnKiStop->setEnabled(false);
}
// ::on_btnKiStop_clicked




void MainWindow::on_btnKiTrain_clicked()
{
    ui->progressBar->setValue(0);
    // 1. SCHUTZ: Ist ein Training bereits aktiv?
    if (m_kiTrainingAktiv) {
        QMessageBox::warning(this, "Training läuft bereits",
                             "Das TensorFlow-Training läuft aktuell im Hintergrund. Bitte warte, bis es beendet ist!");
        return;
    }

    // 2. SCHUTZ: Läuft parallel die Live-Erkennung?
    if (m_kiErkennungAktiv) {
        QMessageBox::warning(this, "Erkennung aktiv",
                             "Bitte stoppe zuerst die Live-Erkennung (KI Stop), bevor Du ein neues Training startest!");
        return;
    }

    // Pfade für das Skript vorbereiten
    QDir baseDir(QCoreApplication::applicationDirPath());
    QDir projDir = baseDir;
    projDir.cdUp();

    QStringList moeglichePfade;
    moeglichePfade << projDir.absoluteFilePath("train_tensorFlow_net.py");
    moeglichePfade << projDir.absoluteFilePath("train_cnn.py");
    moeglichePfade << baseDir.absoluteFilePath("train_tensorFlow_net.py");
    moeglichePfade << baseDir.absoluteFilePath("train_cnn.py");

    QString gefundenerSkriptPfad = "";
    for (const QString &pfad : std::as_const(moeglichePfade)) {
        if (QFile::exists(pfad)) {
            gefundenerSkriptPfad = pfad;
            break;
        }
    }

    if (gefundenerSkriptPfad.isEmpty()) {
        QMessageBox::critical(this, "Skript nicht gefunden", "Das Trainings-Skript konnte nicht lokalisiert werden!");
        return;
    }

    // SYSTEM SPERREN: Status auf aktiv setzen und Button ausgrauen
    m_kiTrainingAktiv = true;
    ui->btnKiTrain->setEnabled(false); // Der Button wird sofort für Klicks blockiert!

    ui->statusbar->showMessage("Starte TensorFlow-CNN-Training im Hintergrund...", 5000);
    cout << "\n========================================================" << endl;
    cout << "START: Training wird ausgeführt via:\n -> " << gefundenerSkriptPfad.toStdString() << endl;
    cout << "========================================================" << endl;

    QProcess *trainProzess = new QProcess(this);



    // Live-Textausgaben (stdout) an C++ Terminal umleiten
    connect(trainProzess, &QProcess::readyReadStandardOutput, this, [trainProzess]() {
        cout << trainProzess->readAllStandardOutput().trimmed().toStdString() << endl;
    });

    // Fehlermeldungen (stderr) an C++ Terminal umleiten
    connect(trainProzess, &QProcess::readyReadStandardError, this, [trainProzess]() {
        cout << "[TRAIN PYTHON LOG]: " << trainProzess->readAllStandardError().trimmed().toStdString() << endl;
    });

    // Wenn das Training vollständig fertig ist
    connect(trainProzess, &QProcess::finished, this, [this, trainProzess](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(status);

        // SYSTEM FREIGEBEN: Status zurücksetzen und Button wieder aktiv schalten
        m_kiTrainingAktiv = false;
        ui->btnKiTrain->setEnabled(true); // Button wird wieder normal klickbar!

        if (exitCode == 0) {
            cout << "\n========================================================" << endl;
            cout << "SUCCESS: TensorFlow-Training erfolgreich beendet!" << endl;
            cout << "========================================================" << endl;
            ui->statusbar->showMessage("KI-Training erfolgreich abgeschlossen!", 5000);
            QMessageBox::information(this, "Training beendet", "Das neuronale Netz wurde erfolgreich trainiert und das neue TFLite-Modell exportiert!");
        } else {
            ui->statusbar->showMessage("Fehler beim KI-Training! Siehe Konsole.", 5000);
            QMessageBox::critical(this, "Trainings-Abbruch", "Das Python-Skript meldete einen Fehler. Bitte prüfe die schwarze Konsole!");
        }
        trainProzess->deleteLater();
    });

    QStringList argumente;
    argumente << "-u" << gefundenerSkriptPfad;

#ifdef Q_OS_WIN
    trainProzess->start("python", argumente);
#else
    trainProzess->start("python3", argumente);
#endif
}
// on_btnKiTrain_clicked



// eof


