/***************************************************************************
    File                 : SurfaceDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Define surface plot dialog

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
#ifndef SURFACEDIALOG_H
#define SURFACEDIALOG_H

#include <QDialog>

class QPushButton;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QStackedWidget;
class QSpinBox;
class Graph3D;

//! Define surface plot dialog
class SurfaceDialog : public QDialog
{
    Q_OBJECT

public:
    SurfaceDialog( QWidget* parent = 0, Qt::WFlags fl = 0 );

public slots:
	void accept();
	void setFunction(Graph3D *);
	void setParametricSurface(Graph3D *);

private slots:
	void clearList();

private:
	Graph3D *d_graph;

	void acceptParametricSurface();
	void acceptFunction();
	void initFunctionPage();
	void initParametricSurfacePage();

	QWidget* functionPage;
	QWidget* parametricPage;
	QStackedWidget* optionStack;
    QPushButton* buttonOk;
	QPushButton* buttonCancel;
	QPushButton* buttonClear;
	QComboBox* boxType;
	QComboBox* boxFunction;
	QLineEdit* boxXFrom;
	QLineEdit* boxXTo;
	QLineEdit* boxYFrom;
	QLineEdit* boxYTo;
	QLineEdit* boxZFrom;
	QLineEdit* boxZTo;

	QLineEdit* boxX;
	QLineEdit* boxY;
	QLineEdit* boxZ;

	QLineEdit* boxUFrom;
	QLineEdit* boxUTo;
	QLineEdit* boxVFrom;
	QLineEdit* boxVTo;

	QCheckBox *boxUPeriodic, *boxVPeriodic;
	QSpinBox *boxColumns, *boxRows, *boxFuncColumns, *boxFuncRows;
};

#endif
