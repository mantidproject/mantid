/***************************************************************************
	File                 : ColorMapDialog.h
	Project              : QtiPlot
--------------------------------------------------------------------
	Copyright            : (C) 2007 by Ion Vasilief
	Email (use @ for *)  : ion_vasilief*yahoo.fr
	Description          : A QwtLinearColorMap Editor Dialog
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
#ifndef COLORMAPDIALOG_H
#define COLORMAPDIALOG_H

#include <QDialog>

class QPushButton;
class ColorMapEditor;
class Matrix;
	
class ColorMapDialog: public QDialog
{
    Q_OBJECT

public:
	ColorMapDialog(QWidget* parent=0, Qt::WFlags fl = 0);
	void setMatrix(Matrix *m);
	
protected slots:
	void apply();

private:
	QPushButton *applyBtn, *closeBtn;
	ColorMapEditor *editor;
	Matrix *d_matrix;
};
   
#endif
