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


#define PY_SSIZE_T_CLEAN
#include <Python.h>


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


// =========================================================
// ZENTRALE BILDVERARBEITUNG
// =========================================================
QImage prozessiereKiBild(const QImage &quellBild)
{
    if (quellBild.isNull()) return QImage();

    int w = quellBild.width();
    int h = quellBild.height();
    int minDim = std::min(w, h);
    int startX = (w - minDim) / 2;
    int startY = (h - minDim) / 2;

    QImage quadrat = quellBild.copy(startX, startY, minDim, minDim);
    QImage kiBild = quadrat.scaled(224, 224, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    kiBild = kiBild.convertToFormat(QImage::Format_Grayscale8);

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


// =========================================================
// MAINWINDOW
// =========================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    FreeConsole();
    AllocConsole();
    std::freopen("CONOUT$", "w", stdout);
    std::freopen("CONOUT$", "w", stderr);

    ui->setupUi(this);

    // =====================================================
    // Dataset Pfad
    // =====================================================
    QDir baseDir(QCoreApplication::applicationDirPath());
    baseDir.cdUp();
    baseDir.mkdir("dataset");
    m_datasetPath = baseDir.absoluteFilePath("dataset");

    cout << "Zentraler Speicherpfad: " << m_datasetPath.toStdString() << endl;

    ladeLabelsKonfiguration();

    ui->labelCameraLive->setScaledContents(true);
    ui->labelScreenshot->setScaledContents(true);
    ui->labelKiInput->setScaledContents(false);

    // =====================================================
    // PYTHON EMBED INIT (NEU)
    // =====================================================
    Py_Initialize();
    if (!Py_IsInitialized()) {
        cout << "[PYTHON] Initialisierung fehlgeschlagen!" << endl;
    } else {
        cout << "[PYTHON] Interpreter aktiv" << endl;
    }

    // =====================================================
    // Kamera Setup
    // =====================================================
    QCameraDevice standardCam = QMediaDevices::defaultVideoInput();

    if (standardCam.isNull()) {
        m_camera = new QCamera(this);
    } else {
        m_camera = new QCamera(standardCam, this);
    }

    m_imageCapture = new QImageCapture(this);

    const QList<QCameraFormat> formats = standardCam.videoFormats();
    if (!formats.isEmpty()) {
        QCameraFormat selectedFormat = formats.first();

        for (const QCameraFormat &format : formats) {
            if (format.resolution().width() == 640 &&
                format.resolution().height() == 480) {
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

    // =====================================================
    // Live Stream Pipeline (unverändert)
    // =====================================================
    static QElapsedTimer kiIntervallTimer;
    kiIntervallTimer.start();

    connect(m_sink, &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame &frame)
    {
        QImage liveBild = frame.toImage().convertToFormat(QImage::Format_RGB32);

        if (!liveBild.isNull()) {
            ui->labelCameraLive->setPixmap(QPixmap::fromImage(liveBild));

            if (m_kiErkennungAktiv && m_kiProzess &&
                m_kiProzess->state() == QProcess::Running)
            {
                if (kiIntervallTimer.elapsed() >= 100) {
                    kiIntervallTimer.restart();

                    QImage kiBild = prozessiereKiBild(liveBild);

                    QByteArray rawBytes(
                        reinterpret_cast<const char*>(kiBild.bits()),
                        224 * 224
                    );

                    QByteArray b64Text = rawBytes.toBase64();

                    m_kiProzess->write("DATA:" + b64Text + "\n");
                    m_kiProzess->waitForBytesWritten(1);
                }
            }
        }
    });

    // =====================================================
    // Screenshot Handling
    // =====================================================
    connect(m_imageCapture, &QImageCapture::imageCaptured,
            this, [this](int id, const QImage &preview)
    {
        Q_UNUSED(id);
        if (preview.isNull()) return;

        m_currentImage = preview;
        ui->labelScreenshot->setPixmap(QPixmap::fromImage(m_currentImage));

        QImage kiBild = prozessiereKiBild(m_currentImage);
        ui->labelKiInput->setPixmap(QPixmap::fromImage(kiBild));
    });

    QTimer::singleShot(300, this, [this]() {
        if (m_camera) {
            m_camera->start();
        }
    });

    ui->progressBar->setValue(0);

    // =====================================================
    // QProcess bleibt erstmal bestehen (noch nicht ersetzt)
    // =====================================================
    m_kiProzess = new QProcess(this);

    connect(m_kiProzess, &QProcess::readyReadStandardOutput,
            this, [this]() {
        QString ausgabe = m_kiProzess->readAllStandardOutput().trimmed();
        QStringList zeilen = ausgabe.split("\n");

        for (const QString &zeile : std::as_const(zeilen)) {
            if (zeile.startsWith("RESULT:")) {
                ui->statusbar->showMessage("KI Live: " + zeile.mid(7).trimmed());
            }
        }
    });

    connect(m_kiProzess, &QProcess::readyReadStandardError,
            this, [this]() {
        QString fehler = m_kiProzess->readAllStandardError().trimmed();
        cout << "[PYTHON KI FEHLER]: " << fehler.toStdString() << endl;
    });
}
// mainwindow:mainwindow



//========================================
// ***SNIP***
//========================================

//========================================
// ***SNIP***
//========================================



MainWindow::~MainWindow()
{
    if (m_camera)
        m_camera->stop();

    delete ui;

    // =====================================================
    // Python sauber beenden (muss zu Teil 1 passen)
    // =====================================================
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
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
    if (m_camera && m_camera->isActive() &&
        m_imageCapture && m_imageCapture->isReadyForCapture())
    {
        m_imageCapture->capture();
        cout << "Hardware-Screenshot vom Live-Stream angefordert." << endl;

        QPixmap currentPixmap = ui->labelCameraLive->pixmap();
        if (!currentPixmap.isNull()) {
            QString fileName = m_datasetPath + "/screenshot_stream.png";
            currentPixmap.save(fileName, "PNG");
        }
    }
    // FALL 2: Standbild / Pausenmodus
    else
    {
        QPixmap currentPixmap = ui->labelCameraLive->pixmap();
        if (!currentPixmap.isNull()) {
            m_currentImage = currentPixmap.toImage();
            ui->labelScreenshot->setPixmap(currentPixmap);

            QString fileName = m_datasetPath + "/screenshot_pause.png";
            currentPixmap.save(fileName, "PNG");

            QImage kiBild = prozessiereKiBild(m_currentImage);
            ui->labelKiInput->setPixmap(QPixmap::fromImage(kiBild));
        }
        else {
            cout << "Fehler: Kein Standbild im Kamera-Label vorhanden." << endl;
        }
    }
}




// =========================================================
// OPTIMIERT: Speichert KI Bild mit ID-Maske
// =========================================================
void MainWindow::on_btnSaveKIpng_clicked()
{
    if (ui->labelScreenshot->pixmap().isNull()) {
        QMessageBox::warning(this,
                             "Fehler",
                             "Kein Standbild im 2. Fenster vorhanden!");
        return;
    }

    if (m_klassenNamen.empty()) {
        QMessageBox::critical(this,
                              "Fehler",
                              "Keine Muster-Klassen geladen!");
        return;
    }

    QString spickzettel =
        "<b>Verfügbare Muster-IDs:</b><br><br><table>";

    int col = 0;

    for (auto const& [id, name] : m_klassenNamen) {
        if (col == 0) spickzettel += "<tr>";

        spickzettel += QString("<td><b>%1:</b> %2</td>").arg(id).arg(name);
        col++;

        if (col >= 2) {
            spickzettel += "</tr>";
            col = 0;
        }
    }

    if (col != 0) spickzettel += "</tr>";
    spickzettel += "</table><br>ID Eingabe:";

    QDialog dialog(this);
    dialog.setWindowTitle("Multi-Muster Klassifizierung");

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    QLabel *textLabel = new QLabel(spickzettel, &dialog);
    textLabel->setTextFormat(Qt::RichText);
    mainLayout->addWidget(textLabel);

    QLineEdit *customInput = new QLineEdit(&dialog);
    customInput->setPlaceholderText("z.B. 1 5 14");
    mainLayout->addWidget(customInput);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *okButton = new QPushButton("OK", &dialog);
    QPushButton *cancelButton = new QPushButton("Abbrechen", &dialog);

    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    QStringList idList =
        customInput->text().split(QRegularExpression("\\s+"),
                                  Qt::SkipEmptyParts);

    std::vector<int> validIds;
    QString suffix;

    for (const QString &idStr : idList) {
        bool ok;
        int id = idStr.toInt(&ok);

        if (!ok || m_klassenNamen.find(id) == m_klassenNamen.end()) {
            QMessageBox::critical(this,
                                  "Fehler",
                                  "Ungültige ID: " + idStr);
            return;
        }

        validIds.push_back(id);
        suffix += "_ID" + QString::number(id);
    }

    if (validIds.empty())
        return;

    QImage raw = ui->labelScreenshot->pixmap().toImage();
    QImage kiBild = prozessiereKiBild(raw);

    QString ts = QDateTime::currentDateTime()
                     .toString("yyyyMMdd_hhmmss_zzz");

    QString defaultName =
        QString("%1/ZZKI%2_%3.png")
            .arg(m_datasetPath, suffix, ts);

    QString file = QFileDialog::getSaveFileName(
        this,
        "Speichern",
        defaultName,
        "PNG (*.png)");

    if (!file.isEmpty()) {
        if (!file.contains("_ID")) {
            QMessageBox::critical(this,
                                  "Fehler",
                                  "ID-Maske fehlt im Dateinamen!");
            return;
        }

        kiBild.save(file, "PNG");
    }
}


// =========================================================
// DATASET LOAD + CSV EXPORT
// =========================================================
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
    printf("on_btnExportCSV_clicked\n");

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
    printf("getNumClasses gestartet\n");
    if (m_klassenNamen.empty()) return 0;
    return m_klassenNamen.rbegin()->first + 1;
}



// Liest die labels.cfg Datei zeilenweise aus dem Quellcode-Verzeichnis ein
void MainWindow::ladeLabelsKonfiguration()
{
    printf("ladeLabelsKonfiguration\n");
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





//========================================
// ***SNIP***
//========================================

//========================================
// ***SNIP***
//========================================





// ========================================
// KI RUN (angepasst: kein initPython Mischbetrieb mehr)
// ========================================
void MainWindow::on_btnKiRun_clicked()
{
    printf("on_btnKiRun_clicked\n");

    ui->progressBar->setValue(0);

    if (m_kiErkennungAktiv)
        return;

    // WICHTIG: Python wurde bereits im Konstruktor initialisiert
    if (!Py_IsInitialized()) {
        QMessageBox::critical(this, "Fehler", "Python Interpreter nicht aktiv!");
        return;
    }

    // optional: Inference-Funktion vorbereiten (falls genutzt)
    if (!m_pyInferFunc) {
        cout << "[PYTHON] Hinweis: Inference Funktion noch nicht gebunden." << endl;
    }

    m_kiErkennungAktiv = true;

    ui->statusbar->showMessage("KI läuft (Python Embedded)", 3000);
    ui->btnKiRun->setEnabled(false);
    ui->btnKiStop->setEnabled(true);
}
// on_btnKiRun_clicked

// ========================================
// KI STOP
// ========================================
void MainWindow::on_btnKiStop_clicked()
{
    ui->progressBar->setValue(0);

    if (!m_kiErkennungAktiv)
        return;

    cout << "Stoppe KI..." << endl;

    m_kiErkennungAktiv = false;

    if (m_kiProzess) {
        m_kiProzess->close();
    }

    ui->statusbar->showMessage("KI gestoppt", 3000);
    ui->btnKiRun->setEnabled(true);
    ui->btnKiStop->setEnabled(false);
}
// on_btnKIStop_clicked

// ========================================
// TRAINING (unverändert strukturell, nur robust gehalten)
// ========================================
void MainWindow::on_btnKiTrain_clicked()
{
    cout << ">>> on_btnKiTrain_clicked()" << endl;

    ui->progressBar->setValue(0);

    if (m_kiTrainingAktiv) {
        QMessageBox::warning(this, "Training läuft bereits",
                             "Bitte warten bis aktuelles Training fertig ist.");
        return;
    }

    if (m_kiErkennungAktiv) {
        QMessageBox::warning(this, "Erkennung aktiv",
                             "Bitte zuerst KI stoppen.");
        return;
    }

    QDir baseDir(QCoreApplication::applicationDirPath());
    QDir projDir = baseDir;
    projDir.cdUp();

    QStringList moeglichePfade;
    moeglichePfade << projDir.absoluteFilePath("train_tensorFlow_net.py");
    moeglichePfade << projDir.absoluteFilePath("train_cnn.py");
    moeglichePfade << baseDir.absoluteFilePath("train_tensorFlow_net.py");
    moeglichePfade << baseDir.absoluteFilePath("train_cnn.py");

    QString skript;

    for (const QString &p : moeglichePfade) {
        if (QFile::exists(p)) {
            skript = p;
            break;
        }
    }

    if (skript.isEmpty()) {
        QMessageBox::critical(this, "Fehler", "Kein Training-Skript gefunden!");
        return;
    }

    m_kiTrainingAktiv = true;
    ui->btnKiTrain->setEnabled(false);

    ui->statusbar->showMessage("Training gestartet...", 5000);

    cout << "Training gestartet: " << skript.toStdString() << endl;

    QProcess *proc = new QProcess(this);

    connect(proc, &QProcess::readyReadStandardOutput, this, [proc]() {
        cout << proc->readAllStandardOutput().toStdString();
    });

    connect(proc, &QProcess::readyReadStandardError, this, [proc]() {
        cout << "[TRAIN] " << proc->readAllStandardError().toStdString();
    });

    connect(proc, &QProcess::finished, this,
            [this, proc](int code, QProcess::ExitStatus)
    {
        m_kiTrainingAktiv = false;
        ui->btnKiTrain->setEnabled(true);

        if (code == 0) {
            ui->statusbar->showMessage("Training fertig", 4000);
        } else {
            ui->statusbar->showMessage("Training Fehler", 4000);
        }

        proc->deleteLater();
    });

    QStringList args;
    args << "-u" << skript;

#ifdef Q_OS_WIN
    proc->start("python", args);
#else
    proc->start("python3", args);
#endif
}

//========================================
// eof
//========================================


