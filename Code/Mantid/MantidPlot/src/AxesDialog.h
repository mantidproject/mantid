/***************************************************************************
 File                 : AxesDialog.h
 Project              : QtiPlot
 --------------------------------------------------------------------
 Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
 Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
 Description          : General plot options dialog

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef AXESDIALOG_H
#define AXESDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QList>
#include <QTextEdit>
#include "AxisAxisDetails.h"
#include "ScaleAxisDetails.h"
//#include "MantidKernel/Logger.h"

class QTimeEdit;
class QDateTimeEdit;
class QListWidget;
class QListWidgetItem;
class QCheckBox;
class QGroupBox;
class QComboBox;
class QLabel;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTabWidget;
class QStackedLayout;
class QVBoxLayout;
class QHBoxLayout;
class QWidget;
class QStringList;
class ColorBox;
class ColorButton;
class Graph;
class TextFormatButtons;
class DoubleSpinBox;
class Grid;
class ApplicationWindow;

//! General plot options dialog
/**
 * Remark: Don't use this dialog as a non modal dialog!
 */
class AxesDialog: public QDialog
{
  Q_OBJECT

  public:
    //! Constructor
    /**
     * @param parent :: parent widget
     * @param fl :: window flags
     */
    AxesDialog(ApplicationWindow* app, Graph* g, Qt::WFlags fl = 0);

  public slots:
    void setCurrentScale(int axisPos);
    void showGeneralPage();
    void showAxesPage();
    void showGridPage();
    //void showFormulaBox();

    //! Shows the dialog as a modal dialog
    /**
     * Show the dialog as a modal dialog and do
     * some initialization.
     */
    int exec();

  private slots:
    bool apply();
    void updateScale();
    void majorGridEnabled(bool on);
    void minorGridEnabled(bool on);
    void showGridOptions(int axis);
    void accept();
    void drawFrame(bool framed);
    int mapToQwtAxis(int axis);
    int mapToQwtAxisId();
    void updateGrid();
    void updateFrame(int);
    void changeMinorTicksLength(int minLength);
    void changeMajorTicksLength(int majLength);
    void pickCanvasFrameColor();
    void changeAxesLinewidth(int);
    void drawAxesBackbones(bool);
    void pageChanged(QWidget *page);

  protected:
    //! generate UI for the axes page
    void initAxesPage();
    //! generate UI for the scales page
    void initScalesPage();
    //! generate UI for the grid page
    void initGridPage();
    //! generate UI for the general page
    void initGeneralPage();
    //! Modifies the grid
    void applyChangesToGrid(Grid *grid);
    void setGraph(Graph *g);

    ApplicationWindow* d_app;
    Graph *d_graph;
    //QFrame *scalePrefsArea, *axesPrefsArea;
	  QStackedLayout *scalePrefsArea, *axesPrefsArea;
//common widgets
    QPushButton* buttonApply, *buttonOk, *buttonCancel;
    QTabWidget* generalDialog;
    QWidget *scalesPage, *gridPage, *axesPage, *generalPage;

    QHBoxLayout *scalesLayout, *axesLayout;
    QListWidget* axesList;
    QCheckBox* boxMajorGrid;
    QCheckBox* boxMinorGrid;
    QComboBox* boxTypeMajor;
    ColorBox* boxColorMinor;
    ColorBox* boxColorMajor;
    ColorButton *boxCanvasColor;
    DoubleSpinBox* boxWidthMajor;
    QComboBox* boxTypeMinor;
    DoubleSpinBox* boxWidthMinor;
    QCheckBox* boxXLine;
    QCheckBox* boxYLine;
    QListWidget* axesGridList;
    QListWidget* axesTitlesList;

    QSpinBox *boxFrameWidth, *boxAxesLinewidth;
    QCheckBox *boxBackbones;
    QGroupBox *boxFramed;
    QSpinBox *boxMajorTicksLength, *boxMinorTicksLength, *boxBorderWidth;
    QComboBox *boxGridXAxis, *boxGridYAxis;
    ColorButton *boxFrameColor;

    QCheckBox *boxAntialiseGrid;
    QComboBox *boxApplyGridFormat;
    //! Last selected tab
    QWidget* lastPage;
    bool m_updatePlot;

    //static Mantid::Kernel::Logger &g_log;

  private:

    ///A map of QListWidgetItem objects to their Axis details objects
    QMap<QListWidgetItem*, AxisAxisDetails*> m_Axis_map;
    ///A map of QListWidgetItem objects to their Scale details objects
    QMap<QListWidgetItem*, ScaleAxisDetails*> m_Scale_map;
	int oldaxis;
};

#endif
