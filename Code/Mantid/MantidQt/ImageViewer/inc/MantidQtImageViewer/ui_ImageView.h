/********************************************************************************
** Form generated from reading UI file 'ImageView.ui'
**
** Created: Fri Mar 30 14:05:20 2012
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IMAGEVIEW_H
#define UI_IMAGEVIEW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFormLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QScrollBar>
#include <QtGui/QSlider>
#include <QtGui/QSplitter>
#include <QtGui/QStatusBar>
#include <QtGui/QTableWidget>
#include <QtGui/QWidget>
#include "qwt_plot.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *action;
    QAction *actionClose;
    QAction *action_Hscroll;
    QAction *action_Vscroll;
    QAction *actionVertical_Cut;
    QAction *actionGraph_Rebinned_Data;
    QAction *actionHeat;
    QAction *actionGray;
    QAction *actionNegative_Gray;
    QAction *actionGreen_Yellow;
    QAction *actionRainbow;
    QAction *actionOptimal;
    QAction *actionMulti;
    QAction *actionSpectrum;
    QAction *actionClear_Selections;
    QAction *actionOverlaid;
    QAction *actionOffset_Vertically;
    QAction *actionOffset_Diagonally;
    QWidget *centralwidget;
    QGridLayout *gridLayout_5;
    QSplitter *left_right_splitter;
    QSplitter *vgraphSplitter;
    QwtPlot *v_graphPlot;
    QLabel *label_8;
    QTableWidget *v_graph_table;
    QSplitter *imageSplitter;
    QWidget *layoutWidget;
    QGridLayout *gridLayout;
    QwtPlot *imagePlot;
    QScrollBar *imageVerticalScrollBar;
    QScrollBar *imageHorizontalScrollBar;
    QwtPlot *h_graphPlot;
    QWidget *layoutWidget1;
    QGridLayout *gridLayout_4;
    QFormLayout *formLayout;
    QLabel *X_Min_label;
    QLineEdit *x_min_input;
    QLabel *X_Max_label;
    QLineEdit *x_max_input;
    QLabel *Step_Label;
    QLineEdit *step_input;
    QGridLayout *gridLayout_2;
    QSlider *intensity_slider;
    QLabel *intensity_label;
    QGridLayout *gridLayout_3;
    QLabel *graph_max_label;
    QSlider *graph_max_slider;
    QLabel *label_2;
    QWidget *color_scale;
    QLabel *table_label;
    QTableWidget *image_table;
    QLabel *label;
    QTableWidget *h_graph_table;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuEdit;
    QMenu *menuOptions;
    QMenu *menuColor_Map;
    QMenu *menuGraph_Selected;
    QMenu *menuHelp;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1251, 712);
        action = new QAction(MainWindow);
        action->setObjectName(QString::fromUtf8("action"));
        actionClose = new QAction(MainWindow);
        actionClose->setObjectName(QString::fromUtf8("actionClose"));
        action_Hscroll = new QAction(MainWindow);
        action_Hscroll->setObjectName(QString::fromUtf8("action_Hscroll"));
        action_Vscroll = new QAction(MainWindow);
        action_Vscroll->setObjectName(QString::fromUtf8("action_Vscroll"));
        actionVertical_Cut = new QAction(MainWindow);
        actionVertical_Cut->setObjectName(QString::fromUtf8("actionVertical_Cut"));
        actionGraph_Rebinned_Data = new QAction(MainWindow);
        actionGraph_Rebinned_Data->setObjectName(QString::fromUtf8("actionGraph_Rebinned_Data"));
        actionHeat = new QAction(MainWindow);
        actionHeat->setObjectName(QString::fromUtf8("actionHeat"));
        actionGray = new QAction(MainWindow);
        actionGray->setObjectName(QString::fromUtf8("actionGray"));
        actionNegative_Gray = new QAction(MainWindow);
        actionNegative_Gray->setObjectName(QString::fromUtf8("actionNegative_Gray"));
        actionGreen_Yellow = new QAction(MainWindow);
        actionGreen_Yellow->setObjectName(QString::fromUtf8("actionGreen_Yellow"));
        actionRainbow = new QAction(MainWindow);
        actionRainbow->setObjectName(QString::fromUtf8("actionRainbow"));
        actionOptimal = new QAction(MainWindow);
        actionOptimal->setObjectName(QString::fromUtf8("actionOptimal"));
        actionMulti = new QAction(MainWindow);
        actionMulti->setObjectName(QString::fromUtf8("actionMulti"));
        actionSpectrum = new QAction(MainWindow);
        actionSpectrum->setObjectName(QString::fromUtf8("actionSpectrum"));
        actionClear_Selections = new QAction(MainWindow);
        actionClear_Selections->setObjectName(QString::fromUtf8("actionClear_Selections"));
        actionOverlaid = new QAction(MainWindow);
        actionOverlaid->setObjectName(QString::fromUtf8("actionOverlaid"));
        actionOffset_Vertically = new QAction(MainWindow);
        actionOffset_Vertically->setObjectName(QString::fromUtf8("actionOffset_Vertically"));
        actionOffset_Diagonally = new QAction(MainWindow);
        actionOffset_Diagonally->setObjectName(QString::fromUtf8("actionOffset_Diagonally"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout_5 = new QGridLayout(centralwidget);
        gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
        left_right_splitter = new QSplitter(centralwidget);
        left_right_splitter->setObjectName(QString::fromUtf8("left_right_splitter"));
        left_right_splitter->setOrientation(Qt::Horizontal);
        vgraphSplitter = new QSplitter(left_right_splitter);
        vgraphSplitter->setObjectName(QString::fromUtf8("vgraphSplitter"));
        vgraphSplitter->setOrientation(Qt::Vertical);
        v_graphPlot = new QwtPlot(vgraphSplitter);
        v_graphPlot->setObjectName(QString::fromUtf8("v_graphPlot"));
        vgraphSplitter->addWidget(v_graphPlot);
        label_8 = new QLabel(vgraphSplitter);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        vgraphSplitter->addWidget(label_8);
        v_graph_table = new QTableWidget(vgraphSplitter);
        v_graph_table->setObjectName(QString::fromUtf8("v_graph_table"));
        vgraphSplitter->addWidget(v_graph_table);
        left_right_splitter->addWidget(vgraphSplitter);
        imageSplitter = new QSplitter(left_right_splitter);
        imageSplitter->setObjectName(QString::fromUtf8("imageSplitter"));
        imageSplitter->setOrientation(Qt::Vertical);
        layoutWidget = new QWidget(imageSplitter);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        gridLayout = new QGridLayout(layoutWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        imagePlot = new QwtPlot(layoutWidget);
        imagePlot->setObjectName(QString::fromUtf8("imagePlot"));

        gridLayout->addWidget(imagePlot, 0, 0, 1, 1);

        imageVerticalScrollBar = new QScrollBar(layoutWidget);
        imageVerticalScrollBar->setObjectName(QString::fromUtf8("imageVerticalScrollBar"));
        imageVerticalScrollBar->setOrientation(Qt::Vertical);

        gridLayout->addWidget(imageVerticalScrollBar, 0, 1, 1, 1);

        imageHorizontalScrollBar = new QScrollBar(layoutWidget);
        imageHorizontalScrollBar->setObjectName(QString::fromUtf8("imageHorizontalScrollBar"));
        imageHorizontalScrollBar->setOrientation(Qt::Horizontal);

        gridLayout->addWidget(imageHorizontalScrollBar, 1, 0, 1, 1);

        imageSplitter->addWidget(layoutWidget);
        h_graphPlot = new QwtPlot(imageSplitter);
        h_graphPlot->setObjectName(QString::fromUtf8("h_graphPlot"));
        imageSplitter->addWidget(h_graphPlot);
        left_right_splitter->addWidget(imageSplitter);
        layoutWidget1 = new QWidget(left_right_splitter);
        layoutWidget1->setObjectName(QString::fromUtf8("layoutWidget1"));
        gridLayout_4 = new QGridLayout(layoutWidget1);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        gridLayout_4->setContentsMargins(0, 0, 0, 0);
        formLayout = new QFormLayout();
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        X_Min_label = new QLabel(layoutWidget1);
        X_Min_label->setObjectName(QString::fromUtf8("X_Min_label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, X_Min_label);

        x_min_input = new QLineEdit(layoutWidget1);
        x_min_input->setObjectName(QString::fromUtf8("x_min_input"));

        formLayout->setWidget(0, QFormLayout::FieldRole, x_min_input);

        X_Max_label = new QLabel(layoutWidget1);
        X_Max_label->setObjectName(QString::fromUtf8("X_Max_label"));

        formLayout->setWidget(1, QFormLayout::LabelRole, X_Max_label);

        x_max_input = new QLineEdit(layoutWidget1);
        x_max_input->setObjectName(QString::fromUtf8("x_max_input"));

        formLayout->setWidget(1, QFormLayout::FieldRole, x_max_input);

        Step_Label = new QLabel(layoutWidget1);
        Step_Label->setObjectName(QString::fromUtf8("Step_Label"));

        formLayout->setWidget(2, QFormLayout::LabelRole, Step_Label);

        step_input = new QLineEdit(layoutWidget1);
        step_input->setObjectName(QString::fromUtf8("step_input"));

        formLayout->setWidget(2, QFormLayout::FieldRole, step_input);


        gridLayout_4->addLayout(formLayout, 0, 0, 1, 1);

        gridLayout_2 = new QGridLayout();
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        intensity_slider = new QSlider(layoutWidget1);
        intensity_slider->setObjectName(QString::fromUtf8("intensity_slider"));
        intensity_slider->setOrientation(Qt::Horizontal);

        gridLayout_2->addWidget(intensity_slider, 1, 0, 1, 1);

        intensity_label = new QLabel(layoutWidget1);
        intensity_label->setObjectName(QString::fromUtf8("intensity_label"));

        gridLayout_2->addWidget(intensity_label, 0, 0, 1, 1);


        gridLayout_4->addLayout(gridLayout_2, 1, 0, 1, 1);

        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        graph_max_label = new QLabel(layoutWidget1);
        graph_max_label->setObjectName(QString::fromUtf8("graph_max_label"));

        gridLayout_3->addWidget(graph_max_label, 0, 0, 1, 1);

        graph_max_slider = new QSlider(layoutWidget1);
        graph_max_slider->setObjectName(QString::fromUtf8("graph_max_slider"));
        graph_max_slider->setOrientation(Qt::Horizontal);

        gridLayout_3->addWidget(graph_max_slider, 1, 0, 1, 1);


        gridLayout_4->addLayout(gridLayout_3, 2, 0, 1, 1);

        label_2 = new QLabel(layoutWidget1);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout_4->addWidget(label_2, 3, 0, 1, 1);

        color_scale = new QWidget(layoutWidget1);
        color_scale->setObjectName(QString::fromUtf8("color_scale"));

        gridLayout_4->addWidget(color_scale, 4, 0, 1, 1);

        table_label = new QLabel(layoutWidget1);
        table_label->setObjectName(QString::fromUtf8("table_label"));

        gridLayout_4->addWidget(table_label, 5, 0, 1, 1);

        image_table = new QTableWidget(layoutWidget1);
        image_table->setObjectName(QString::fromUtf8("image_table"));

        gridLayout_4->addWidget(image_table, 6, 0, 1, 1);

        label = new QLabel(layoutWidget1);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout_4->addWidget(label, 7, 0, 1, 1);

        h_graph_table = new QTableWidget(layoutWidget1);
        h_graph_table->setObjectName(QString::fromUtf8("h_graph_table"));

        gridLayout_4->addWidget(h_graph_table, 8, 0, 1, 1);

        left_right_splitter->addWidget(layoutWidget1);

        gridLayout_5->addWidget(left_right_splitter, 0, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 1251, 26));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuEdit = new QMenu(menubar);
        menuEdit->setObjectName(QString::fromUtf8("menuEdit"));
        menuOptions = new QMenu(menubar);
        menuOptions->setObjectName(QString::fromUtf8("menuOptions"));
        menuColor_Map = new QMenu(menuOptions);
        menuColor_Map->setObjectName(QString::fromUtf8("menuColor_Map"));
        menuGraph_Selected = new QMenu(menuOptions);
        menuGraph_Selected->setObjectName(QString::fromUtf8("menuGraph_Selected"));
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName(QString::fromUtf8("menuHelp"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuEdit->menuAction());
        menubar->addAction(menuOptions->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionClose);
        menuEdit->addAction(action_Vscroll);
        menuEdit->addAction(action_Hscroll);
        menuOptions->addAction(menuColor_Map->menuAction());
        menuOptions->addAction(menuGraph_Selected->menuAction());
        menuOptions->addAction(actionGraph_Rebinned_Data);
        menuColor_Map->addAction(actionHeat);
        menuColor_Map->addAction(actionGray);
        menuColor_Map->addAction(actionNegative_Gray);
        menuColor_Map->addAction(actionGreen_Yellow);
        menuColor_Map->addAction(actionRainbow);
        menuColor_Map->addAction(actionOptimal);
        menuColor_Map->addAction(actionMulti);
        menuColor_Map->addAction(actionSpectrum);
        menuGraph_Selected->addAction(actionClear_Selections);
        menuGraph_Selected->addAction(actionOverlaid);
        menuGraph_Selected->addAction(actionOffset_Vertically);
        menuGraph_Selected->addAction(actionOffset_Diagonally);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
        action->setText(QApplication::translate("MainWindow", "Vertical Scroll Bar", 0, QApplication::UnicodeUTF8));
        actionClose->setText(QApplication::translate("MainWindow", "Close", 0, QApplication::UnicodeUTF8));
        action_Hscroll->setText(QApplication::translate("MainWindow", "Horizontal Scroll Bar", 0, QApplication::UnicodeUTF8));
        action_Vscroll->setText(QApplication::translate("MainWindow", "Vertical Scroll Bar", 0, QApplication::UnicodeUTF8));
        actionVertical_Cut->setText(QApplication::translate("MainWindow", "Vertical Cut", 0, QApplication::UnicodeUTF8));
        actionGraph_Rebinned_Data->setText(QApplication::translate("MainWindow", "Graph Rebinned Data", 0, QApplication::UnicodeUTF8));
        actionHeat->setText(QApplication::translate("MainWindow", "Heat", 0, QApplication::UnicodeUTF8));
        actionGray->setText(QApplication::translate("MainWindow", "Gray", 0, QApplication::UnicodeUTF8));
        actionNegative_Gray->setText(QApplication::translate("MainWindow", "Negative Gray", 0, QApplication::UnicodeUTF8));
        actionGreen_Yellow->setText(QApplication::translate("MainWindow", "Green-Yellow", 0, QApplication::UnicodeUTF8));
        actionRainbow->setText(QApplication::translate("MainWindow", "Rainbow", 0, QApplication::UnicodeUTF8));
        actionOptimal->setText(QApplication::translate("MainWindow", "Optimal", 0, QApplication::UnicodeUTF8));
        actionMulti->setText(QApplication::translate("MainWindow", "Multi", 0, QApplication::UnicodeUTF8));
        actionSpectrum->setText(QApplication::translate("MainWindow", "Spectrum", 0, QApplication::UnicodeUTF8));
        actionClear_Selections->setText(QApplication::translate("MainWindow", "Clear Selections", 0, QApplication::UnicodeUTF8));
        actionOverlaid->setText(QApplication::translate("MainWindow", "Overlaid", 0, QApplication::UnicodeUTF8));
        actionOffset_Vertically->setText(QApplication::translate("MainWindow", "Offset Vertically", 0, QApplication::UnicodeUTF8));
        actionOffset_Diagonally->setText(QApplication::translate("MainWindow", "Offset Diagonally", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("MainWindow", "Vertical Graph Info", 0, QApplication::UnicodeUTF8));
        X_Min_label->setText(QApplication::translate("MainWindow", "X Min", 0, QApplication::UnicodeUTF8));
        X_Max_label->setText(QApplication::translate("MainWindow", "X Max", 0, QApplication::UnicodeUTF8));
        Step_Label->setText(QApplication::translate("MainWindow", "Step (- for Log)", 0, QApplication::UnicodeUTF8));
        intensity_label->setText(QApplication::translate("MainWindow", "Intensity", 0, QApplication::UnicodeUTF8));
        graph_max_label->setText(QApplication::translate("MainWindow", "Graph Max", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("MainWindow", "Color Map (zero at center)", 0, QApplication::UnicodeUTF8));
        table_label->setText(QApplication::translate("MainWindow", "Image Info", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("MainWindow", "Horizontal Graph Info", 0, QApplication::UnicodeUTF8));
        menuFile->setTitle(QApplication::translate("MainWindow", "File", 0, QApplication::UnicodeUTF8));
        menuEdit->setTitle(QApplication::translate("MainWindow", "View", 0, QApplication::UnicodeUTF8));
        menuOptions->setTitle(QApplication::translate("MainWindow", "Options", 0, QApplication::UnicodeUTF8));
        menuColor_Map->setTitle(QApplication::translate("MainWindow", "Color Map", 0, QApplication::UnicodeUTF8));
        menuGraph_Selected->setTitle(QApplication::translate("MainWindow", "Graph Selected", 0, QApplication::UnicodeUTF8));
        menuHelp->setTitle(QApplication::translate("MainWindow", "Help", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMAGEVIEW_H
