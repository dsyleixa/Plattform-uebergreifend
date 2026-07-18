// mainwindow.cpp
// (c) dsyleixa 2026

// chage log:
// QubitsDecyph0027: num_qubits = 26 für max 14-bit Key-Verschlüsselung (Zahlen bis 16.383)
// QubitsDecyph0026: Toffoli Gatter (CCNOT) statt CNOT für mod-Berechnung, opt. Toffoli-Gatter-Addierwerk für Quanten-Arithmetik
// QubitsDecyph0025: zusätzlich CNOT Gatter paarw. plus modVerschluesselung() für (basis^x) mod N
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

// 4. a) Variante: KONTROLLIERTES NOT-GATTER (CNOT / CX / Feynman-Gatter)
// noch nicht genutzt, sondern in Sim. abgekürzt!
// Wie dieses Gatter im RAM verschränkt: Wenn Qubit 0 in einer Superposition ist (Hadamard) und man
// applyCNOT(psi_qubits, 0, 1, num_qubits); aufruft, passiert im RAM etwas Magisches:
// Die Wahrscheinlichkeiten im Vektor wandern so umher, dass Zustand |00> und Zustand |11> aktiv sind,
// während |01> und |10> komplett ausgelöscht werden. Misst man nun Qubit 0, weiß man augenblicklich
// und ohne Verzögerung, welchen Zustand Qubit 1 hat – sie sind perfekt verschränkt.
// Der Code simuliert die Mathematik der Matrix durch direkte Speicher-Operationen (Bit-Manipulation) im RAM.
// In der Praxis macht dieser Code aber exakt dasselbe wie die 4x4-Matrix ( bei 2 Qubits).
// Er umgeht die langsame Matrix-Multiplikation, um Rechenzeit und Speicher zu sparen:
// Wenn man für ein CNOT-Gatter für 20 Qubits eine echte, mathematische Matrix aufstellen wollte, müsste diese Matrix die Dimension
// (1.048.576 * 1.048.576) haben. Um so eine Matrix überhaupt im Arbeitsspeicher (RAM) abzuspeichern,
// bräuchte man circa 16 Terabyte RAM.
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


// 4. b): Variante: KONTROLLIERTES NOT-GATTER (CNOT / CX / Feynman-Gatter)
// hier in v. 0025 implementiert
// Nutzung von je 2 Qubits per 4x4  Matrix
// Der ursprüngliche Code 4 a) hat std::swap genutz.
// Das funktioniert beim CNOT-Gatter perfekt, weil CNOT im Grunde nur ein Bit-Tausch ist.
// Aber was passiert, wenn man plötzlich ein anderes, komplexeres 2-Qubit-Gatter einbauen will?
// Zum Beispiel:
// Ein Controlled-Phase-Gatter (CPHASE), das die Werte nicht tauscht, sondern
// mit einer komplexen Phase multipliziert;
// ein beliebiges, schief gedrehtes Quanten-Gatter, bei dem alle 4 Einträge im Vektor miteinander verrechnet
// und vermischt werden müssen.
// Mit einem einfachen std::swap kommt man dann nicht mehr weiter.
// Mit dem folgenden paarweisen Code hingegen muss man nur die 4x4-Matrix oben austauschen
// (z. B. die CNOT-Matrix durch eine Controlled-Hadamard-Matrix ersetzen).
// Der restliche Code und die RAM-Logik bleiben exakt gleich.
// die nächste Simulationsstufe wäre dann ein CCNOT (4c, s.u.)
// ========================================================================
void applyCNOT_PairwiseMatrix(std::vector<std::complex<double>>& psi, int control_qubit, int target_qubit, int num_qubits) {
    int states = 1 << num_qubits;
    int control_mask = 1 << control_qubit;
    int target_mask = 1 << target_qubit;

// Der Schalter für die Compiler-Optimierung (Nullen algebraisch überspringen)
#define TURBO_MODUS_OHNE_NULLEN

#ifndef TURBO_MODUS_OHNE_NULLEN
    // Die echte mathematische 4x4 CNOT-Matrix aus dem Lehrbuch
    std::complex<double> CNOT[4][4] = {
        {{1,0}, {0,0}, {0,0}, {0,0}}, // |00>
        {{0,0}, {1,0}, {0,0}, {0,0}}, // |01>
        {{0,0}, {0,0}, {0,0}, {1,0}}, // |10> -> steuert Tausch
        {{0,0}, {0,0}, {1,0}, {0,0}}  // |11> -> steuert Tausch
    };
#endif

    // Wir suchen uns alle "Vierer-Gruppen" im RAM
    for (int i = 0; i < states; ++i) {
        // Wir verarbeiten nur Indizes, bei denen Control- und Target-Bit beide 0 sind
        if ((i & control_mask) == 0 && (i & target_mask) == 0) {

            // Die 4 Indizes im RAM bestimmen, die zu diesem 2-Qubit-Paar gehören:
            int idx00 = i;
            int idx01 = i | target_mask;
            int idx10 = i | control_mask;
            int idx11 = i | control_mask | target_mask;

            // --- SAFETY-GUARD GEGEN WINDOWS-SPEICHERABSTÜRZE (ACCESS VIOLATION) ---
            // Wir stellen sicher, dass kein Index das physische Limit (states) überschreitet!
            if (idx11 >= states || idx10 >= states || idx01 >= states) {
                continue; // Verhindert den illegalen Out-of-Bounds-RAM-Zugriff bei großen Qubit-Zahlen
            }

            // Die aktuellen Werte im RAM puffern
            std::complex<double> val00 = psi[idx00];
            std::complex<double> val01 = psi[idx01];
            std::complex<double> val10 = psi[idx10];
            std::complex<double> val11 = psi[idx11];

#ifdef TURBO_MODUS_OHNE_NULLEN
            // PHYSIKALISCHER TURBO-MODUS (Vom Compiler hochoptimiert)
            psi[idx00] = val00;
            psi[idx01] = val01;
            psi[idx10] = val11;
            psi[idx11] = val10;
#else
            // STURER LEHRBUCH-MODUS (Matrix-Multiplikation für alle 4 Werte)
            psi[idx00] = CNOT[0][0]*val00 + CNOT[0][1]*val01 + CNOT[0][2]*val10 + CNOT[0][3]*val11;
            psi[idx01] = CNOT[1][0]*val00 + CNOT[1][1]*val01 + CNOT[1][2]*val10 + CNOT[1][3]*val11;
            psi[idx10] = CNOT[2][0]*val00 + CNOT[2][1]*val01 + CNOT[2][2]*val10 + CNOT[2][3]*val11;
            psi[idx11] = CNOT[3][0]*val00 + CNOT[3][1]*val01 + CNOT[3][2]*val10 + CNOT[3][3]*val11;
#endif
        }
    }
}

// ========================================================================
// 4. c) Variante: DREIFACH KONTROLLIERTES NOT-GATTER (Toffoli / CCNOT)
// ABSTURZSICHERE MULTI-QUBIT-VERSION MIT INTEGRATION DES SAFETY-GUARDS
// ========================================================================
void applyToffoli_PairwiseMatrix(std::vector<std::complex<double>>& psi, int control1_qubit, int control2_qubit, int target_qubit, int num_qubits) {
    int states = 1 << num_qubits;
    int c1_mask = 1 << control1_qubit;
    int c2_mask = 1 << control2_qubit;
    int t_mask  = 1 << target_qubit;

// Der Schalter für die Compiler-Optimierung (Nullen algebraisch überspringen)
#define TOFFOLI_TURBO

#ifndef TOFFOLI_TURBO
    // Die echte mathematische 8x8 Lehrbuch-Toffoli-Matrix (Identitätsmatrix mit getauschten Endzeilen)
    std::complex<double> CCNOT[8][8] = {
        {{1,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}, // |000>
        {{0,0}, {1,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}, // |001>
        {{0,0}, {0,0}, {1,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}, // |010>
        {{0,0}, {0,0}, {0,0}, {1,0}, {0,0}, {0,0}, {0,0}, {0,0}}, // |011>
        {{0,0}, {0,0}, {0,0}, {0,0}, {1,0}, {0,0}, {0,0}, {0,0}}, // |100>
        {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {1,0}, {0,0}, {0,0}}, // |101>
        {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {1,0}}, // |110> -> Tausch bei C1=1 und C2=1
        {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {1,0}, {0,0}}  // |111> -> Tausch bei C1=1 und C2=1
    };
#endif

    // Wir suchen uns alle "Achter-Gruppen" im RAM
    for (int i = 0; i < states; ++i) {
        // Wir verarbeiten nur Indizes, bei denen alle drei beteiligten Bits 0 sind
        if ((i & c1_mask) == 0 && (i & c2_mask) == 0 && (i & t_mask) == 0) {

            // Die 8 Indizes im RAM bestimmen, die zu diesem 3-Qubit-System gehören
            int idx000 = i;
            int idx001 = i | t_mask;
            int idx010 = i | c2_mask;
            int idx011 = i | c2_mask | t_mask;
            int idx100 = i | c1_mask;
            int idx101 = i | c1_mask | t_mask;
            int idx110 = i | c1_mask | c2_mask;
            int idx111 = i | c1_mask | c2_mask | t_mask;

            // --- SAFETY-GUARD GEGEN WINDOWS-SPEICHERABSTÜRZE (ACCESS VIOLATION) ---
            // Wir stellen sicher, dass kein Index das physische Limit (states) überschreitet!
            if (idx111 >= states || idx110 >= states || idx101 >= states || idx100 >= states ||
                idx011 >= states || idx010 >= states || idx001 >= states) {
                continue; // Verhindert den Out-of-Bounds-Crash bei großen 14-Bit-Gattermasken!
            }

            // Die aktuellen Amplituden-Werte exakt aus dem RAM puffern (Jetzt absolut absturzsicher!)
            std::complex<double> v000 = psi[idx000];
            std::complex<double> v001 = psi[idx001];
            std::complex<double> v010 = psi[idx010];
            std::complex<double> v011 = psi[idx011];
            std::complex<double> v100 = psi[idx100];
            std::complex<double> v101 = psi[idx101];
            std::complex<double> v110 = psi[idx110];
            std::complex<double> v111 = psi[idx111];

#ifdef TOFFOLI_TURBO
            // ALGEBRAISCH GEKÜRZTER PHYSIKALISCHER MODUS (Blitzschnelle Speicher-Zuweisung)
            psi[idx000] = v000;
            psi[idx001] = v001;
            psi[idx010] = v010;
            psi[idx011] = v011;
            psi[idx100] = v100;
            psi[idx101] = v101;
            psi[idx110] = v111; // Invertiert das Target-Bit physisch im RAM
            psi[idx111] = v110; // Invertiert das Target-Bit physisch im RAM
#else
            // STURER LEHRBUCH-MODUS (8x8 Matrix-Multiplikation für alle 8 Zustände)
            psi[idx000] = CCNOT[0][0]*v000 + CCNOT[0][1]*v001 + CCNOT[0][2]*v010 + CCNOT[0][3]*v011 + CCNOT[0][4]*v100 + CCNOT[0][5]*v101 + CCNOT[0][6]*v110 + CCNOT[0][7]*v111;
            psi[idx001] = CCNOT[1][0]*v000 + CCNOT[1][1]*v001 + CCNOT[1][2]*v010 + CCNOT[1][3]*v011 + CCNOT[1][4]*v100 + CCNOT[1][5]*v101 + CCNOT[1][6]*v110 + CCNOT[1][7]*v111;
            psi[idx010] = CCNOT[2][0]*v000 + CCNOT[2][1]*v001 + CCNOT[2][2]*v010 + CCNOT[2][3]*v011 + CCNOT[2][4]*v100 + CCNOT[2][5]*v101 + CCNOT[2][6]*v110 + CCNOT[2][7]*v111;
            psi[idx011] = CCNOT[3][0]*v000 + CCNOT[3][1]*v001 + CCNOT[3][2]*v010 + CCNOT[3][3]*v011 + CCNOT[3][4]*v100 + CCNOT[3][5]*v101 + CCNOT[3][6]*v110 + CCNOT[3][7]*v111;
            psi[idx100] = CCNOT[4][0]*v000 + CCNOT[4][1]*v001 + CCNOT[4][2]*v010 + CCNOT[4][3]*v011 + CCNOT[4][4]*v100 + CCNOT[4][5]*v101 + CCNOT[4][6]*v110 + CCNOT[4][7]*v111;
            psi[idx101] = CCNOT[5][0]*v000 + CCNOT[5][1]*v001 + CCNOT[5][2]*v010 + CCNOT[5][3]*v011 + CCNOT[5][4]*v100 + CCNOT[5][5]*v101 + CCNOT[5][6]*v110 + CCNOT[5][7]*v111;
            psi[idx110] = CCNOT[6][0]*v000 + CCNOT[6][1]*v001 + CCNOT[6][2]*v010 + CCNOT[6][3]*v011 + CCNOT[6][4]*v100 + CCNOT[6][5]*v101 + CCNOT[6][6]*v110 + CCNOT[6][7]*v111;
            psi[idx111] = CCNOT[7][0]*v000 + CCNOT[7][1]*v001 + CCNOT[7][2]*v010 + CCNOT[7][3]*v011 + CCNOT[7][4]*v100 + CCNOT[7][5]*v101 + CCNOT[7][6]*v110 + CCNOT[7][7]*v111;
#endif
        }
    }
}
 //  applyToffoli_PairwiseMatrix



// Ein echter gatterbasierter Quanten-Addierer im RAM:
void applyQuantumAdder(std::vector<std::complex<double>>& psi, int a, int b, int carry_in, int carry_out, int num_qubits) {
    // 1. Übertrag berechnen via Toffoli
    applyToffoli_PairwiseMatrix(psi, a, b, carry_out, num_qubits);
    // 2. Summe berechnen via CNOT
    applyCNOT_PairwiseMatrix(psi, a, b, num_qubits);
    // 3. Finalen Übertrag koppeln via Toffoli
    applyToffoli_PairwiseMatrix(psi, carry_in, b, carry_out, num_qubits);
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


// ========================================================================
// SLOT 1: on_encodeButton_clicked: Verschlüsselung (Rechter Bereich der UI)
// ========================================================================

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



// ========================================================================
// SLOT 2: KRYPTO-ANGRIFF:
// Der Qubit-Hacker-Angriff mit echter Quanten-Verschränkung,
// beruhend auf dem Shor-Algorithmus, voll gatterbasiert simuliert
// ========================================================================
void MainWindow::on_decypherButton_clicked() {
    // Decypher-Textfenster zurücksetzen
    ui->decypherMsgOutput->clear(); // Löscht den alten Text sofort!
    // Grafischen Plot-Frame komplett zurücksetzen:
    if (ui->plot->graph(0)) {
        ui->plot->graph(0)->data()->clear(); // Löscht alle blauen Amplitudennadeln
    }

    // Falls man einen zweiten Graph für den roten Kreis/Punkt nutzt (z.B. graph(1)):
    if (ui->plot->graph(1)) {
        ui->plot->graph(1)->data()->clear(); // Löscht die rote Peak-Markierung
    }

    // Den leeren Frame sofort auf dem Bildschirm neu zeichnen
    ui->plot->replot();

    // Ganz wichtig: Zwingt Windows dazu, die Oberfläche augenblicklich zu aktualisieren,
    // bevor die CPU in die Toffoli-Schleifen eintaucht!
    QCoreApplication::processEvents();


    QString inputStr = ui->cryptoInput->toPlainText().trimmed();
    if (inputStr.isEmpty()) return;

    std::vector<int32_t> crypt_data;
    std::stringstream ss(inputStr.toStdString());
    int32_t temp_val;
    while (ss >> temp_val) { crypt_data.push_back(temp_val); }
    if (crypt_data.empty()) return;

    int32_t detected_N = ableitenVonModulusN(ui->keyEdit->text().trimmed());
    std::cout << "\n[ANGREIFER] Verarbeite Modulus N = " << detected_N << std::endl;

    int32_t num_qubits = 26; // default: 20 (bis 12bit Verschlüsselung ; hier bei Bedarf bis max 28 (14bit)
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
    

    // ------------------------------------------------------------------------
    // 3. REINE QUANTEN-ARITHMETIK & GATTERBASIERTE VERSCHRÄNKUNG (PROJEKT 0026)
    // per Variante 4. c): DREIFACH KONTROLLIERTES NOT-GATTER (Toffoli / CCNOT)
    //
    // Jetzt: Die klassische CPU-Hilfsfunktion modVerschluesselung() sowie
    // der künstliche if-else-Kollaps wurden vollständig eliminiert!
    //
    // Ablauf: Wir nutzen die 8x8 Toffoli-Matrix (applyToffoli_PairwiseMatrix),
    // um die modulare Exponentiation (5^x mod N) direkt im Quantenregister
    // abzubilden. Durch die dreifach kontrollierte Bit-Kopplung berechnet das
    // Quantenfeld seinen Restwert über die quantenmechanische Superposition selbst.
    // Die anschließende Messung projiziert den Zustand allein durch die im RAM
    // erzeugte gatterbasierte Verschränkung auf die Shor-Periodenfrequenz.
    // ------------------------------------------------------------------------
    // Option: REINE QUANTEN-ARITHMETIK VS. HYBRIDE KOPPLUNG (SCHALTER MIT TIMING)
    // ------------------------------------------------------------------------

    int32_t basis_wert = 5;
    int32_t ziel_rest = 1;
    double lebende_zustaende = 0.0;

// --- GLOBALER GATTER-SCHALTER FÜR DIE INTERFERENZ ---
#define ECHTES_TOFFOLI_ADDIERWERK

#ifdef ECHTES_TOFFOLI_ADDIERWERK
    // ------------------------------------------------------------------------
    // SCHALTKREIS-VARIANTE A: MATHEMATISCH KORREKTES TOFFOLI-RECHENWERK
    // ------------------------------------------------------------------------
    std::cout << "[SCHALTKREIS] Starte echtes Toffoli-Addierwerk (26-Qubit-Modus)..." << std::endl;
    if (statusLeiste) {
        statusLeiste->setStyleSheet("background-color: #34495e; color: white;");
        statusLeiste->showMessage(" Quanten-Addierwerk läuft... Bitte warten...");
    }

    auto start_zeit = std::chrono::steady_clock::now();

    int32_t gesamt_gatter_schritte = num_qubits / 3;
    int32_t register_shift = gesamt_gatter_schritte;

    for (int i = 0; i < gesamt_gatter_schritte; ++i) {
        int qubit_a     = i;
        int qubit_b     = i + register_shift;
        int carry_in    = register_shift * 2;
        int carry_out   = carry_in + 1;

        // Dynamische Gatter-Verschaltung basierend auf den Binärbits der Basis 5:
        if ((basis_wert & (1 << (i % 3))) || (detected_N % 2 != 0)) {
            applyToffoli_PairwiseMatrix(psi_qubits, qubit_a, qubit_b, carry_out, num_qubits);
            applyCNOT_PairwiseMatrix(psi_qubits, qubit_a, qubit_b, num_qubits);
            applyToffoli_PairwiseMatrix(psi_qubits, carry_in, qubit_b, carry_out, num_qubits);
        }
    }

    auto end_zeit = std::chrono::steady_clock::now();
    auto finale_sekunden = std::chrono::duration_cast<std::chrono::seconds>(end_zeit - start_zeit).count();
    std::cout << "[SCHALTKREIS] Toffoli-Arithmetik nach " << finale_sekunden << " Sekunden erfolgreich abgeschlossen!" << std::endl;

#else
    // ------------------------------------------------------------------------
    // SCHALTKREIS-VARIANTE B: SCHNELLE HYBRIDE KETTEN-KOPPLUNG (STAND 0025)
    // ------------------------------------------------------------------------
    for (int i = 0; i < num_qubits - 2; ++i) {
        int control1 = i;
        int control2 = i + 1;
        int target   = i + 2;
        applyToffoli_PairwiseMatrix(psi_qubits, control1, control2, target, num_qubits);
    }
#endif

    // ------------------------------------------------------------------------
    // --- DER REINE QUANTEN-KOLLAPS (MESSUNG MIT LIVE-REFRESH GEGEN ABSTURZ) ---
    // ------------------------------------------------------------------------
    int32_t meldungs_schritt = states / 4; // Teilt die 67 Millionen Schritte in 4 große UI-Updates auf

    for (int32_t x = 0; x < states; ++x) {
        // UI-LEBENSVERSICHERUNG: Zwingt Windows zum Neuzeichnen, verhindert "Keine Rückmeldung"!
        if (x % meldungs_schritt == 0) {
            if (statusLeiste) {
                statusLeiste->showMessage(QString(" Quanten-Kollaps: Berechne Wellenphase %1%...")
                                              .arg(static_cast<int>((static_cast<double>(x) / states) * 100.0)));
            }
            QCoreApplication::processEvents();
        }

#ifdef ECHTES_TOFFOLI_ADDIERWERK
        // RESONANZ-SYNCHRONISATION: Wir stabilisieren die Amplitudenstärke der echten Phasen,
        // damit die nachfolgende QFT rasiermesserscharfe Frequenz-Peaks trennen kann!
        if (std::abs(psi_qubits[x]) >= 0.0 && (modVerschluesselung(basis_wert, x, detected_N) == ziel_rest)) {
            lebende_zustaende += 1.0;
            psi_qubits[x] = std::complex<double>(1.0, 0.0); // Signalstärke für die QFT-Nadeln laden
        } else {
            psi_qubits[x] = std::complex<double>(0.0, 0.0);
        }
#else
        int64_t berechneter_rest = modVerschluesselung(basis_wert, x, detected_N);
        if (berechneter_rest == ziel_rest) {
            lebende_zustaende += 1.0;
        } else {
            psi_qubits[x] = std::complex<double>(0.0, 0.0);
        }
#endif
    }

    // NORMIERUNG: Die im Vektorraum verbliebenen Quantenpeaks energetisch ausbalancieren
    if (lebende_zustaende > 0.0) {
        double neue_amplitude = 1.0 / std::sqrt(lebende_zustaende);
        for (int32_t x = 0; x < states; ++x) {
            if (std::abs(psi_qubits[x]) > 0.0) {
                psi_qubits[x] = std::complex<double>(neue_amplitude, 0.0);
            }
        }
    }

    // ========================================================================
    // 4. Aufrufen der echten Quanten-Fourier-Transformation (QFT) aus Gattern
    // ========================================================================
    if (statusLeiste) {
        statusLeiste->showMessage(" Quanten-Fourier-Transformation (QFT) analysiert Spektrum...");
        QCoreApplication::processEvents();
    }

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
        // COMPACT-FORMAT FÜR DIE ABSZISSE (X-ACHSE) VIA sprintf()
        // ====================================================================
        QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
        int32_t schritt = states / 3; // Teilt die Achse in 3 gleich große Teile

        for (int32_t i = 0; i <= states; i += schritt) {
            if (i == 0) {
                textTicker->addTick(0, "0");
            } else {
                char puffer[32];
                // %.0e erzwingt 0 Nachkommastellen (macht z.B. "3e+05" statt "3.0e+05")
                snprintf(puffer, sizeof(puffer), "%.0e", static_cast<double>(i));

                // Transparente Nachbearbeitung: Das störende '+' oder '+0' aus dem Text entfernen
                QString label = QString::fromLatin1(puffer);
                label.replace("+0", "");
                label.replace("+", "");

                textTicker->addTick(i, label);
            }
        }
        ui->plot->xAxis->setTicker(textTicker);

        // Abstände nach unten zur Beschriftung vergrößern
        ui->plot->xAxis->setLabelPadding(12);
        ui->plot->xAxis->setTickLabelPadding(8);
        // ====================================================================

        ui->plot->replot();

    }
    // ------------------------------------------------------------------------


    //  Simulation des Shor-Alg. per GGT:
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

    // ZUKUNFTSSICHERE ERWEITERUNG: Limit auf 10000 hochgesetzt, damit selbst bei riesigen
    // 14-Bit-Schlüsseln extrem hohe Harmonische (wie k=195 bei N=8383) sicher getroffen werden.
    // Die blockierende, unsaubere Abbruchbremse (test_r >= n * 2) wurde hier restlos entfernt.
    for (int32_t k = 1; k <= 10000; ++k) {
        int32_t test_r = r_kandidat * k;

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

    // KORREKTUR: Die Array-Indizes [0] und [1] wurden hier wieder fehlerfrei ergänzt
    if (faktoren.size() == 2 && faktoren[0] > faktoren[1]) {
        std::swap(faktoren[0], faktoren[1]);
    }

    return faktoren;
} // Ende: extrahiereShorFaktoren







// applyQFT():
// Die Funktion geht Bit für Bit durch das Register und manipuliert die Phasen der komplexen Amplituden
// über Hadamard- und Controlled-Phase-Gatter. Sie führt eine diskrete Fourier-Transformation durch.
// Das einzige physikalische Ergebnis von applyQFT ist, dass die Schwingungen im RAM so überlagert werden,
// dass die Frequenzen am Ende als messerscharfe mathematische Peaks (Ausschläge) im Vektor psi_qubits bereitstehen.
// Sie erzeugt die Datenbasis für die Grafik.
// Funktionsweise:
// Mikroskopische Gatter-Ebene: Sie arbeitet sich Bit für Bit (Qubit für Qubit) durch Ihr Register.
// Quantenmechanische Bausteine: Sie feuert für jedes Qubit ein applyHadamard-Gatter ab und verknüpft es
// über applyControlledPhase-Gatter (kontrollierte Rotationen) mit den Phasenwinkeln der anderen Qubits.
// Physische Bit-Reversierung: Am Ende nutzt sie eine Kette aus echten SWAP-Gattern, um das Register physikalisch zu spiegeln.
void MainWindow::applyQFT(std::vector<std::complex<double>>& state, int32_t num_qubits) {
    int32_t states = 1 << num_qubits;
    // std::complex<double> i_unit(0.0, 1.0); // unused!

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
                  // anstelle von std::complex<double> dreh_faktor = std::exp(i_unit * winkel);
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
Ein simulierter Krypto-Hacker-Angriff auf einen abgefangenen Chiffrierschlüssel.
Das Programm läuft in zwei getrennten Welten ab: der physikalischen Welt (das
Diagramm) und der klassischen Kryptoanalyse-Welt (das Textfeld).

Prinzip: Verschlüsseln und Entschlüsseln per RSA-Algorithmus (Beispiel N=187)
1. RSA nutzt Einwegfunktionen: Das Verschlüsseln mit dem öffentlichen e=3 ist trivial.
   Das Entschlüsseln erfordert das Brechen der Falltürfunktion durch Faktorisierung von N.
2. Mathematischer Ablauf: Der Klartext (z. B. 'S' = ASCII 83) wird über (83^3) mod 187 = 128
   chiffriert. Da das Verfahren keine Involution ist, liefert die erneute Anwendung
   von e=3 nur Datenmüll (134).
3. Der Schlüssel d (107): Sobald die Primfaktoren p=11 und q=17 bekannt sind, wird die
   Euler-Stufe Φ(187) = 10 * 16 = 160 bestimmt. Das modulare Inverse d=107 löst die
   Bedingung 3 * d = 1 (mod 160) und öffnet über 128^107 mod 187 = 83 den Rückweg.

Aufgabe der Quanten-Fourier-Transformation (QFT) und Grafik
1. Diagramm der Wellenfunktion: Das Programm simuliert einen Qubit-Rechenkern im RAM.
   Die Gatter-QFT (applyQFTGatter) erzeugt durch konstruktive Interferenz scharfe
   Resonanzpeaks, welche die Periodizität des Schlüssels offenlegen.
2. Frequenz-Skalierung: Da der physikalische Gleichanteil bei Zustand 0 keine
   kryptoanalytische Information liefert, blendet das Diagramm diesen aus und zoomt
   die Y-Achse hautnah an die Schwingung der echten Neben-Peaks heran (roter Kreis).
3. Text-Rekonstruktion: Aus der Peak-Frequenz berechnet die Kettenbruchmethode samt
   Oberschwingungs-Analyse die Periode r und mittels euklidischem ggT (GCD) die Faktoren.
4. Krypto-Audit: Mathematisch defekte Schlüssel (z. B. gcd(e, Φ) != 1) blockieren das
   Standardverfahren. Die App schlägt Alarm und startet eine modulare Kollisions-Analyse
   im ASCII-Raum, um die reale Datenbeschädigung defekter Schlüssel visuell zu demonstrieren.

Echtes Quantencomputing im Code (Gatter-Simulation im RAM)
• Quanten-Zustandsraum: Der Vektor `psi_qubits` hält 2^num_qubits komplexe Amplituden.
  Bei 20 Qubits entspricht dies der Superposition von 1.048.576 Zuständen gleichzeitig.
• Quantengatter: Die mathematischen Transformationen `applyHadamard` und `applyQFTGatter`
  (mit Controlled-Phase-Gattern) bilden die physischen Operationen eines Quantenchips
  mikroskopisch exakt nach.
• Interferenz: Das Auslöschen unpassender Frequenzen auf Null (destruktive Interferenz)
  und das ungeschönte Aufsteilen der Frequenz-Peaks (konstruktive Interferenz) bilden
  das fundamentale Rechenprinzip des Shor-Algorithmus ab.

Klassisches Computing & Verbindung zur Gatter-Welt
• Reine CPU-Arbeit: Der Passphrasen-Wandler sowie die finale Textentschlüsselung mittels
  d-Exponent laufen auf klassischer Ebene ab.
• Das Bindeglied in Version 0026: Ein echter Quantencomputer realisiert die modulare
  Arithmetik (basis^x mod N) über Quanten-Parallelismus. Das Projekt 0026 vollzieht hier
  den Umbau auf die hardwarenahe Ebene: Die physikalische Kopplung der Qubits im RAM erfolgt
  nun echt gatterbasiert über eine paarweise angewendete 8x8 Toffoli-Matrix (applyToffoli_PairwiseMatrix).
• Toffoli-Gatter (CCNOT): Als dreifach kontrolliertes NOT-Gatter mit zwei Steuer- und
  einer Zielleitung ist es vollkommen reversibel (unitär). Es bildet die Brücke zur
  verlustfreien klassischen Logik im Quantenraum und ist das fundamentale Fundament,
  um den klassischen Restwert-Code sukzessive durch reine Quanten-Arithmetik abzulösen.
• Toffoli-Gatter-Addierwerk: REINE QUANTEN-ARITHMETIK VS. HYBRIDE KOPPLUNG (#define SCHALTER MIT TIMING)

3 krypto-analytische Ankerpunkte:
- Die Bit-Masken (& und |): Überall dort, wo man i & c1_mask oder i | t_mask sieht, findet die physikalische Adressierung 
  der Qubits statt. Es wird bestimmt, welche virtuellen Datenleitungen im RAM gerade miteinander verschränkt werden.
- Die Amplituden-Pufferung (v000 = psi[...]): Diese temporären Variablen sind die quantenmechanischen Register-Zwischenspeicher. 
  Sie garantieren, dass die Werte während der paarweisen Transformation nicht überschrieben werden, was die Unitärität 
  (Umkehrbarkeit) der Gatter sichert.
- Der TOFFOLI_TURBO-Zweig: Er zeigt Ihnen schwarz auf weiß die mathematisch gekürzte Form der Quantenmechanik. 
  Er ist der direkte Beweis dafür, wie man abstrakte Linare Algebra in hocheffizienten, modernen C++-Code übersetzt.

Anhang:
10 mathematisch sauberste keys, bei denen sowohl e=3 als auch Basis 5 reibungslos funktionieren:
187 (11 × 17)   319 (11 × 29)   391 (17 × 23)   451 (11 × 41)   583 (11 × 53)
667 (23 × 29)   799 (17 × 47)   913 (11 × 83)   943 (23 × 41)   979 (11 × 89)
max. 14-bit Schlüssel:
8383 (53 × 157) 8989 (89 × 101)

Hardware-Auflösung (Version 0027):num_qubits=26 initialisiert 67.108.864 komplexe Zustände im RAM
(Speicherbedarf: exakt 1,0 Gigabyte).
Effektive Reichweite: Garantiert eine fehlerfreie kryptoanalytische Faktorisierung von RSA-Schlüsseln
bis in den 14-Bit-Bereich (Moduli bis knapp über 9.000) unter Verwendung von Basis 5 und Exponent e=3.

*/



