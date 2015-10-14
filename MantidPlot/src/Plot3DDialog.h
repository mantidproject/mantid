/***************************************************************************
    File                 : Plot3DDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2004-2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Surface plot options dialog

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
#ifndef PLOT3DDIALOG_H
#define PLOT3DDIALOG_H

#include "Graph3D.h"
#include <QCheckBox>

class QComboBox;
class QLabel;
class QLineEdit;
class QTextEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTabWidget;
class QWidget;
class QStringList;
class QStackedWidget;
class QDoubleSpinBox;
class ColorButton;
class TextFormatButtons;

using namespace Qwt3D;

//! Surface plot options dialog
class Plot3DDialog : public QDialog
{
    Q_OBJECT

public:
    Plot3DDialog( QWidget* parent, Qt::WFlags fl = 0 );
	void setPlot(Graph3D *);

	void showTitleTab();
	void showAxisTab();
	void showGeneralTab();

private slots:
	void accept();
	bool updatePlot();

	void pickTitleFont();
	void viewAxisOptions(int axis);
	QFont axisFont(int axis);
	void pickAxisLabelFont();
	void pickNumbersFont();

	QStringList scaleOptions(int axis, double start, double end,
							const QString& majors, const QString& minors);
	void viewScaleLimits(int axis);
	void disableMeshOptions();
	void showBarsTab(double rad);
	void showPointsTab(double rad, bool smooth);
	void showConesTab(double rad, int quality);
	void showCrossHairTab(double rad, double linewidth, bool smooth, bool boxed);

	void worksheet();

	void initPointsOptionsStack();
	void changeZoom(int);
	void changeTransparency(int val);
    void pickDataColorMap();

private:
    void initScalesPage();
	void initAxesPage();
	void initTitlePage();
	void initColorsPage();
	void initGeneralPage();

    Graph3D *d_plot;
	QFont titleFont, xAxisFont,yAxisFont,zAxisFont, numbersFont;
	QStringList labels, scales, tickLengths;
    QDoubleSpinBox *boxMeshLineWidth;
    QPushButton* buttonApply;
    QPushButton* buttonOk;
    QPushButton* buttonCancel;
	QPushButton *btnTitleFont, *btnLabelFont;
    QPushButton *btnNumbersFont, *btnTable, *btnColorMap;
	ColorButton *btnBackground, *btnMesh, *btnAxes, *btnTitleColor, *btnLabels;
	ColorButton *btnFromColor, *btnToColor, *btnNumbers, *btnGrid;
    QTabWidget* generalDialog;
	QWidget *scale, *colors, *general, *axes, *title, *bars, *points;
	QLineEdit *boxFrom, *boxTo;
	QTextEdit *boxTitle, *boxLabel;
	QSpinBox *boxMajors, *boxMinors;
	QGroupBox *TicksGroupBox, *AxesColorGroupBox;
	QSpinBox *boxResolution, *boxDistance, *boxTransparency;
	QCheckBox *boxLegend, *boxSmooth, *boxBoxed, *boxCrossSmooth, *boxOrthogonal;
	QListWidget *axesList, *axesList2;
	QComboBox *boxType, *boxPointStyle;
	QLineEdit *boxMajorLength, *boxMinorLength, *boxConesRad;
	QSpinBox *boxZoom, *boxXScale, *boxYScale, *boxZScale, *boxQuality;
	QLineEdit *boxSize, *boxBarsRad, *boxCrossRad, *boxCrossLinewidth;
	QStackedWidget *optionStack;
	QWidget *dotsPage, *conesPage, *crossPage;
	TextFormatButtons *titleFormatButtons, *axisTitleFormatButtons;
    double xScale;
    double yScale;
    double zScale;
    double zoom;

};

#endif
