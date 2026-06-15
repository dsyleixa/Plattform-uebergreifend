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
        MainWindow->resize(800, 654);
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
        vidScrshot = new QPushButton(centralwidget);
        vidScrshot->setObjectName("vidScrshot");
        vidScrshot->setGeometry(QRect(160, 280, 91, 51));
        vidRun = new QPushButton(centralwidget);
        vidRun->setObjectName("vidRun");
        vidRun->setGeometry(QRect(270, 280, 91, 51));
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
        quitButton->setGeometry(QRect(50, 570, 111, 51));
        btnSaveKIpng = new QPushButton(centralwidget);
        btnSaveKIpng->setObjectName("btnSaveKIpng");
        btnSaveKIpng->setGeometry(QRect(560, 550, 81, 51));
        btnLoadDataset = new QPushButton(centralwidget);
        btnLoadDataset->setObjectName("btnLoadDataset");
        btnLoadDataset->setGeometry(QRect(50, 360, 91, 51));
        progressBar = new QProgressBar(centralwidget);
        progressBar->setObjectName("progressBar");
        progressBar->setGeometry(QRect(160, 420, 91, 21));
        progressBar->setStyleSheet(QString::fromUtf8("border: 1px solid #828282;"));
        progressBar->setValue(24);
        btnExportCSV = new QPushButton(centralwidget);
        btnExportCSV->setObjectName("btnExportCSV");
        btnExportCSV->setGeometry(QRect(160, 360, 91, 51));
        btnKiRun = new QPushButton(centralwidget);
        btnKiRun->setObjectName("btnKiRun");
        btnKiRun->setGeometry(QRect(50, 470, 91, 51));
        btnKiStop = new QPushButton(centralwidget);
        btnKiStop->setObjectName("btnKiStop");
        btnKiStop->setGeometry(QRect(160, 470, 91, 51));
        btnKiTrain = new QPushButton(centralwidget);
        btnKiTrain->setObjectName("btnKiTrain");
        btnKiTrain->setGeometry(QRect(270, 470, 91, 51));
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
        vidScrshot->setText(QCoreApplication::translate("MainWindow", "ScrShot", nullptr));
        vidRun->setText(QCoreApplication::translate("MainWindow", "Cam Run", nullptr));
        labelCameraLive->setText(QCoreApplication::translate("MainWindow", "Cam", nullptr));
        labelKiInput->setText(QCoreApplication::translate("MainWindow", "s/w 224x224", nullptr));
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
