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
 //Heavily edited and refactored to fix bugs by Keith Brown
#ifndef AXESDIALOG_H
#define AXESDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QList>
#include <QTextEdit>
#include "AxisDetails.h"
#include "ScaleDetails.h"
#include "GridDetails.h"
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
    AxesDialog(ApplicationWindow* app, Graph* g, Qt::WFlags fl = 0);
    virtual ~AxesDialog();

  public slots:
    void setCurrentScale(int axisPos);
    void showGeneralPage();
    void showAxesPage();
    void showGridPage();
    int exec();

  private slots:
    bool pressToGraph();
    void apply();
    void accept();
    void updateGrid();
    void changeMinorTicksLength(int minLength);
    void changeMajorTicksLength(int majLength);
    void pageChanged(QWidget *page);
    void setModified();

  protected:
    //! generate UI for the axes page
    void initAxesPage();
    //! generate UI for the scales page
    void initScalesPage();
    //! generate UI for the grid page
    void initGridPage();
    //! generate UI for the general page
    void initGeneralPage();

    ApplicationWindow* m_app;
    Graph *m_graph;
	  QStackedLayout *m_scalePrefsArea, *m_axesPrefsArea, *m_gridPrefsArea;

    //common widgets
    QPushButton *m_btnApply, *m_btnOk, *m_btnCancel;
    QTabWidget *m_generalDialog;
    QWidget *m_scalesPage, *m_gridPage, *m_axesPage, *m_generalPage, *m_lastPage;

    QHBoxLayout *scalesLayout, *axesLayout;
    QListWidget *m_lstScales, *m_lstGrid, *m_lstAxes;
    QSpinBox *m_spnAxesLinewidth, *m_spnMajorTicksLength, *m_spnMinorTicksLength;
    QCheckBox *m_chkBackbones, *m_chkAntialiseGrid;
    QComboBox *m_cmbApplyGridFormat;

  private:

    ///A map of QListWidgetItem objects to their Axis details objects
    QList<AxisDetails*> m_Axis_list;
    ///A map of QListWidgetItem objects to their Scale details objects
    QList<ScaleDetails*> m_Scale_list;
    ///A map of QListWidgetItem objects to their Scale details objects
    QList<GridDetails*> m_Grid_list;
    bool m_generalModified;
};

#endif
