/********************************************************************************
** Form generated from reading UI file 'base_main_window.ui'
**
** Created by: Qt User Interface Compiler version 5.15.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BASE_MAIN_WINDOW_H
#define UI_BASE_MAIN_WINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "g2o_qglviewer.h"

QT_BEGIN_NAMESPACE

class Ui_BaseMainWindow
{
public:
    QAction *actionLoad;
    QAction *actionSave;
    QAction *actionQuit;
    QAction *actionWhite_Background;
    QAction *actionDefault_Background;
    QAction *actionDump_Images;
    QAction *actionProperties;
    QAction *actionSave_Screenshot;
    QAction *actionSave_Viewer_State;
    QAction *actionLoad_Viewer_State;
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout;
    QFrame *frame;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *cbDrawAxis;
    QLabel *label;
    QSpinBox *spIterations;
    QCheckBox *cbRobustKernel;
    QCheckBox *cbOnlyLoop;
    QComboBox *coRobustKernel;
    QLabel *label_4;
    QLineEdit *leKernelWidth;
    QLabel *label_2;
    QComboBox *coOptimizer;
    QPushButton *btnOptimizerParameters;
    QSpacerItem *verticalSpacer_2;
    QComboBox *cbxIniitialGuessMethod;
    QPushButton *btnInitialGuess;
    QPushButton *btnReload;
    QPushButton *btnSetZero;
    QPushButton *btnOptimize;
    QPushButton *btnForceStop;
    QPushButton *btnQuit;
    QSpacerItem *verticalSpacer;
    QSplitter *splitter;
    g2o::G2oQGLViewer *viewer;
    QPlainTextEdit *plainTextEdit;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuView;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *BaseMainWindow)
    {
        if (BaseMainWindow->objectName().isEmpty())
            BaseMainWindow->setObjectName(QString::fromUtf8("BaseMainWindow"));
        BaseMainWindow->resize(800, 600);
        actionLoad = new QAction(BaseMainWindow);
        actionLoad->setObjectName(QString::fromUtf8("actionLoad"));
        actionSave = new QAction(BaseMainWindow);
        actionSave->setObjectName(QString::fromUtf8("actionSave"));
        actionQuit = new QAction(BaseMainWindow);
        actionQuit->setObjectName(QString::fromUtf8("actionQuit"));
        actionWhite_Background = new QAction(BaseMainWindow);
        actionWhite_Background->setObjectName(QString::fromUtf8("actionWhite_Background"));
        actionDefault_Background = new QAction(BaseMainWindow);
        actionDefault_Background->setObjectName(QString::fromUtf8("actionDefault_Background"));
        actionDump_Images = new QAction(BaseMainWindow);
        actionDump_Images->setObjectName(QString::fromUtf8("actionDump_Images"));
        actionDump_Images->setCheckable(true);
        actionProperties = new QAction(BaseMainWindow);
        actionProperties->setObjectName(QString::fromUtf8("actionProperties"));
        actionSave_Screenshot = new QAction(BaseMainWindow);
        actionSave_Screenshot->setObjectName(QString::fromUtf8("actionSave_Screenshot"));
        actionSave_Viewer_State = new QAction(BaseMainWindow);
        actionSave_Viewer_State->setObjectName(QString::fromUtf8("actionSave_Viewer_State"));
        actionLoad_Viewer_State = new QAction(BaseMainWindow);
        actionLoad_Viewer_State->setObjectName(QString::fromUtf8("actionLoad_Viewer_State"));
        centralwidget = new QWidget(BaseMainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        horizontalLayout = new QHBoxLayout(centralwidget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        frame = new QFrame(centralwidget);
        frame->setObjectName(QString::fromUtf8("frame"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(frame->sizePolicy().hasHeightForWidth());
        frame->setSizePolicy(sizePolicy);
        frame->setMinimumSize(QSize(150, 0));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        verticalLayout_2 = new QVBoxLayout(frame);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        cbDrawAxis = new QCheckBox(frame);
        cbDrawAxis->setObjectName(QString::fromUtf8("cbDrawAxis"));
        cbDrawAxis->setChecked(true);

        verticalLayout_2->addWidget(cbDrawAxis);

        label = new QLabel(frame);
        label->setObjectName(QString::fromUtf8("label"));

        verticalLayout_2->addWidget(label);

        spIterations = new QSpinBox(frame);
        spIterations->setObjectName(QString::fromUtf8("spIterations"));
        spIterations->setMinimum(1);
        spIterations->setMaximum(10000);
        spIterations->setValue(10);

        verticalLayout_2->addWidget(spIterations);

        cbRobustKernel = new QCheckBox(frame);
        cbRobustKernel->setObjectName(QString::fromUtf8("cbRobustKernel"));

        verticalLayout_2->addWidget(cbRobustKernel);

        cbOnlyLoop = new QCheckBox(frame);
        cbOnlyLoop->setObjectName(QString::fromUtf8("cbOnlyLoop"));
        cbOnlyLoop->setEnabled(false);

        verticalLayout_2->addWidget(cbOnlyLoop);

        coRobustKernel = new QComboBox(frame);
        coRobustKernel->setObjectName(QString::fromUtf8("coRobustKernel"));
        coRobustKernel->setEnabled(false);

        verticalLayout_2->addWidget(coRobustKernel);

        label_4 = new QLabel(frame);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setEnabled(false);

        verticalLayout_2->addWidget(label_4);

        leKernelWidth = new QLineEdit(frame);
        leKernelWidth->setObjectName(QString::fromUtf8("leKernelWidth"));
        leKernelWidth->setEnabled(false);

        verticalLayout_2->addWidget(leKernelWidth);

        label_2 = new QLabel(frame);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        verticalLayout_2->addWidget(label_2);

        coOptimizer = new QComboBox(frame);
        coOptimizer->setObjectName(QString::fromUtf8("coOptimizer"));

        verticalLayout_2->addWidget(coOptimizer);

        btnOptimizerParameters = new QPushButton(frame);
        btnOptimizerParameters->setObjectName(QString::fromUtf8("btnOptimizerParameters"));

        verticalLayout_2->addWidget(btnOptimizerParameters);

        verticalSpacer_2 = new QSpacerItem(20, 15, QSizePolicy::Minimum, QSizePolicy::Minimum);

        verticalLayout_2->addItem(verticalSpacer_2);

        cbxIniitialGuessMethod = new QComboBox(frame);
        cbxIniitialGuessMethod->addItem(QString());
        cbxIniitialGuessMethod->addItem(QString());
        cbxIniitialGuessMethod->setObjectName(QString::fromUtf8("cbxIniitialGuessMethod"));

        verticalLayout_2->addWidget(cbxIniitialGuessMethod);

        btnInitialGuess = new QPushButton(frame);
        btnInitialGuess->setObjectName(QString::fromUtf8("btnInitialGuess"));

        verticalLayout_2->addWidget(btnInitialGuess);

        btnReload = new QPushButton(frame);
        btnReload->setObjectName(QString::fromUtf8("btnReload"));

        verticalLayout_2->addWidget(btnReload);

        btnSetZero = new QPushButton(frame);
        btnSetZero->setObjectName(QString::fromUtf8("btnSetZero"));

        verticalLayout_2->addWidget(btnSetZero);

        btnOptimize = new QPushButton(frame);
        btnOptimize->setObjectName(QString::fromUtf8("btnOptimize"));

        verticalLayout_2->addWidget(btnOptimize);

        btnForceStop = new QPushButton(frame);
        btnForceStop->setObjectName(QString::fromUtf8("btnForceStop"));

        verticalLayout_2->addWidget(btnForceStop);

        btnQuit = new QPushButton(frame);
        btnQuit->setObjectName(QString::fromUtf8("btnQuit"));

        verticalLayout_2->addWidget(btnQuit);

        verticalSpacer = new QSpacerItem(20, 364, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);


        horizontalLayout->addWidget(frame);

        splitter = new QSplitter(centralwidget);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(splitter->sizePolicy().hasHeightForWidth());
        splitter->setSizePolicy(sizePolicy1);
        splitter->setOrientation(Qt::Vertical);
        viewer = new g2o::G2oQGLViewer(splitter);
        viewer->setObjectName(QString::fromUtf8("viewer"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(100);
        sizePolicy2.setHeightForWidth(viewer->sizePolicy().hasHeightForWidth());
        viewer->setSizePolicy(sizePolicy2);
        splitter->addWidget(viewer);
        plainTextEdit = new QPlainTextEdit(splitter);
        plainTextEdit->setObjectName(QString::fromUtf8("plainTextEdit"));
        QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(1);
        sizePolicy3.setHeightForWidth(plainTextEdit->sizePolicy().hasHeightForWidth());
        plainTextEdit->setSizePolicy(sizePolicy3);
        plainTextEdit->setReadOnly(true);
        splitter->addWidget(plainTextEdit);

        horizontalLayout->addWidget(splitter);

        BaseMainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(BaseMainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 22));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuView = new QMenu(menubar);
        menuView->setObjectName(QString::fromUtf8("menuView"));
        BaseMainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(BaseMainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        BaseMainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuView->menuAction());
        menuFile->addAction(actionLoad);
        menuFile->addAction(actionSave);
        menuFile->addSeparator();
        menuFile->addAction(actionSave_Screenshot);
        menuFile->addSeparator();
        menuFile->addAction(actionLoad_Viewer_State);
        menuFile->addAction(actionSave_Viewer_State);
        menuFile->addSeparator();
        menuFile->addAction(actionQuit);
        menuView->addAction(actionWhite_Background);
        menuView->addAction(actionDefault_Background);
        menuView->addSeparator();
        menuView->addAction(actionDump_Images);
        menuView->addSeparator();
        menuView->addAction(actionProperties);

        retranslateUi(BaseMainWindow);
        QObject::connect(cbRobustKernel, SIGNAL(toggled(bool)), label_4, SLOT(setEnabled(bool)));
        QObject::connect(cbRobustKernel, SIGNAL(toggled(bool)), leKernelWidth, SLOT(setEnabled(bool)));
        QObject::connect(btnQuit, SIGNAL(clicked()), BaseMainWindow, SLOT(close()));
        QObject::connect(cbRobustKernel, SIGNAL(toggled(bool)), coRobustKernel, SLOT(setEnabled(bool)));
        QObject::connect(cbRobustKernel, SIGNAL(toggled(bool)), cbOnlyLoop, SLOT(setEnabled(bool)));

        QMetaObject::connectSlotsByName(BaseMainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *BaseMainWindow)
    {
        BaseMainWindow->setWindowTitle(QCoreApplication::translate("BaseMainWindow", "g2o Viewer", nullptr));
        actionLoad->setText(QCoreApplication::translate("BaseMainWindow", "Load", nullptr));
        actionSave->setText(QCoreApplication::translate("BaseMainWindow", "Save", nullptr));
        actionQuit->setText(QCoreApplication::translate("BaseMainWindow", "Quit", nullptr));
        actionWhite_Background->setText(QCoreApplication::translate("BaseMainWindow", "White Background", nullptr));
        actionDefault_Background->setText(QCoreApplication::translate("BaseMainWindow", "Default Background", nullptr));
        actionDump_Images->setText(QCoreApplication::translate("BaseMainWindow", "Dump Images", nullptr));
        actionProperties->setText(QCoreApplication::translate("BaseMainWindow", "Draw Options", nullptr));
        actionSave_Screenshot->setText(QCoreApplication::translate("BaseMainWindow", "Save Screenshot", nullptr));
        actionSave_Viewer_State->setText(QCoreApplication::translate("BaseMainWindow", "Save Viewer State", nullptr));
        actionLoad_Viewer_State->setText(QCoreApplication::translate("BaseMainWindow", "Load Viewer State", nullptr));
        cbDrawAxis->setText(QCoreApplication::translate("BaseMainWindow", "Draw Axis", nullptr));
        label->setText(QCoreApplication::translate("BaseMainWindow", "# Iterations", nullptr));
        cbRobustKernel->setText(QCoreApplication::translate("BaseMainWindow", "Robust Kernel", nullptr));
#if QT_CONFIG(tooltip)
        cbOnlyLoop->setToolTip(QCoreApplication::translate("BaseMainWindow", "Only apply the robust kernel for loop closures", nullptr));
#endif // QT_CONFIG(tooltip)
        cbOnlyLoop->setText(QCoreApplication::translate("BaseMainWindow", "Non Sequential", nullptr));
        label_4->setText(QCoreApplication::translate("BaseMainWindow", "Kernel Width", nullptr));
        leKernelWidth->setText(QCoreApplication::translate("BaseMainWindow", "1.0", nullptr));
        label_2->setText(QCoreApplication::translate("BaseMainWindow", "Optimizer", nullptr));
#if QT_CONFIG(tooltip)
        btnOptimizerParameters->setToolTip(QCoreApplication::translate("BaseMainWindow", "Adjust the parameters of the optimizer", nullptr));
#endif // QT_CONFIG(tooltip)
        btnOptimizerParameters->setText(QCoreApplication::translate("BaseMainWindow", "Parameters", nullptr));
        cbxIniitialGuessMethod->setItemText(0, QCoreApplication::translate("BaseMainWindow", "Spanning Tree", nullptr));
        cbxIniitialGuessMethod->setItemText(1, QCoreApplication::translate("BaseMainWindow", "Odometry", nullptr));

#if QT_CONFIG(tooltip)
        cbxIniitialGuessMethod->setToolTip(QCoreApplication::translate("BaseMainWindow", "Which method is applied to compute the initial guess", nullptr));
#endif // QT_CONFIG(tooltip)
        btnInitialGuess->setText(QCoreApplication::translate("BaseMainWindow", "Initial Guess", nullptr));
        btnReload->setText(QCoreApplication::translate("BaseMainWindow", "Reload", nullptr));
        btnSetZero->setText(QCoreApplication::translate("BaseMainWindow", "SetZero", nullptr));
        btnOptimize->setText(QCoreApplication::translate("BaseMainWindow", "Optimize", nullptr));
        btnForceStop->setText(QCoreApplication::translate("BaseMainWindow", "Stop", nullptr));
        btnQuit->setText(QCoreApplication::translate("BaseMainWindow", "Quit", nullptr));
        menuFile->setTitle(QCoreApplication::translate("BaseMainWindow", "File", nullptr));
        menuView->setTitle(QCoreApplication::translate("BaseMainWindow", "View", nullptr));
    } // retranslateUi

};

namespace Ui {
    class BaseMainWindow: public Ui_BaseMainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BASE_MAIN_WINDOW_H
