#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVector>
#include <QString>
#include <QCloseEvent>
#include <complex>
#include <vector>
#include <cstdint>

// Das globale Bibliotheks-Include dank des INCLUDEPATHs in der .pro-Datei
#include <qcustomplot.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_encodeButton_clicked();
    void on_decypherButton_clicked();
    void on_quitButton_released();

private:
    Ui::MainWindow *ui;

    // MATHE-UPGRADE: Der 20-Qubit-Rechenkern als lesbarer C++ Standard-Vektor (1.048.576 komplexe Amplituden)
    std::vector<std::complex<double>> psi_qubits;

    // Mathematische Hilfs- und Kernfunktionen mit strikter int32_t / int64_t Typisierung
    int64_t modVerschluesselung(int64_t basis, int64_t exponent, int64_t modulus);
    void applyQFT(std::vector<std::complex<double>>& state, int32_t num_qubits);
    int64_t gcd(int64_t a, int64_t b);
    int32_t findPeriodKlassisch(int32_t base, int32_t modulus);
    std::vector<int32_t> test_extrahiereAlleFaktoren(int32_t n);
    std::vector<int32_t> extrahiereShorFaktoren(int32_t n, int32_t bin, int32_t states, int32_t basis);
};

#endif // MAINWINDOW_H
