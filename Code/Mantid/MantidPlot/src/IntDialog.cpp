/***************************************************************************
    File                 : IntDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2004-2007 by Ion Vasilief, Vasileios Gkanis
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Integration options dialog

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
#include "IntDialog.h"
#include "ApplicationWindow.h"
#include "Graph.h"
#include "Integration.h"
#include "DoubleSpinBox.h"

#include <QGroupBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QLayout>

IntDialog::IntDialog(QWidget* parent, Graph *g, Qt::WFlags fl )
    : QDialog( parent, fl),
	d_graph(g)
{
    setName( "IntegrationDialog" );
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(tr("MantidPlot - Integration Options"));
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

    QGroupBox *gb1 = new QGroupBox();
    QGridLayout *gl1 = new QGridLayout(gb1);
	gl1->addWidget(new QLabel(tr("Function")), 0, 0);
	boxName = new QTextEdit();
	boxName->setMaximumHeight(60);
	gl1->addWidget(boxName, 0, 1);

    gl1->addWidget(new QLabel(tr("Variable")), 1, 0);
	boxVariable = new QLineEdit();
	boxVariable->setText("x");
	gl1->addWidget(boxVariable, 1, 1);

	gl1->addWidget(new QLabel(tr("Order (1 - 5, 1 = Trapezoidal Rule)")), 2, 0);
	boxOrder = new QSpinBox();
	boxOrder->setRange(1, 5);
	gl1->addWidget(boxOrder, 2, 1);

	gl1->addWidget(new QLabel(tr("Number of iterations (Max=20)")), 3, 0);
	boxSteps = new QSpinBox();
	boxSteps->setRange(1, 20);
	boxSteps->setValue(20);
	gl1->addWidget(boxSteps, 3, 1);

	QLocale locale = ((ApplicationWindow *)parent)->locale();
	gl1->addWidget(new QLabel(tr("Tolerance")), 4, 0);
	boxTol = new DoubleSpinBox();
	boxTol->setLocale(locale);
	boxTol->setValue(0.01);
	boxTol->setMinimum(0.0);
	boxTol->setSingleStep(0.001);
	gl1->addWidget(boxTol, 4, 1);

	gl1->addWidget(new QLabel(tr("Lower limit")), 5, 0);
	boxStart = new DoubleSpinBox();
	boxStart->setLocale(locale);
	gl1->addWidget(boxStart, 5, 1);

	gl1->addWidget(new QLabel(tr("Upper limit")), 6, 0);
	boxEnd = new DoubleSpinBox();
	boxEnd->setLocale(locale);
	boxEnd->setValue(1.0);
	gl1->addWidget(boxEnd, 6, 1);

    boxPlot = new QCheckBox(tr("&Plot area"));
	boxPlot->setChecked(true);
    gl1->addWidget(boxPlot, 7, 1);
    gl1->setRowStretch(8, 1);

	buttonOk = new QPushButton(tr( "&Integrate" ));
    buttonOk->setDefault( true );
    buttonCancel = new QPushButton(tr("&Close" ));

	QVBoxLayout *vl = new QVBoxLayout();
 	vl->addWidget(buttonOk);
	vl->addWidget(buttonCancel);
    vl->addStretch();

    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->addWidget(gb1);
    hb->addLayout(vl);

    connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

void IntDialog::accept()
{
    QString formula = boxName->text().remove("\n");
	Integration *i = new Integration(formula, boxVariable->text(),
                    (ApplicationWindow *)this->parent(), d_graph, boxStart->value(), boxEnd->value());
	i->setTolerance(boxTol->text().toDouble());
	i->setMaximumIterations(boxSteps->value());
	i->setMethodOrder(boxOrder->value());
	if (d_graph && boxPlot->isChecked())
	    i->enableGraphicsDisplay(true, d_graph);
    i->run();
    delete i;
}
