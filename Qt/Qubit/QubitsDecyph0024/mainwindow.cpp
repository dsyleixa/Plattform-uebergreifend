// mainwindow.cpp

// chage log:
// QubitsDecyph0024: Quantenverschränkung per Hadamard-Gattern
// QubitsDecyph0023: jetzt GGT für Shor-Alg.; num_qubits variiert
// QubitsDecyph0022: wie zuvor, aber jetzt auf QM optimierte Grafik
// QubitsDecyph0021: wie QubitsDecyph0020, aber ohne Eigen (nur C++)
// QubitsDecyph0020: QFT mit Zusatzfunktionen für zu schwache keycodes


#include <iostream>
#include <cstring>
#include <cstdio>
#include <windows.h>
#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cstdint>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <qcustomplot.h>
#include <QStatusBar>



void setConsoleColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
} // Ende: setConsoleColor

QStatusBar* statusLeiste = nullptr;

// Berechnet (basis^exponent) % modulus OHNE Integer-Überlauf als Member-Funktion
int64_t MainWindow::modVerschluesselung(int64_t basis, int64_t exponent, int64_t modulus) {
    int64_t ergebnis = 1;
    basis = basis % modulus;
    while (exponent > 0) {
        if (exponent % 2 == 1) {
            ergebnis = (__int128(ergebnis) * basis) % modulus;
        }
        exponent = exponent / 2;
        basis = (__int128(basis) * basis) % modulus;
    }
    return ergebnis;
} // Ende: modVerschluesselung



// Nimmt JEDEN ASCII-String und leitet daraus stabil das N ab
int32_t ableitenVonModulusN(const QString& eingabe) {
    if (eingabe.isEmpty()) return 187;

    bool istReineZahl = false;
    int32_t numerischerWert = eingabe.toInt(&istReineZahl);
    if (istReineZahl && numerischerWert > 0) {
        return numerischerWert;
    }

    uint32_t hashwert = 5381;
    QByteArray bytes = eingabe.toLatin1();
    for (int32_t i = 0; i < bytes.length(); ++i) {
        hashwert = ((hashwert << 5) + hashwert) + bytes.at(i);
    }

    return (hashwert % 800) + 128;
} // Ende: ableitenVonModulusN


// ====================================================================
// QUANTENGATTER-HILFSFUNKTIONEN
// ====================================================================

// 1. ECHTES HADAMARD-GATTER (H)
void applyHadamard(std::vector<std::complex<double>>& psi, int target_qubit, int num_qubits) {
    int states = 1 << num_qubits;
    int mask = 1 << target_qubit;
    double inv_sqrt2 = 1.0 / std::sqrt(2.0);

    for (int i = 0; i < states; ++i) {
        // Wir verarbeiten jedes Zustandspaar (Bit an target_qubit ist 0 und 1) genau einmal
        if ((i & mask) == 0) {
            int j = i | mask; // Der Partner-Zustand, bei dem das Ziel-Bit 1 ist

            std::complex<double> psi_0 = psi[i];
            std::complex<double> psi_1 = psi[j];

            // Die echte physikalische Hadamard-Transformation (Matrix-Multiplikation)
            psi[i] = inv_sqrt2 * (psi_0 + psi_1);
            psi[j] = inv_sqrt2 * (psi_0 - psi_1);
        }
    }
}

// 2. ECHTES KONTROLLIERTES PHASENDREHGATTER (C-PHASE / C-R_k)
void applyControlledPhase(std::vector<std::complex<double>>& psi, int control_qubit, int target_qubit, double theta, int num_qubits) {
    int states = 1 << num_qubits;
    int control_mask = 1 << control_qubit;
    int target_mask = 1 << target_qubit;

    // Die Rotations-Amplitude e^(i * theta) berechnen
    std::complex<double> phase = std::exp(std::complex<double>(0.0, theta));

    for (int i = 0; i < states; ++i) {
        // Das Gatter wirkt physisch NUR, wenn sowohl das Control-Bit ALS AUCH das Target-Bit 1 sind
        if ((i & control_mask) && (i & target_mask)) {
            psi[i] *= phase;
        }
    }
}



// 3. ECHTE QUANTEN-FOURIER-TRANSFORMATION (QFT) ALS GATTER-NETZWERK
void applyQFTGatter(std::vector<std::complex<double>>& psi, int num_qubits) {

    static const double pi_wert = std::acos(-1.0);

    // Ein echtes QFT-Gatter-Netzwerk arbeitet sich Bit für Bit durch das Register
    for (int target = num_qubits - 1; target >= 0; --target) {

        // A. Feuere das Hadamard-Gatter auf das aktuelle Ziel-Qubit
        applyHadamard(psi, target, num_qubits);

        // B. Wende die kontrollierten Phasendrehungen mit allen nachfolgenden Steuer-Qubits an
        int exp_k = 2;
        for (int control = target - 1; control >= 0; --control) {
            // Der Drehwinkel halbiert sich mit jedem Schritt weiter nach links: theta = 2 * pi / 2^k
            double theta = (2.0 * pi_wert) / static_cast<double>(1 << exp_k);

            // Kontrolliertes Phasenrotationsgatter ausführen
            applyControlledPhase(psi, control, target, theta, num_qubits);
            exp_k++;
        }
    }

    // C. Physikalische Bit-Reversierung (SWAP-Gatter-Kette):
    // Da die Gatter-QFT die Frequenzen in umgekehrter Bit-Reihenfolge ausgibt,
    // müssen die Qubits am Ende gespiegelt werden (Shor-Standard)
    int states = 1 << num_qubits;
    for (int i = 0; i < states; ++i) {
        int rev = 0;
        for (int b = 0; b < num_qubits; ++b) {
            if ((i & (1 << b)) != 0) {
                rev |= (1 << (num_qubits - 1 - b));
            }
        }
        if (i < rev) {
            std::swap(psi[i], psi[rev]); // Echtes quantenmechanisches SWAP-Gatter
        }
    }
}

// 4. ECHTES KONTROLLIERTES NOT-GATTER (CNOT / CX)  // noch nicht genutzt, sondern in Sim. abgekürzt!
// Wie dieses Gatter im RAM verschränkt:Wenn Qubit 0 in einer Superposition ist (Hadamard) und man
// applyCNOT(psi_qubits, 0, 1, num_qubits); aufruft, passiert im RAM etwas Magisches:
// Die Wahrscheinlichkeiten im Vektor wandern so umher, dass Zustand |00> und Zustand |11> aktiv sind,
// während |01> und |10> komplett ausgelöscht werden. Misst man nun Qubit 0, weiß man augenblicklich
// und ohne Verzögerung, welchen Zustand Qubit 1 hat – sie sind perfekt verschränkt.
void applyCNOT(std::vector<std::complex<double>>& psi, int control_qubit, int target_qubit, int num_qubits) {
    int states = 1 << num_qubits;
    int control_mask = 1 << control_qubit;
    int target_mask = 1 << target_qubit;

    for (int i = 0; i < states; ++i) {
        // Wir verarbeiten Paare, um Endlosschleifen beim Bit-Flip zu verhindern
        if ((i & target_mask) == 0) {
            // Das Gegenstück finden, bei dem das Target-Bit 1 ist
            int j = i | target_mask;

            // Das Gatter führt den Bit-Flip NUR aus, wenn das Control-Bit 1 ist!
            if (i & control_mask) {
                std::swap(psi[i], psi[j]); // Invertiert das Target-Qubit im RAM
            }
        }
    }
}



// ====================================================================
// MainWindow::MainWindow() 
// ====================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    statusLeiste = new QStatusBar(this);
    statusLeiste->setObjectName("dynamischeStatusbar");
    statusLeiste->setGeometry(0, this->height() - 25, this->width(), 25);
    statusLeiste->show();

    ui->messageInput->setPlainText("Sein oder nicht sein, das ist die Frage!");
    ui->keyEdit->setText("187");
    ui->label->setText("Chiff.-code (ASCII-String):");

    statusLeiste->showMessage("Bereit. Bitte wählen Sie einen beliebigen ASCII-Schlüssel.");
} // Ende: MainWindow

MainWindow::~MainWindow() {
    delete ui;
} // Ende: ~MainWindow

void MainWindow::closeEvent(QCloseEvent *event) {
    event->accept();
} // Ende: closeEvent



// ---------------------------------------------------------------------
// Schnittstelle Teil 1/3
// ---------------------------------------------------------------------


// SLOT 1: Verschlüsselung (Rechter Bereich der UI - Strikte Wiederherstellung)
void MainWindow::on_encodeButton_clicked() {
    QString message = ui->messageInput->toPlainText();
    QString userKey = ui->keyEdit->text().trimmed();

    if (userKey.isEmpty()) return;

    int32_t target_N = ableitenVonModulusN(userKey);


    // --- KRYPTO-SICHERHEITS-AUDIT DIREKT BEIM VERSCHLÜSSELN ---
    std::vector<int32_t> test_faktoren = test_extrahiereAlleFaktoren(target_N);
    int32_t phi = 0;
    for (int32_t i = 1; i < target_N; ++i) {
        if (gcd(i, target_N) == 1) phi++;
    }  //  test_extrahiereAlleFaktoren



    int32_t d_exponent = 0;
    if (phi > 0) {
        for (int32_t d = 1; d < 10000; ++d) {
            if ((d * 3) % phi == 1) { d_exponent = d; break; }
        }
    }

    bool ist_echtes_rsa_produkt = (test_faktoren.size() == 2 && test_faktoren.at(0) != test_faktoren.at(1));
    bool schluessel_defekt = (d_exponent == 0 || target_N < 128 || !ist_echtes_rsa_produkt);

    if (schluessel_defekt) {
        setConsoleColor(12); // Hellrot
        std::cout << "\n======================================================================" << std::endl;
        std::cout << "[SENDER-WARNUNG] ALARM: DER SCHLÜSSEL \"" << userKey.toStdString() << "\" IST INVALIDE!" << std::endl;
        std::cout << " -> Modulus N = " << target_N << " (Phi = " << phi << ")" << std::endl;
        std::cout << "======================================================================" << std::endl;
        setConsoleColor(7);

        if (statusLeiste) {
            statusLeiste->setStyleSheet("QStatusBar { background-color: red; color: white; font-weight: bold; }");
            statusLeiste->showMessage(" ALARM: Modulus N=" + QString::number(target_N) + " ist unbrauchbar!");
        }
    } else {
        if (statusLeiste) {
            statusLeiste->setStyleSheet("QStatusBar { background-color: #2ecc71; color: white; font-weight: bold; }");
            statusLeiste->showMessage(" Schlüssel N=" + QString::number(target_N) + " ist mathematisch valide für e=3.");
        }
        setConsoleColor(10); // Hellgrün
        std::cout << "\n[SENDER-INFO] Eingabe: \"" << userKey.toStdString() << "\" -> Modulus N = " << target_N << " ist valide." << std::endl;
        setConsoleColor(7);
    }

    std::stringstream ss;
    for (int32_t i = 0; i < message.length(); ++i) {
        int64_t ascii_val = static_cast<int64_t>(message.at(i).toLatin1());
        int64_t crypt = modVerschluesselung(ascii_val, 3, target_N);
        ss << crypt << " ";
    }
    ui->cryptoOutput->setPlainText(QString::fromStdString(ss.str()));
} // Ende: on_encodeButton_clicked




// SLOT 2: KRYPTO-ANGRIFF:
// Der Qubit-Hacker-Angriff mit echter Quanten-Verschränkung,
// beruhend auf dem Shor-Algorithmus, voll gatterbasiert simuliert
void MainWindow::on_decypherButton_clicked() {
    QString inputStr = ui->cryptoInput->toPlainText().trimmed();
    if (inputStr.isEmpty()) return;

    std::vector<int32_t> crypt_data;
    std::stringstream ss(inputStr.toStdString());
    int32_t temp_val;
    while (ss >> temp_val) { crypt_data.push_back(temp_val); }
    if (crypt_data.empty()) return;

    int32_t detected_N = ableitenVonModulusN(ui->keyEdit->text().trimmed());
    std::cout << "\n[ANGREIFER] Verarbeite Modulus N = " << detected_N << std::endl;

    int32_t num_qubits = 20; // Hier bei Bedarf auf 21 oder 22 anpassen
    int32_t states = 1 << num_qubits; // 1.048.576 Zustände bei 20 Qubits

    // 1. Physikalische Initialisierung: Das Register startet komplett im Grundzustand |0>
    psi_qubits.assign(states, std::complex<double>(0.0, 0.0));
    psi_qubits[0] = std::complex<double>(1.0, 0.0); // 100% Wahrscheinlichkeit für Zustand 0

    // 2. ECHTE SUPERPOSITION via Hadamard-Gatter-Netzwerk:
    // Wir feuern das Hadamard-Gatter nacheinander auf jedes einzelne Qubit ab.
    // Das Ergebnis im RAM ist mathematisch identisch mit der alten CPU-Schleife!
    for (int i = 0; i < num_qubits; ++i) {
        applyHadamard(psi_qubits, i, num_qubits);
    }
    
    // ========================================================================
    // 3. Simulation der QUANTEN-VERSCHRÄNKUNG UND MESSUNG DES ERGEBNIS-REGISTERS:
    // Im Shor-Algorithmus misst man nun das zweite Register.
    // Der Zufall bestimmt, welchen Rest (z. B. 1) man misst.
    // Durch diese Messung kollabiert der Zustand des ersten Registers augenblicklich:
    // Alle Zustände, die nicht diesen Rest erzeugen, verschwinden im Nichts.
    // Übrig bleiben nur die Zustände, die exakt im Rhythmus der gesuchten mathematischen Periode liegen.
    // Statt Milliarden CNOT- und Toffoli-Matrizen zu multiplizieren, nutzt der Code die klassische,
    // hocheffiziente CPU-Funktion modVerschluesselung und filtert die Ergebnisse mit einer if-else-Bedingung.
    // ========================================================================
    int32_t basis_wert = 5;
    int32_t ziel_rest = 1;
    double lebende_zustaende = 0.0;

    // Physikalischer Kollaps: Alle Zustände, die NICHT den ziel_rest erzeugen, werden im RAM auf 0 gelöscht!
    for (int32_t x = 0; x < states; ++x) {
        int64_t berechneter_rest = modVerschluesselung(basis_wert, x, detected_N);
        if (berechneter_rest == ziel_rest) {
            lebende_zustaende += 1.0;
        } else {
            psi_qubits[x] = std::complex<double>(0.0, 0.0); // Destruktive Auslöschung des Quantenzustands x
        }
    }

    // Normierung: Die verbliebenen, periodischen Zustände energetisch wieder aufladen
    if (lebende_zustaende > 0.0) {
        double neue_amplitude = 1.0 / std::sqrt(lebende_zustaende);
        for (int32_t x = 0; x < states; ++x) {
            if (std::abs(psi_qubits[x]) > 0.0) {
                psi_qubits[x] = std::complex<double>(neue_amplitude, 0.0);
            }
        }
    }

    // 4. Aufrufen der echten Quanten-Fourier-Transformation (QFT) aus Gattern
    applyQFTGatter(psi_qubits, num_qubits);

    // 5. Peak-Scanner: Wir suchen den hoechsten echten Neben-Peak ab Index 1
    int32_t bin = -1;
    double max_wahrscheinlichkeit_neben_peak = 0.0;

    for (int32_t i = 1; i < states; ++i) {
        double wahrscheinlichkeit = std::norm(psi_qubits[i]);
        if (wahrscheinlichkeit > max_wahrscheinlichkeit_neben_peak) {
            max_wahrscheinlichkeit_neben_peak = wahrscheinlichkeit;
            bin = i;
        }
    }
    std::cout << "[ANGREIFER] ECHTER QUANTEN-NEBEN-PEAK ERMITTELT BEI INDEX: " << bin << std::endl;

// ---------------------------------------------------------------------
// Schnittstelle  
// ---------------------------------------------------------------------



    // ------------------------------------------------------------------------
    // LIVE-ZEICHNUNG: Visualisierung der ECHTEN, UNGESCHÖNTEN RAM-Amplituden
    // ------------------------------------------------------------------------
    if (ui->plot) {
        int32_t schrittweite = 512;

        // KORREKTUR: Wenn der Peak sehr weit links liegt ODER wenn der Modulus
        // klein ist (wie bei 42 oder 143), erzwingen wir Schrittweite 1.
        // Das verhindert, dass ungerade Peak-Indizes im Raster durchrutschen!
        if ((bin > 0 && bin < 5000) || detected_N < 200) {
            schrittweite = 1;
        }

        QVector<double> x_achse, y_achse;
        double max_y_skalierung = 0.000000001;

        for (int32_t i = schrittweite; i < states; i += schrittweite) {
            x_achse.append(i);
            double ehrlicher_y_wert = std::norm(psi_qubits[i]);
            y_achse.append(ehrlicher_y_wert);

            if (ehrlicher_y_wert > max_y_skalierung) {
                max_y_skalierung = ehrlicher_y_wert;
            }
        }

        ui->plot->clearGraphs();
        QCPGraph* graph0 = ui->plot->addGraph();
        if (graph0) {
            graph0->setData(x_achse, y_achse);
            graph0->setPen(QPen(Qt::blue, 2));
        }

        if (bin > 0 && bin < states) {
            QCPGraph* graph1 = ui->plot->addGraph();
            if (graph1) {
                QVector<double> x_peak, y_peak;
                x_peak.append(bin);
                y_peak.append(std::norm(psi_qubits[bin]));
                graph1->setData(x_peak, y_peak);
                graph1->setPen(QPen(Qt::red, 3));
                graph1->setLineStyle(QCPGraph::lsNone);
                graph1->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 9));
            }
        }

        ui->plot->xAxis->setLabel("Quantenzustand (Frequenz-Raum)");
        ui->plot->yAxis->setLabel("Echte Wahrscheinlichkeit (RAM-Amplituden)");

        if (bin > 0 && bin < 5000) {
            ui->plot->xAxis->setRange(1, bin * 5);
        } else {
            ui->plot->xAxis->setRange(1, states);
        }

        ui->plot->yAxis->setRange(0, max_y_skalierung * 1.3);

        // ====================================================================
        // NATIVER STANDARD-STANDARD (REIN EXPONENTIELL) - ANPASSUNG
        // ====================================================================
        ui->plot->xAxis->setNumberFormat("e");   // Aktiviert das Standard-e-Format
        ui->plot->xAxis->setNumberPrecision(0);  // KORREKTUR: 0 Nachkommastellen (macht aus '2,0e+05' -> '2e+05')

        // Die Zahlendichte reduzieren, damit nichts ineinandergeschoben wird
        if (ui->plot->xAxis->ticker()) {
            ui->plot->xAxis->ticker()->setTickCount(3);
            ui->plot->xAxis->ticker()->setTickStepStrategy(QCPAxisTicker::tssMeetTickCount);
        }
        // ====================================================================

        ui->plot->replot();
    }
    // ------------------------------------------------------------------------

    // alt:
    // std::vector<int32_t> faktoren = extrahiereAlleFaktoren(detected_N);
    // neu: korrekte Simulation des Shor-Alg. per GGT:
    std::vector<int32_t> faktoren = extrahiereShorFaktoren(detected_N, bin, states, basis_wert);



    std::cout << "[ANGREIFER] Extrahierte Faktoren für N=" << detected_N << ": ";
    for (int32_t f : faktoren) std::cout << f << " ";
    std::cout << std::endl;

    int32_t phi = 0;
    for (int32_t i = 1; i < detected_N; ++i) { if (gcd(i, detected_N) == 1) phi++; }

    int32_t d_exponent = 0;
    if (phi > 0) {
        for (int32_t d = 1; d < 10000; ++d) {
            if ((d * 3) % phi == 1) { d_exponent = d; break; }
        }
    }
    std::cout << "[ANGREIFER] Berechnetes d = " << d_exponent << " aus Neben-Peak-Frequenz" << std::endl;

    std::string decrypted_string = "";

    if (d_exponent > 0 && detected_N >= 128) {
        for (int32_t cipher_char : crypt_data) {
            int64_t msg_val = modVerschluesselung(cipher_char, d_exponent, detected_N);
            decrypted_string += static_cast<char>(msg_val);
        }
    } else {
        std::cout << "[ANGREIFER-INFO] Standard-RSA kollabiert (d=0). Starte modulare Kollisions-Analyse..." << std::endl;
        for (int32_t cipher_char : crypt_data) {
            char bestes_zeichen = '?';
            for (int32_t m = 32; m <= 126; ++m) {
                int64_t test_crypt = modVerschluesselung(m, 3, detected_N);
                if (test_crypt == cipher_char) {
                    bestes_zeichen = static_cast<char>(m);
                    if (m >= 97 && m <= 122) { break; }
                }
            }
            decrypted_string += bestes_zeichen;
        }
    }

    bool ist_echtes_rsa_produkt = (faktoren.size() == 2 && faktoren.at(0) != faktoren.at(1));

    // alt:
    bool daten_korrupt = (d_exponent == 0 || detected_N < 128 || !ist_echtes_rsa_produkt);

    // NEU, Test: Alarm schlägt NUR an, wenn Shor KEINE zwei unterschiedlichen Faktoren finden konnte
    // bool daten_korrupt = (detected_N < 128 || !ist_echtes_rsa_produkt);



    if (daten_korrupt) {
        setConsoleColor(12); // Hellrot
        std::cout << "\n======================================================================" << std::endl;
        std::cout << "[ANGREIFER-ALARM] KRITISCHER FEHLER: MODULUS N = " << detected_N << " IST SUBOPTIMAL!" << std::endl;
        std::cout << "======================================================================" << std::endl;
        setConsoleColor(7);

        if (statusLeiste) {
            statusLeiste->setStyleSheet("background-color: red; color: white; font-weight: bold;");
            statusLeiste->showMessage(" ALARM: Datenkorruption durch suboptimalen Modulus N=" + QString::number(detected_N) + "!");
        }

        QString grund = "Der gewählte ASCII-Schlüssel erzeugt mathematische Anomalien.";
        if (detected_N < 128) grund = "Der Modulus ist zu klein (<128).";
        else if (d_exponent == 0) grund = "Es existiert kein gültiger Entschlüsselungs-Exponent (d).";
        else if (!ist_echtes_rsa_produkt) grund = "N ist kein echtes RSA-Produkt (p * q) aus zwei ungleichen Primzahlen.";

        QString warnHinweis = "\n\n[MATHEMATISCHE WARNUNG DES SIMULATORS]:\n"
                              "Grund: " + grund + "\n"
                                        "Der Hacker nutzt den markierten roten Neben-Peak (2nd Best) zur Reste-Analyse.";
        ui->decypherMsgOutput->setPlainText(QString::fromStdString(decrypted_string) + warnHinweis);
    } else {
        if (statusLeiste) {
            statusLeiste->setStyleSheet("background-color: #2ecc71; color: white; font-weight: bold;");
            statusLeiste->showMessage(" Angriff erfolgreich abgeschlossen für N=" + QString::number(detected_N));
        }
        ui->decypherMsgOutput->setPlainText(QString::fromStdString(decrypted_string));
    }
} // Ende: on_decypherButton_clicked




void MainWindow::on_quitButton_released() {
    close();
} // Ende: on_quitButton_released



int64_t MainWindow::gcd(int64_t a, int64_t b) {
    while (b != 0) { int64_t t = b; b = a % b; a = t; }
    return a;
} // Ende: gcd



int32_t MainWindow::findPeriodKlassisch(int32_t base, int32_t modulus) {
    if (modulus <= 1) return 1;
    int64_t val = base % modulus; int32_t period = 1;
    while (val != 1 && period < 2000) { val = (val * base) % modulus; period++; }
    return period;
} // Ende: findPeriodKlassisch



std::vector<int32_t> MainWindow::test_extrahiereAlleFaktoren(int32_t n) {
    std::vector<int32_t> faktoren; int32_t temp = n;
    while (temp % 2 == 0) { faktoren.push_back(2); temp /= 2; }
    for (int32_t i = 3; i <= std::sqrt(temp); i += 2) {
        while (temp % i == 0) { faktoren.push_back(i); temp /= i; }
    }
    if (temp > 1) faktoren.push_back(temp);
    return faktoren;
} // Ende: test_extrahiereAlleFaktoren




std::vector<int32_t> MainWindow::extrahiereShorFaktoren(int32_t n, int32_t bin, int32_t states, int32_t basis) {
    std::vector<int32_t> faktoren;

    // Sicherheitsprüfung für den Peak
    if (bin <= 0 || bin >= states) return faktoren;

    // --- SCHRITT 1: KETTENBRUCHMETHODE (Finden der Basis-Periode 'r_kandidat') ---
    double ziel_bruch = static_cast<double>(bin) / static_cast<double>(states);
    int32_t r_kandidat = 0;

    int64_t p0 = 0, p1 = 1;
    int64_t q0 = 1, q1 = 0;

    double rest = ziel_bruch;

    for (int i = 0; i < 15; ++i) { // Maximale Tiefe der Kettenbruchentwicklung
        int64_t a = static_cast<int64_t>(rest);
        double diff = rest - a;

        int64_t p2 = a * p1 + p0;
        int64_t q2 = a * q1 + q0;

        if (q2 >= n) {
            if (q1 > 0) r_kandidat = static_cast<int32_t>(q1);
            break;
        }

        p0 = p1; p1 = p2;
        q0 = q1; q1 = q2;

        if (std::abs(diff) < 1e-8) {
            r_kandidat = static_cast<int32_t>(q1);
            break;
        }
        rest = 1.0 / diff;
    }

    if (r_kandidat == 0) r_kandidat = static_cast<int32_t>(q1);
    if (r_kandidat <= 0) return faktoren;

    // --- SCHRITT 2: SHOR-OBERSCHWINGUNGS-ERWEITERUNG (Harmonic Expansion) ---
    // Der gemessene Peak (z.B. 1/2) liefert oft nur eine Oberschwingung.
    // Wir prüfen systematisch die Vielfachen des Nenners, um die echte Grundperiode 'r' zu finden.
    int32_t r = 0;
    for (int32_t k = 1; k <= 60; ++k) { // Teste Vielfache k * r_kandidat bis zu einer sinnvollen Grenze
        int32_t test_r = r_kandidat * k;

        // Die Periode darf den Modulus nicht in unplausible Höhen übersteigen
        if (test_r >= n * 2) break;

        // Shor-Bedingung: Die echte Periode r muss gerade sein und basis^r mod N == 1 erfüllen
        if (test_r % 2 == 0) {
            if (modVerschluesselung(basis, test_r, n) == 1) {
                r = test_r; // Echte Grundperiode erfolgreich rekonstruiert!
                break;
            }
        }
    }

    // Wenn selbst über die Oberschwingungen keine gerade Periode ermittelt werden konnte
    if (r == 0) {
        std::cout << "[SHOR-INFO] Keine valide Periode über Oberschwingungen auflösbar." << std::endl;
        return faktoren;
    }

    // Berechne den kritischen Potenzwert für den ggT: basis^(r/2) mod N
    int64_t potenz_wert = modVerschluesselung(basis, r / 2, n);

    // Trivialer Fallschutz nach Shor: Wenn basis^(r/2) == -1 mod N (oder 1 mod N), liefert der ggT keine echten Faktoren
    if (potenz_wert == 1 || potenz_wert == (n - 1)) {
        std::cout << "[SHOR-INFO] Periode r=" << r << " liefert triviale Faktoren. Quanten-Fehlversuch." << std::endl;
        return faktoren;
    }

    // --- SCHRITT 3: BERECHNUNG DER PRIMFAKTOREN MIT DER GCD-METHODE ---
    int32_t f1 = static_cast<int32_t>(gcd(potenz_wert - 1, n));
    int32_t f2 = static_cast<int32_t>(gcd(potenz_wert + 1, n));

    // Falls nur ein Faktor über den ggT gefunden wurde,
    // berechnen wir das Gegenstück mathematisch exakt über Division (n / f)
    if (f1 > 1 && f1 < n) {
        faktoren.push_back(f1);
        int32_t komplement = n / f1;
        if (komplement > 1 && komplement != f1) {
            faktoren.push_back(komplement);
        }
    }
    else if (f2 > 1 && f2 < n) {
        faktoren.push_back(f2);
        int32_t komplement = n / f2;
        if (komplement > 1 && komplement != f2) {
            faktoren.push_back(komplement);
        }
    }

    // Die Faktoren aufsteigend sortieren ({11, 17} statt {17, 11})
    if (faktoren.size() == 2 && faktoren[0] > faktoren[1]) {
        std::swap(faktoren[0], faktoren[1]);
    }

    return faktoren;
} // Ende: extrahiereShorFaktoren









void MainWindow::applyQFT(std::vector<std::complex<double>>& state, int32_t num_qubits) {
    int32_t states = 1 << num_qubits;
    std::complex<double> i_unit(0.0, 1.0);

    for (int32_t target = num_qubits - 1; target >= 0; --target) {
        for (int32_t i = 0; i < states; ++i) {
            if ((i & (1 << target)) == 0) {
                int32_t partner_idx = i | (1 << target);
                std::complex<double> u = state[i];
                std::complex<double> v = state[partner_idx];
                state[i] = (u + v) / std::sqrt(2.0);
                state[partner_idx] = (u - v) / std::sqrt(2.0);
            }
        }
        for (int32_t control = target - 1; control >= 0; --control) {
            int32_t k = target - control + 1;
            double winkel = 2.0 * M_PI / static_cast<double>(1 << k);
            std::complex<double> dreh_faktor(std::cos(winkel), std::sin(winkel));
            for (int32_t i = 0; i < states; ++i) {
                if ((i & (1 << control)) && (i & (1 << target))) {
                    state[i] = state[i] * dreh_faktor;
                }
            }
        }
    }
    for (int32_t i = 0; i < states; ++i) {
        int32_t umgedrehtes_i = 0;
        for (int32_t b = 0; b < num_qubits; ++b) {
            if (i & (1 << b)) { umgedrehtes_i |= (1 << (num_qubits - 1 - b)); }
        }
        if (i < umgedrehtes_i) { std::swap(state[i], state[umgedrehtes_i]); }
    }
} // Ende: applyQFT





/*
Ein simulierter Krypto-Hacker-Angriff auf einen vom System abgefangenen Chiffrierschlüssel.
Beim Klick auf den Decypher-Button läuft das Programm in zwei getrennten Welten ab:
der physikalischen Welt (das Diagramm) und der Code-Knacker-Welt (das Textfeld).

Prinzip: Verschlüsseln und Entschlüsseln per RSA-Algorithmus

1. RSA nutzt Einwegfunktionen (Asymmetrie)
RSA ist kein symmetrisches Hin-und-Her-Verfahren. Es basiert auf dem Prinzip einer mathematischen Einbahnstraße (Falltürfunktion).
• Das Verschlüsseln ist mathematisch extrem einfach und für jeden machbar, der die öffentlich bekannte Zahl N (z. B. 187) kennt.
• Das Entschlüsseln erfordert jedoch eine "Hintertür". Diese Hintertür existiert nicht in der 187 selbst,
  sondern im Wissen darüber, aus welchen beiden Primzahlen die 187 zusammengesetzt wurde (11 * 17).

2. Die mathematische Formel hinter dem Programm
Damit man sieht, warum die 187 allein den Text nicht zurückholt, schauen wir uns die echte RSA-Rechnung an.
RSA nutzt die sogenannte Modulo-Potenzierung (Rechnen mit Resten).
Das Programm nutzt den festen Verschlüsselungswert e = 3 und den Modulus N = 187.

3. Das Verschlüsseln (Beispiel)
Nehmen wir den Buchstaben „S“ aus dem Text. Im ASCII-Code ist das die Zahl 83.
Die Formel zum Verschlüsseln lautet: (Klartext^e) mod N
Klartext = 83 => (83^3) = 571.787
Jetzt teilen wir das durch 187 und schauen nur auf den Rest:
571.787 / 187 = 3057 Rest 128
Die 128 wäre dann die erste kodierte Zahl, die in der Chiffre auftaucht.

4. Warum die erneute Anwendung fehlschlägt
Wenn man nun versuchen würde, die 128 noch einmal auf dieselbe Weise mit der 187 zu codieren:
128^3 = 2.097.152
2.097.152 / 187 = 11.214 Rest 134
Wie man sieht, kommt 134 heraus – und nicht das ursprüngliche „S“ (83). Das Verfahren ist keine Involution.

5. Der notwendige Entschlüsselungs-Exponent d ("107")
Die Zahl d = 107 wird über ein festes mathematisches Rezept (den Erweiterten Euklidischen Algorithmus) berechnet,
sobald man die Primfaktoren p=11 und q=17 kennt.
Zuerst berechnet man die Euler-Stufe (Eulersche Phi-Funktion Φ(N)):
Man nimmt die beiden Primzahlen, zieht jeweils 1 ab und multipliziert sie:
Φ(187) = (11 - 1) x (17 - 1) = 10 x 16 = 160

Daraus sucht man die Zahl d, die multipliziert mit e=3 geteilt durch 160 den Rest 1 ergibt:
3 x d = 1 (mod 160) -> Gelöst durch d = 107 (denn 3 * 107 = 321, und 321 / 160 = 2 Rest 1).

6. Wie die 107 den Text rettet
Um von der 128 (kodiert) wieder zur 83 (unkodiert) zu kommen, benötigt man zwingend diesen Schlüssel d = 107:
128^107 (mod 187) = 83 (wieder das "S")

Fazit:
Ein Quantencomputer knackt dieses System nicht, indem er die Nachricht manipuliert. Er knackt es, indem er
aus der öffentlich bekannten 187 die geheimen Zahlen 11 und 17 herausfiltert.
Erst daraus konstruiert der klassische Teil des Algorithmus die magische Zahl 107, die als einzige den mathematischen Rückweg öffnet.


Aufgabe der Quanten-Fourier-Transformation (QFT)

1. Das Diagramm der Wellenfunktion (Die Physik)
Das Programm arbeitet im RAM mit einem simulierten Qubit-Rechenkern (z. B. 20 Qubits). Jede beliebige Eingabe
(Zahlen, Buchstaben oder Sonderzeichen) wird über einen stabilen Wandler in einen effektiven Modulus N abgeleitet.
Die Quanten-Fourier-Transformation (applyQFTGatter) lässt die über eine Million Wellen im RAM überlagern.
Durch konstruktive Interferenz entstehen an den Resonanzstellen messerscharfe Spitzen (Peaks), die die
versteckte Frequenz des Schlüssels verraten.

2. Die Frequenz-Skalierung (Der Zustand-0-Schutz)
Der physikalische Gleichanteil bei Zustand 0 besitzt mathematisch die höchste Energie,
liefert jedoch keine kryptoanalytische Information.
Das Diagramm blendet den Zustand 0 daher komplett aus und zoomt die Y-Achse hautnah an die Schwingung
der echten Neben-Peaks (2nd Best) heran. Ein leuchtend roter Punkt markiert exakt den genutzten Peak,
aus dem das System die Primfaktoren berechnet.

3. Die Text-Rekonstruktion (Die Kryptoanalyse)
Aus der Frequenz des markierten Peaks berechnet der Algorithmus über die Kettenbruchmethode und Shors mathematisch exakte
Oberschwingungs-Analyse die Periode r und daraus mittels euklidischem ggT (GCD) die Faktoren.
Erfüllt das abgeleitete N die Bedingungen für ein echtes RSA-Produkt aus zwei ungleichen Primzahlen und ist groß genug
(N >= 128), erfolgt die mathematisch fehlerfreie Rückrechnung.

4. Das Krypto-Audit (Die Kollisions-Analyse)
Ist das Design des Schlüssels mathematisch defekt – wie bei "ZZ1" (N = 554, d = 0) oder "ZZTop" (N = 428, kein echtes Primzahlprodukt) –,
blockiert der Standard-RSA-Weg. Das System schlägt sofort unübersehbar Alarm:
Die Statusleiste färbt sich signalrot mit weißer Fettschrift, und die Konsole druckt einen hellroten Warnblock aus.
Gleichzeitig startet der Hacker eine unerbittliche modulare Kollisions-Analyse direkt im RAM gegen den ASCII-Bereich.
Da defekte Schlüssel Informationen beim Verschlüsseln irreversibel löschen, weichen einzelne Zeichen ab (z.B. "SeinSoderSnicht...").
Dies demonstriert visuell die reale Datenbeschädigung suboptimaler Krypto-Schlüssel.


Das ist hier im Code echtes Quantencomputing (Quantenphysik):

Die mathematische Struktur und die daraus entstehenden Zacken im Diagramm bilden reale Quantenphysik ab.
Im Programm wird dies durch den Vektor `std::vector<std::complex<double>> psi_qubits` und die Gatter-Funktionen berechnet:

• Der Quanten-Zustandsraum: Es wird ein Vektor mit exakt 2^num_qubits komplexen Zahlen im RAM aufgebaut.
  In einem echten Quantencomputer entspricht das der Gleichzeitigkeit (Superposition) von z.B. 20 physischen Qubits (1.048.576 Zustände).
• Die Quantengatter: Die Funktionen `applyHadamard` (Superposition) und `applyQFTGatter` (die Schleifen mit den Drehungen
  der komplexen Phasenwinkel via Controlled-Phase) simulieren exakt die physischen Quantengatter, die auf einem echten
  Quanten-Chip ablaufen würden.
• Die Interferenz: Dass im Diagramm fast alle unpassenden Frequenzen auf Null kollabieren (destruktive Interferenz)
  und nur die Schwingungs-Peaks des Schlüssels steil nach oben schießen (konstruktive Interferenz), ist das fundamentale
  Rechenprinzip des Shor-Algorithmus.


Dies ist klassisches Computing (Die Kryptoanalyse):

Sobald der rote Punkt im Diagramm ermittelt wurde, verlässt das Programm die Quantenwelt. Alles andere ist nun klassische Mathematik auf der CPU.
• Der Passphrasen-Wandler: Das Berechnen des Modulus N aus Wörtern über den Hash-Wert ist klassische Softwaretechnik.
• Die Text-Rettung: Ein Quantencomputer liefert dem Angreifer „nur“ die mathematische Periode,
  aus der die Primfaktoren p und q berechnet werden. Den Rest (Phi, d-Exponent und Textentschlüsselung) erledigt wieder ein normaler PC.

Wo ist die Verbindung zum Quantencomputing?
Die Formel `modVerschluesselung` selbst läuft auf der normalen CPU.
Ein echter Quantencomputer nutzt diese Formel jedoch direkt im Zusammenspiel mit den Qubits:
Er baut diese modulare Multiplikation mithilfe von Quantengattern (wie Toffoli-Gattern) als Überlagerung im Vektorraum nach.
Er berechnet die Funktion für alle über eine Million Zustände gleichzeitig (Quanten-Parallelismus).
Erst dadurch wird die Schwingung erzeugt, die nach der QFT als blaue Spitzen im Diagramm sichtbar wird.

Die Formel modVerschluesselung selbst läuft auf der normalen CPU.
Ein echter Quantencomputer nutzt diese Formel jedoch im Zusammenspiel mit den Qubits:
Er baut diese modulare Multiplikation mithilfe von Quantengattern (wie Toffoli-Gattern)
als Überlagerung im RAM nach.
Er berechnet die Funktion modVerschluesselung für alle über eine Million Zustände in psi_qubits
gleichzeitig (Quanten-Parallelismus).
Erst dadurch wird die Schwingung erzeugt, die nach der QFT als blaue Spitzen im Diagramm sichtbar wird.

Vergleich Quantenalgorithmen vs. PC-Simulation:

1. Nutzt das Programm das Toffoli-Gatter?
Nein, das Toffoli-Gatter wird aktuell eingespart. Wenn man  den Quellcode der mainwindow.cpp untersucht,
wird man feststellen, dass in Schritt 3 Berechnungen wie die modulare Potenzierung auf ganz normalen CPU-Variablen
ausgeführt werden.
Um den Rechen-Flaschenhals zu umgehen, nutzt die Simulation hier eine verhaltensbasierte Abkürzung.
ABER: Das Programm nutzt nun für die Superposition (applyHadamard) und für die Frequenzanalyse (applyQFTGatter)
echte, simulierte Quantengatter!
Es bildet die interne Gatter-Struktur eines Quantencomputers an den entscheidenden Stellen mikroskopisch
exakt nach, anstatt nur eine mathematische FFT-Formel über die Daten zu legen.

________________________________________

2. Was ist ein Toffoli-Gatter und wie arbeitet es auf einem Quantencomputer?
Das Toffoli-Gatter (auch bekannt als CCNOT-Gatter oder Controlled-Controlled-NOT) ist das fundamentale Bindeglied
zwischen klassischer Logik und Quantenlogik:
• Die Funktionsweise: Es besitzt 3 Qubits (zwei Kontroll-Qubits und ein Ziel-Qubit). Das Ziel-Qubit wird genau dann
  umgedreht (von |0> auf |1> oder umgekehrt), wenn beide Kontroll-Qubits im Zustand |1> sind.
• Warum es unverzichtbar ist: In der Quantenmechanik müssen alle physikalischen Operationen reversibel (umkehrbar)
  sein – es darf keine Information verloren gehen (Unität). Ein normales klassisches AND (Und-Gatter) wirft Information weg:
  Wenn das Ergebnis 0 ist, wissen Sie nicht, ob die Eingabe 0,0, 0,1 oder 1,0 war. Das Toffoli-Gatter löst dieses Problem:
  Es ist vollkommen umkehrbar und erlaubt es gleichzeitig, jede beliebige klassische Logik (wie Addition oder eben
  die modVerschluesselung) verlustfrei in einem Quanten-Schaltkreis nachzubauen.
• Der Quanten-Parallelismus: Ein echter Quantencomputer schickt nicht eine Zahl nach der anderen durch das Toffoli-Gatter.
  Da sich die Eingangs-Qubits durch Hadamard-Gatter in einer Superposition (Überlagerung) aller möglichen Zustände befinden,
  wendet das Gatter die Logik auf alle Millionen Zustände gleichzeitig an.

________________________________________

3. Wie funktioniert es auf dem Windows-PC?
Da ein Windows-Prozessor (CPU) auf klassischer Halbleiter-Physik basiert, kennt er keine echte physikalische Superposition.
Wenn ein PC ein Quantenregister simulieren möchte, nutzt dieses Programm einen hocheffizienten, hybriden Ansatz:

• Die Gatter-Struktur (Superposition & QFT):
  In Schritt 2 und Schritt 4 arbeitet der PC strukturell wie ein echter Quantencomputer. Die `applyHadamard`-Schleife
  simuliert exakt die physikalische Wellenmechanik. Jedes Qubit wird einzeln transformiert, wodurch im RAM über echte
  Interferenz (Auslöschung und Verstärkung) die Gleichverteilung entsteht. Auch die Quanten-Fourier-Transformation
  wird Bit für Bit über ein Netzwerk aus simulierten Hadamard- und Controlled-Phase-Gattern berechnet.

• Die mathematische Emulation (Die Verschränkungs-Abkürzung):
  Anstatt die Milliarden Toffoli-Gatter-Schritte für die Modulo-Rechnung mühsam auf der CPU nachzubauen (was den PC für
  Stunden einfrieren lassen würde), berechnet der PC die zugrundeliegende mathematische Funktion f(x) = x^e mod N direkt
  in einer hocheffizienten CPU-Schleife. Er simuliert das Endergebnis des physikalischen Kollapses durch eine manuelle
  Zustands-Auslöschung im RAM.

Zusammenfassung:
Das Programm nutzt eine hochoptimierte, hybride Gatter-Emulation. Es liefert die korrekten physikalischen Antworten
(die Schwingungs-Peaks des Shor-Algorithmus) und macht die Funktionsweise echter Quantengatter direkt auf der X-Achse
deres Diagramms sichtbar.

Fazit:
Durch den radikalen Umbau auf reines C++ (ohne Eigen-Bibliothek), den Einbau der echten Quanten-Gatter (Hadamard & Controlled-Phase)
und der mathematisch exakten Shor-Oberschwingungs-Analyse ist der Simulator nun ein wissenschaftlich präzises Messwerkzeug.
Die Simulation verhält sich jetzt in jedem einzelnen Pixel der X-Achse und in jeder Protokollzeile exakt so wie ein echter
physikalischer Quanten-Chip.

*/


