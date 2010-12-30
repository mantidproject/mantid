/***************************************************************************
	File                 : ColorMapDialog.cpp
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
#include "ColorMapDialog.h"
#include "ColorMapEditor.h"
#include "Matrix.h"
#include "MatrixCommand.h"

#include <QPushButton>
#include <QLayout>

ColorMapDialog::ColorMapDialog(QWidget* parent, Qt::WFlags fl)
	: QDialog(parent, fl)
{
setObjectName( "ColorMapDialog" );
setWindowTitle(tr("MantidPlot") + " - " + tr("Custom Color Map"));
editor = new ColorMapEditor();
	
applyBtn = new QPushButton(tr("&Apply"));
connect(applyBtn, SIGNAL(clicked()), this, SLOT(apply()));

closeBtn = new QPushButton(tr("&Close"));
connect(closeBtn, SIGNAL(clicked()), this, SLOT(reject()));

QHBoxLayout* hb = new QHBoxLayout();
hb->setSpacing(5);
hb->addStretch();
hb->addWidget(applyBtn);
hb->addWidget(closeBtn);
hb->addStretch();
	
QVBoxLayout* vl = new QVBoxLayout(this);
vl->setSpacing(0);
vl->addWidget(editor);	
vl->addLayout(hb);
	
setMaximumWidth(editor->width() + 20);
}

void ColorMapDialog::setMatrix(Matrix *m)
{
	if (!m)
		return;
	
	d_matrix = m;
	
	double minValue = 0.0, maxValue = 0.0;
	m->range(&minValue, &maxValue);
	
	editor->setRange(minValue, maxValue);
	editor->setColorMap(m->colorMap());
}

void ColorMapDialog::apply()
{
	d_matrix->undoStack()->push(new MatrixSetColorMapCommand(d_matrix, d_matrix->colorMapType(), 
						d_matrix->colorMap(), Matrix::Custom, editor->colorMap(), tr("Set Custom Palette")));
	d_matrix->setColorMap(editor->colorMap());
}
