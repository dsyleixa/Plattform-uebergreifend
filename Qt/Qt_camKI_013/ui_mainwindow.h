/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

// # (c)  dsyleixa 2026


#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QLabel *labelScreenshot;
    QPushButton *vidPause;
    QPushButton *vidScrshot;
    QPushButton *vidRun;
    QLabel *labelCameraLive;
    QLabel *labelKiInput;
    QPushButton *quitButton;
    QPushButton *btnSaveKIpng;
    QPushButton *btnLoadDataset;
    QProgressBar *progressBar;
    QPushButton *btnExportCSV;
    QPushButton *btnKiRun;
    QPushButton *btnKiStop;
    QPushButton *btnKiTrain;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 642);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        labelScreenshot = new QLabel(centralwidget);
        labelScreenshot->setObjectName("labelScreenshot");
        labelScreenshot->setGeometry(QRect(430, 20, 321, 241));
        labelScreenshot->setStyleSheet(QString::fromUtf8("border: 1px solid #828282;"));
        labelScreenshot->setFrameShape(QFrame::Shape::NoFrame);
        vidPause = new QPushButton(centralwidget);
        vidPause->setObjectName("vidPause");
        vidPause->setGeometry(QRect(50, 280, 91, 51));
        QPalette palette;
        QBrush brush(QColor(239, 249, 254, 255));
        brush.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush);
        QBrush brush1(QColor(222, 220, 220, 255));
        brush1.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush1);
        QBrush brush2(QColor(222, 222, 222, 255));
        brush2.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush2);
        vidPause->setPalette(palette);
        vidScrshot = new QPushButton(centralwidget);
        vidScrshot->setObjectName("vidScrshot");
        vidScrshot->setGeometry(QRect(160, 280, 91, 51));
        QPalette palette1;
        palette1.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush);
        palette1.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush2);
        palette1.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush2);
        vidScrshot->setPalette(palette1);
        vidRun = new QPushButton(centralwidget);
        vidRun->setObjectName("vidRun");
        vidRun->setGeometry(QRect(270, 280, 91, 51));
        QPalette palette2;
        palette2.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush);
        QBrush brush3(QColor(250, 250, 0, 255));
        brush3.setStyle(Qt::BrushStyle::SolidPattern);
        palette2.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush3);
        palette2.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush2);
        vidRun->setPalette(palette2);
        labelCameraLive = new QLabel(centralwidget);
        labelCameraLive->setObjectName("labelCameraLive");
        labelCameraLive->setGeometry(QRect(40, 20, 321, 241));
        labelCameraLive->setStyleSheet(QString::fromUtf8("border: 1px solid #828282;"));
        labelKiInput = new QLabel(centralwidget);
        labelKiInput->setObjectName("labelKiInput");
        labelKiInput->setGeometry(QRect(490, 310, 224, 224));
        labelKiInput->setStyleSheet(QString::fromUtf8("border: 1px solid #828282;"));
        quitButton = new QPushButton(centralwidget);
        quitButton->setObjectName("quitButton");
        quitButton->setGeometry(QRect(150, 550, 111, 51));
        QPalette palette3;
        QBrush brush4(QColor(240, 149, 149, 255));
        brush4.setStyle(Qt::BrushStyle::SolidPattern);
        palette3.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush4);
        palette3.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush4);
        palette3.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush4);
        quitButton->setPalette(palette3);
        btnSaveKIpng = new QPushButton(centralwidget);
        btnSaveKIpng->setObjectName("btnSaveKIpng");
        btnSaveKIpng->setGeometry(QRect(560, 550, 81, 51));
        QPalette palette4;
        palette4.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush);
        palette4.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush1);
        palette4.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush2);
        btnSaveKIpng->setPalette(palette4);
        btnLoadDataset = new QPushButton(centralwidget);
        btnLoadDataset->setObjectName("btnLoadDataset");
        btnLoadDataset->setGeometry(QRect(50, 360, 91, 51));
        QPalette palette5;
        QBrush brush5(QColor(254, 250, 221, 255));
        brush5.setStyle(Qt::BrushStyle::SolidPattern);
        palette5.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush5);
        palette5.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush2);
        palette5.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush2);
        btnLoadDataset->setPalette(palette5);
        progressBar = new QProgressBar(centralwidget);
        progressBar->setObjectName("progressBar");
        progressBar->setGeometry(QRect(160, 420, 91, 21));
        progressBar->setMinimumSize(QSize(0, 20));
        progressBar->setStyleSheet(QString::fromUtf8("QProgressBar {\n"
"    border: 1px solid #828282;\n"
"    border-radius: 3px;\n"
"    text-align: center;\n"
"    background-color: #f0f0f0;\n"
"    color: #000000; /* Erzwingt schwarze Prozent-Schrift */\n"
"}\n"
"\n"
"QProgressBar::chunk {\n"
"    background-color: #FF2E2E; /* Ein kr\303\244ftiges, helles Signalrot */\n"
"    width: 1px; /* Korrigiert den Sprung: F\303\274llt jetzt pixelgenau und fl\303\274ssig */\n"
"}\n"
""));
        progressBar->setValue(24);
        btnExportCSV = new QPushButton(centralwidget);
        btnExportCSV->setObjectName("btnExportCSV");
        btnExportCSV->setGeometry(QRect(160, 360, 91, 51));
        QPalette palette6;
        palette6.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush5);
        palette6.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush2);
        palette6.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush2);
        btnExportCSV->setPalette(palette6);
        btnKiRun = new QPushButton(centralwidget);
        btnKiRun->setObjectName("btnKiRun");
        btnKiRun->setGeometry(QRect(50, 470, 91, 51));
        QPalette palette7;
        QBrush brush6(QColor(226, 254, 223, 255));
        brush6.setStyle(Qt::BrushStyle::SolidPattern);
        palette7.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush6);
        palette7.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush2);
        palette7.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush2);
        btnKiRun->setPalette(palette7);
        btnKiStop = new QPushButton(centralwidget);
        btnKiStop->setObjectName("btnKiStop");
        btnKiStop->setGeometry(QRect(160, 470, 91, 51));
        QPalette palette8;
        palette8.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush6);
        palette8.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush2);
        palette8.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush2);
        btnKiStop->setPalette(palette8);
        btnKiTrain = new QPushButton(centralwidget);
        btnKiTrain->setObjectName("btnKiTrain");
        btnKiTrain->setGeometry(QRect(270, 360, 91, 51));
        QPalette palette9;
        QBrush brush7(QColor(170, 255, 0, 255));
        brush7.setStyle(Qt::BrushStyle::SolidPattern);
        palette9.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush7);
        palette9.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush2);
        QBrush brush8(QColor(255, 170, 0, 255));
        brush8.setStyle(Qt::BrushStyle::SolidPattern);
        palette9.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush8);
        btnKiTrain->setPalette(palette9);
        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        labelScreenshot->setText(QCoreApplication::translate("MainWindow", "ScrShot", nullptr));
        vidPause->setText(QCoreApplication::translate("MainWindow", "Cam Pause", nullptr));
        vidScrshot->setText(QCoreApplication::translate("MainWindow", "Screen Shot", nullptr));
        vidRun->setText(QCoreApplication::translate("MainWindow", "Cam Run", nullptr));
        labelCameraLive->setText(QCoreApplication::translate("MainWindow", "Cam", nullptr));
        labelKiInput->setText(QCoreApplication::translate("MainWindow", "col 224x224", nullptr));
        quitButton->setText(QCoreApplication::translate("MainWindow", "Quit", nullptr));
        btnSaveKIpng->setText(QCoreApplication::translate("MainWindow", "SaveKIpng", nullptr));
        btnLoadDataset->setText(QCoreApplication::translate("MainWindow", "Lade Daten", nullptr));
        btnExportCSV->setText(QCoreApplication::translate("MainWindow", "Export CSV", nullptr));
        btnKiRun->setText(QCoreApplication::translate("MainWindow", "KI Run", nullptr));
        btnKiStop->setText(QCoreApplication::translate("MainWindow", "KI Stop", nullptr));
        btnKiTrain->setText(QCoreApplication::translate("MainWindow", "KI Training", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
