/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *decypherButton;
    QPushButton *quitButton;
    QTextEdit *cryptoInput;
    QTextEdit *decypherMsgOutput;
    QTextEdit *messageInput;
    QLabel *label;
    QLineEdit *keyEdit;
    QLabel *label_2;
    QLabel *label_3;
    QPushButton *encodeButton;
    QTextEdit *cryptoOutput;
    QCustomPlot *plot;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1032, 528);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        decypherButton = new QPushButton(centralwidget);
        decypherButton->setObjectName("decypherButton");
        decypherButton->setGeometry(QRect(130, 250, 121, 51));
        quitButton = new QPushButton(centralwidget);
        quitButton->setObjectName("quitButton");
        quitButton->setGeometry(QRect(470, 450, 111, 51));
        quitButton->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 170, 0);"));
        cryptoInput = new QTextEdit(centralwidget);
        cryptoInput->setObjectName("cryptoInput");
        cryptoInput->setGeometry(QRect(50, 70, 301, 171));
        decypherMsgOutput = new QTextEdit(centralwidget);
        decypherMsgOutput->setObjectName("decypherMsgOutput");
        decypherMsgOutput->setGeometry(QRect(50, 330, 301, 171));
        messageInput = new QTextEdit(centralwidget);
        messageInput->setObjectName("messageInput");
        messageInput->setGeometry(QRect(700, 70, 301, 171));
        label = new QLabel(centralwidget);
        label->setObjectName("label");
        label->setGeometry(QRect(700, 250, 161, 31));
        keyEdit = new QLineEdit(centralwidget);
        keyEdit->setObjectName("keyEdit");
        keyEdit->setGeometry(QRect(700, 290, 151, 31));
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(700, 10, 301, 31));
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(50, 10, 301, 31));
        encodeButton = new QPushButton(centralwidget);
        encodeButton->setObjectName("encodeButton");
        encodeButton->setGeometry(QRect(880, 270, 121, 51));
        cryptoOutput = new QTextEdit(centralwidget);
        cryptoOutput->setObjectName("cryptoOutput");
        cryptoOutput->setGeometry(QRect(700, 330, 301, 171));
        plot = new QCustomPlot(centralwidget);
        plot->setObjectName("plot");
        plot->setGeometry(QRect(380, 160, 291, 241));
        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Krypto-Sender (Verschluesselung)", nullptr));
        decypherButton->setText(QCoreApplication::translate("MainWindow", "Decypher", nullptr));
        quitButton->setText(QCoreApplication::translate("MainWindow", "Quit", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Chiffre (min.2 Zeichen) :", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Test-Feld f\303\274r zu codierende Texte:", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Simulation: Qubit - Krypto-Angriff", nullptr));
        encodeButton->setText(QCoreApplication::translate("MainWindow", "Encode !", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
