/***************************************************************************
    File                 : FunctionDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Function dialog

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
#include "FunctionDialog.h"
#include "MyParser.h"
#include "ApplicationWindow.h"
#include "FunctionCurve.h"

#include <QTextEdit>
#include <QLineEdit>
#include <QLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QStackedWidget>
#include <QWidget>
#include <QMessageBox>

FunctionDialog::FunctionDialog( ApplicationWindow* app, Graph* g, Qt::WFlags fl )
: QDialog( g, fl ),
  d_app(app),
  graph(g)
{
    setObjectName( "FunctionDialog" );
	setWindowTitle( tr( "MantidPlot - Add function curve" ) );
	setSizeGripEnabled(true);

	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->addWidget(new QLabel(tr( "Curve type " )));
	boxType = new QComboBox();
	boxType->addItem( tr( "Function" ) );
	boxType->addItem( tr( "Parametric plot" ) );
	boxType->addItem( tr( "Polar plot" ) );
	hbox1->addWidget(boxType);

	optionStack = new QStackedWidget();
	optionStack->setFrameShape( QFrame::StyledPanel );
	optionStack->setFrameShadow( QStackedWidget::Plain );

	QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel(tr( "f(x)= " )), 0, 0);
	boxFunction = new QTextEdit();
	boxFunction->setMinimumWidth(350);
	gl1->addWidget(boxFunction, 0, 1);
	gl1->addWidget(new QLabel(tr( "From x= " )), 1, 0);
	boxFrom = new QLineEdit();
	boxFrom->setText("0");
	gl1->addWidget(boxFrom, 1, 1);
	gl1->addWidget(new QLabel(tr( "To x= " )), 2, 0);
	boxTo = new QLineEdit();
	boxTo->setText("1");
	gl1->addWidget(boxTo, 2, 1);
	gl1->addWidget(new QLabel(tr( "Points" )), 3, 0);
	boxPoints = new QSpinBox();
	boxPoints->setRange(2, 1000000);
	boxPoints->setSingleStep(100);
	boxPoints->setValue(100);
	gl1->addWidget(boxPoints, 3, 1);

	functionPage = new QWidget();
	functionPage->setLayout(gl1);
	optionStack->addWidget( functionPage );

	QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel(tr( "Parameter" )), 0, 0);
	boxParameter = new QLineEdit();
	boxParameter->setText("m");
	gl2->addWidget(boxParameter, 0, 1);
	gl2->addWidget(new QLabel(tr( "From" )), 1, 0);
	boxParFrom = new QLineEdit();
	boxParFrom->setText("0");
	gl2->addWidget(boxParFrom, 1, 1);
	gl2->addWidget(new QLabel(tr( "To" )), 2, 0);
	boxParTo = new QLineEdit();
	boxParTo->setText("1");
	gl2->addWidget(boxParTo, 2, 1);
	gl2->addWidget(new QLabel(tr( "x = " )), 3, 0);
	boxXFunction = new QComboBox( );
	boxXFunction->setEditable ( true );
	gl2->addWidget(boxXFunction, 3, 1);
	gl2->addWidget(new QLabel(tr( "y = " )), 4, 0);
	boxYFunction = new QComboBox( );
	boxYFunction->setEditable ( true );
	gl2->addWidget(boxYFunction, 4, 1);
	gl2->addWidget(new QLabel(tr( "Points" )), 5, 0);
	boxParPoints = new QSpinBox();
	boxParPoints->setRange(2, 1000000);
	boxParPoints->setSingleStep(100);
	boxParPoints->setValue(100);
	gl2->addWidget(boxParPoints, 5, 1);
	gl2->setRowStretch(6, 1);

	parametricPage = new QWidget();
	parametricPage->setLayout(gl2);
	optionStack->addWidget( parametricPage );

	QGridLayout *gl3 = new QGridLayout();
    gl3->addWidget(new QLabel(tr( "Parameter" )), 0, 0);
	boxPolarParameter = new QLineEdit();
	boxPolarParameter->setText ("t");
	gl3->addWidget(boxPolarParameter, 0, 1);
	gl3->addWidget(new QLabel(tr( "From" )), 2, 0);
	boxPolarFrom = new QLineEdit();
	boxPolarFrom->setText("0");
	gl3->addWidget(boxPolarFrom, 2, 1);
	gl3->addWidget(new QLabel(tr( "To" )), 3, 0);
	boxPolarTo = new QLineEdit();
	boxPolarTo->setText("pi");
	gl3->addWidget(boxPolarTo, 3, 1);
	gl3->addWidget(new QLabel(tr( "R =" )), 4, 0);
	boxPolarRadius = new QComboBox();
	boxPolarRadius->setEditable ( true );
	gl3->addWidget(boxPolarRadius, 4, 1);
	gl3->addWidget(new QLabel(tr( "Theta =" )), 5, 0);
	boxPolarTheta = new QComboBox();
	boxPolarTheta->setEditable ( true );
	gl3->addWidget(boxPolarTheta, 5, 1);
	gl3->addWidget(new QLabel(tr( "Points" )), 6, 0);
	boxPolarPoints = new QSpinBox();
	boxPolarPoints->setRange(2, 1000000);
	boxPolarPoints->setSingleStep(100);
	boxPolarPoints->setValue(100);
	gl3->addWidget(boxPolarPoints, 6, 1);
	gl3->setRowStretch(7, 1);

	polarPage = new QWidget();
	polarPage->setLayout(gl3);
	optionStack->addWidget( polarPage );

	buttonClear = new QPushButton(tr( "Clear Function" ));
	buttonClear->setAutoDefault(false);
	buttonOk = new QPushButton(tr( "Ok" ));
	buttonOk->setDefault(true);
	buttonCancel = new QPushButton(tr( "Close" ));
	buttonCancel->setAutoDefault(false);

	QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->addStretch();
	hbox2->addWidget(buttonClear);
	hbox2->addWidget(buttonOk);
	hbox2->addWidget(buttonCancel);

	QVBoxLayout *vbox1 = new QVBoxLayout();
    vbox1->addLayout(hbox1);
	vbox1->addWidget(optionStack);
	vbox1->addLayout(hbox2);

	setLayout(vbox1);
	setFocusProxy (boxFunction);

	connect( boxType, SIGNAL( activated(int) ), this, SLOT( raiseWidget(int) ) );
	connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( buttonClear, SIGNAL( clicked() ), this, SLOT(clearList() ) );

	curveID = -1;
}

void FunctionDialog::raiseWidget(int index)
{
	if (index)
		buttonClear->setText( tr( "Clear list" ) );
	else
		buttonClear->setText( tr( "Clear Function" ) );

	optionStack->setCurrentIndex(index);
}

void FunctionDialog::setCurveToModify(Graph *g, int curve)
{
	if (!g)
		return;

	graph = g;

  FunctionCurve *c = dynamic_cast<FunctionCurve *>(graph->curve(curve));
	if (!c)
		return;

	curveID = curve;
	QStringList formulas = c->formulas();

	if (c->functionType() == FunctionCurve::Normal){
		boxFunction->setText(formulas[0]);
		boxFrom->setText(QString::number(c->startRange(), 'g', 15));
		boxTo->setText(QString::number(c->endRange(), 'g', 15));
		boxPoints->setValue(c->dataSize());
	} else if (c->functionType() == FunctionCurve::Polar) {
		optionStack->setCurrentIndex(2);
		boxType->setCurrentItem(2);

		boxPolarRadius->setCurrentText(formulas[0]);
		boxPolarTheta->setCurrentText(formulas[1]);
		boxPolarParameter->setText(c->variable());
		boxPolarFrom->setText(QString::number(c->startRange(), 'g', 15));
		boxPolarTo->setText(QString::number(c->endRange(), 'g', 15));
		boxPolarPoints->setValue(c->dataSize());
	} else if (c->functionType() == FunctionCurve::Parametric) {
		boxType->setCurrentItem(1);
		optionStack->setCurrentIndex(1);

		boxXFunction->setCurrentText(formulas[0]);
		boxYFunction->setCurrentText(formulas[1]);
		boxParameter->setText(c->variable());
		boxParFrom->setText(QString::number(c->startRange(), 'g', 15));
		boxParTo->setText(QString::number(c->endRange(), 'g', 15));
		boxParPoints->setValue(c->dataSize());
	}
}

void FunctionDialog::clearList()
{
	int type=boxType->currentItem();
	switch (type)
	{
		case 0:
			boxFunction->clear();
			break;

		case 1:
			boxXFunction->clear();
			boxYFunction->clear();
			emit clearParamFunctionsList();
			break;

		case 2:
			boxPolarTheta->clear();
			boxPolarRadius->clear();
			emit clearPolarFunctionsList();
			break;
	}
}

void FunctionDialog::acceptFunction()
{
	QString from=boxFrom->text().lower();
	QString to=boxTo->text().lower();
	QString points=boxPoints->text().lower();

	double start, end;
	try
	{
		MyParser parser;
		parser.SetExpr(from.ascii());
		start=parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0, tr("MantidPlot - Start limit error"), QString::fromStdString(e.GetMsg()));
		boxFrom->setFocus();
		return;
	}
	try
	{
		MyParser parser;
		parser.SetExpr(to.ascii());
		end=parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0, tr("MantidPlot - End limit error"), QString::fromStdString(e.GetMsg()));
		boxTo->setFocus();
		return;
	}

	if (start>=end)
	{
		QMessageBox::critical(0, tr("MantidPlot - Input error"),
				tr("Please enter x limits that satisfy: from < end!"));
		boxTo->setFocus();
		return;
	}

	QString formula = boxFunction->text().simplified();
	bool error=false;

	try
	{
		MyParser parser;
		double x=start;
		parser.DefineVar("x", &x);
		parser.SetExpr(formula.ascii());
		parser.Eval();
    // cppcheck-suppress unreadVariable
		x=end;
		parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0, tr("MantidPlot - Input function error"), QString::fromStdString(e.GetMsg()));
		boxFunction->setFocus();
		error=true;
	}

	// Collecting all the information
	int type = boxType->currentItem();
	QStringList formulas;
	formulas+=formula;
	if (!error)
	{
    d_app->updateFunctionLists(type,formulas);
		if (!graph)
      d_app->newFunctionPlot(formulas, start, end, boxPoints->value(), "x", type);
		else {
			if (curveID >= 0)
				graph->modifyFunctionCurve(curveID, type, formulas, "x", start, end, boxPoints->value());
			else
				graph->addFunction(formulas, start, end, boxPoints->value(), "x", type);
		}
	}

}
void FunctionDialog::acceptParametric()
{
	QString from=boxParFrom->text().lower();
	QString to=boxParTo->text().lower();
	QString points=boxParPoints->text().lower();

	double start, end;
	try
	{
		MyParser parser;
		parser.SetExpr(from.ascii());
		start=parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0, tr("MantidPlot - Start limit error"), QString::fromStdString(e.GetMsg()));
		boxParFrom->setFocus();
		return;
	}

	try
	{
		MyParser parser;
		parser.SetExpr(to.ascii());
		end=parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0, tr("MantidPlot - End limit error"), QString::fromStdString(e.GetMsg()));
		boxParTo->setFocus();
		return;
	}

	if (start>=end)
	{
		QMessageBox::critical(0, tr("MantidPlot - Input error"),
				tr("Please enter parameter limits that satisfy: from < end!"));
		boxParTo->setFocus();
		return;
	}

	QString xformula=boxXFunction->currentText();
	QString yformula=boxYFunction->currentText();
	bool error=false;

	try
	{
		MyParser parser;
		double parameter=start;
		parser.DefineVar((boxParameter->text()).ascii(), &parameter);
		parser.SetExpr(xformula.ascii());
		parser.Eval();
    // cppcheck-suppress unreadVariable
		parameter=end;
		parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0, tr("MantidPlot - Input function error"), QString::fromStdString(e.GetMsg()));
		boxXFunction->setFocus();
		error=true;
	}
	try
	{
		MyParser parser;
		double parameter=start;
		parser.DefineVar((boxParameter->text()).ascii(), &parameter);
		parser.SetExpr(yformula.ascii());
		parser.Eval();
    // cppcheck-suppress unreadVariable
		parameter=end;
		parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0, tr("MantidPlot - Input function error"), QString::fromStdString(e.GetMsg()));
		boxYFunction->setFocus();
		error=true;
	}
	// Collecting all the information
	int type = boxType->currentItem();
	QStringList formulas;
	formulas+=xformula;
	formulas+=yformula;
	if (!error)
	{
    d_app->updateFunctionLists(type,formulas);
		if (!graph)
      d_app->newFunctionPlot(formulas, start, end, boxParPoints->value(), boxParameter->text(), type);
		else {
			if (curveID >= 0)
				graph->modifyFunctionCurve(curveID, type, formulas, boxParameter->text(), start, end, boxParPoints->value());
			else
				graph->addFunction(formulas, start, end, boxParPoints->value(), boxParameter->text(), type);
		}
	}
}

void FunctionDialog::acceptPolar()
{
	QString from=boxPolarFrom->text().lower();
	QString to=boxPolarTo->text().lower();
	QString points=boxPolarPoints->text().lower();

	double start, end;
	try
	{
		MyParser parser;
		parser.SetExpr(from.ascii());
		start=parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0, tr("MantidPlot - Start limit error"), QString::fromStdString(e.GetMsg()));
		boxPolarFrom->setFocus();
		return;
	}

	try
	{
		MyParser parser;
		parser.SetExpr(to.ascii());
		end=parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0, tr("MantidPlot - End limit error"), QString::fromStdString(e.GetMsg()));
		boxPolarTo->setFocus();
		return;
	}

	if (start>=end)
	{
		QMessageBox::critical(0, tr("MantidPlot - Input error"),
				tr("Please enter parameter limits that satisfy: from < end!"));
		boxPolarTo->setFocus();
		return;
	}

	QString rformula=boxPolarRadius->currentText();
	QString tformula=boxPolarTheta->currentText();
	bool error=false;

	try
	{
		MyParser parser;
		double parameter=start;;
		parser.DefineVar((boxPolarParameter->text()).ascii(), &parameter);
		parser.SetExpr(rformula.ascii());
		parser.Eval();
    // cppcheck-suppress unreadVariable
		parameter=end;
		parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0, tr("MantidPlot - Input function error"), QString::fromStdString(e.GetMsg()));
		boxPolarRadius->setFocus();
		error=true;
	}
	try
	{
		MyParser parser;
		double parameter=start;;
		parser.DefineVar((boxPolarParameter->text()).ascii(), &parameter);
		parser.SetExpr(tformula.ascii());
		parser.Eval();
    // cppcheck-suppress unreadVariable
		parameter=end;
		parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0, tr("MantidPlot - Input function error"), QString::fromStdString(e.GetMsg()));
		boxPolarTheta->setFocus();
		error=true;
	}
	// Collecting all the information
	int type = boxType->currentItem();
	QStringList formulas;
	formulas+=rformula;
	formulas+=tformula;
	if (!error)
	{
    d_app->updateFunctionLists(type,formulas);

		if (!graph)
      d_app->newFunctionPlot(formulas, start, end, boxPolarPoints->value(), boxPolarParameter->text(), type);
		else {
			if (curveID >= 0)
				graph->modifyFunctionCurve(curveID, type, formulas, boxPolarParameter->text(), start, end, boxPolarPoints->value());
			else
				graph->addFunction(formulas, start, end, boxPolarPoints->value(), boxPolarParameter->text(), type);
		}
	}
}

void FunctionDialog::accept()
{
	switch (boxType->currentIndex())
	{
		case 0:
			acceptFunction();
			break;

		case 1:
			acceptParametric();
			break;

		case 2:
			acceptPolar();
			break;
	}
	close();
}

void FunctionDialog::insertParamFunctionsList(const QStringList& xList, const QStringList& yList)
{
	boxXFunction->insertItems (0, xList);
	boxYFunction->insertItems (0, yList);
}

void FunctionDialog::insertPolarFunctionsList(const QStringList& rList, const QStringList& thetaList)
{
	boxPolarRadius->insertItems (0, rList);
	boxPolarTheta->insertItems (0, thetaList);
}
