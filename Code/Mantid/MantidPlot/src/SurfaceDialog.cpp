/***************************************************************************
    File                 : SurfaceDialog.cpp
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
#include "SurfaceDialog.h"
#include "MyParser.h"
#include "ApplicationWindow.h"
#include "Graph3D.h"
#include "UserFunction.h"

#include <QMessageBox>
#include <QLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QStackedWidget>
#include <QCompleter>
#include <QApplication>

SurfaceDialog::SurfaceDialog( QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl )
{
	setName( "SurfaceDialog" );
	setWindowTitle(tr("MantidPlot - Define surface plot"));
    setSizeGripEnabled( true );

	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->addWidget(new QLabel(tr( "Surface type" )));
	boxType = new QComboBox();
	boxType->addItem( tr( "Function" ) );
	boxType->addItem( tr( "Parametric" ) );
	hbox1->addWidget(boxType);
	hbox1->addStretch();

	optionStack = new QStackedWidget();

	initFunctionPage();
	initParametricSurfacePage();

	buttonClear = new QPushButton(tr("Clear &list"));
	buttonOk = new QPushButton(tr("&OK"));
    buttonOk->setDefault(true);
    buttonCancel = new QPushButton(tr("&Close"));

    QBoxLayout *bl2 = new QBoxLayout ( QBoxLayout::LeftToRight);
    bl2->addStretch();
	bl2->addWidget(buttonClear);
	bl2->addWidget(buttonOk);
	bl2->addWidget(buttonCancel);

	QVBoxLayout* vl = new QVBoxLayout(this);
    vl->addLayout(hbox1);
	vl->addWidget(optionStack);
	vl->addLayout(bl2);

	ApplicationWindow *app = (ApplicationWindow *)parent;
	if (app){
		boxFunction->insertItems(0, app->surfaceFunc);
		boxX->setCompleter (new QCompleter(app->d_param_surface_func));
		boxY->setCompleter (new QCompleter(app->d_param_surface_func));
		boxZ->setCompleter (new QCompleter(app->d_param_surface_func));
	}

	d_graph = 0;
    setFocusProxy(boxFunction);

	connect( boxType, SIGNAL(activated(int)), optionStack, SLOT(setCurrentIndex(int)));
	connect( buttonClear, SIGNAL(clicked()), this, SLOT(clearList()));
    connect( buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect( buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void SurfaceDialog::initFunctionPage()
{
	boxFunction = new QComboBox();
	boxFunction->setEditable(true);

	QBoxLayout *bl1 = new QBoxLayout (QBoxLayout::LeftToRight);
	bl1->addWidget(new QLabel( tr("f(x,y)=")), 1);
	bl1->addWidget(boxFunction, 10);

    QGroupBox *gb1 = new QGroupBox(tr("X - axis"));

	boxXFrom = new QLineEdit();
	boxXFrom->setText(tr("-1"));

	boxXTo = new QLineEdit();
	boxXTo->setText(tr("1"));

    QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel( tr("From")), 0, 0);
    gl1->addWidget(boxXFrom, 0, 1);
    gl1->addWidget(new QLabel(tr("To")), 1, 0);
    gl1->addWidget(boxXTo, 1, 1);
    gl1->setRowStretch(2, 1);
    gb1->setLayout(gl1);

    QGroupBox *gb2 = new QGroupBox(tr("Y - axis"));
	boxYFrom = new QLineEdit();
	boxYFrom->setText(tr("-1"));

	boxYTo = new QLineEdit();
	boxYTo->setText(tr("1"));

    QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel( tr("From")), 0, 0);
    gl2->addWidget(boxYFrom, 0, 1);
    gl2->addWidget(new QLabel(tr("To")), 1, 0);
    gl2->addWidget(boxYTo, 1, 1);
    gl2->setRowStretch(2, 1);
    gb2->setLayout(gl2);

    QGroupBox *gb3 = new QGroupBox(tr("Z - axis"));
	boxZFrom = new QLineEdit();
	boxZFrom->setText(tr("-1"));

	boxZTo = new QLineEdit();
	boxZTo->setText(tr("1"));

    QGridLayout *gl3 = new QGridLayout();
    gl3->addWidget(new QLabel( tr("From")), 0, 0);
    gl3->addWidget(boxZFrom, 0, 1);
    gl3->addWidget(new QLabel(tr("To")), 1, 0);
    gl3->addWidget(boxZTo, 1, 1);
    gl3->setRowStretch(2, 1);
    gb3->setLayout(gl3);

	QBoxLayout *bl2 = new QBoxLayout (QBoxLayout::LeftToRight);
	bl2->addWidget(gb1);
	bl2->addWidget(gb2);
	bl2->addWidget(gb3);

	QGroupBox *gb4 = new QGroupBox(tr("Mesh"));
	boxFuncColumns = new QSpinBox();
	boxFuncColumns->setRange(1, 1000);
	boxFuncColumns->setValue(40);

	boxFuncRows = new QSpinBox();
	boxFuncRows->setRange(1, 1000);
	boxFuncRows->setValue(40);

	QGridLayout *hb4 = new QGridLayout(gb4);
    hb4->addWidget(new QLabel( tr("Columns")), 0, 0);
    hb4->addWidget(boxFuncColumns, 0, 1);
    hb4->addWidget(new QLabel(tr("Rows")), 1, 0);
    hb4->addWidget(boxFuncRows, 1, 1);

	functionPage = new QWidget();

	QVBoxLayout* vl = new QVBoxLayout(functionPage);
    vl->addLayout(bl1);
	vl->addLayout(bl2);
	vl->addWidget(gb4);

	optionStack->addWidget(functionPage);
}

void SurfaceDialog::initParametricSurfacePage()
{
	boxX = new QLineEdit();
	boxY = new QLineEdit();
	boxZ = new QLineEdit();

	QGroupBox *gb = new QGroupBox(tr("Equations"));
	QGridLayout *gl = new QGridLayout(gb);
    gl->addWidget(new QLabel( tr("X(u,v)=")), 0, 0);
    gl->addWidget(boxX, 0, 1);
    gl->addWidget(new QLabel(tr("Y(u,v)=")), 1, 0);
    gl->addWidget(boxY, 1, 1);
	gl->addWidget(new QLabel(tr("Z(u,v)=")), 2, 0);
    gl->addWidget(boxZ, 2, 1);
    gl->setRowStretch(3, 1);

    QGroupBox *gb1 = new QGroupBox(tr("u"));
	boxUFrom = new QLineEdit();
	boxUFrom->setText("0");
	boxUTo = new QLineEdit();
	boxUTo->setText("pi");
    QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel( tr("From")), 0, 0);
    gl1->addWidget(boxUFrom, 0, 1);
    gl1->addWidget(new QLabel(tr("To")), 1, 0);
    gl1->addWidget(boxUTo, 1, 1);
	boxUPeriodic = new QCheckBox(tr("Periodic"));
	gl1->addWidget(boxUPeriodic, 2, 1);
    gl1->setRowStretch(3, 1);
    gb1->setLayout(gl1);

    QGroupBox *gb2 = new QGroupBox(tr("v"));
	boxVFrom = new QLineEdit();
	boxVFrom->setText("0");

	boxVTo = new QLineEdit();
	boxVTo->setText("pi");

    QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel( tr("From")), 0, 0);
    gl2->addWidget(boxVFrom, 0, 1);
    gl2->addWidget(new QLabel(tr("To")), 1, 0);
    gl2->addWidget(boxVTo, 1, 1);
	boxVPeriodic = new QCheckBox(tr("Periodic"));
	gl2->addWidget(boxVPeriodic, 2, 1);
    gl2->setRowStretch(3, 1);
    gb2->setLayout(gl2);

	QGroupBox *gb3 = new QGroupBox(tr("Mesh"));
	boxColumns = new QSpinBox();
	boxColumns->setRange(1, 1000);
	boxColumns->setValue(40);

	boxRows = new QSpinBox();
	boxRows->setRange(1, 1000);
	boxRows->setValue(40);

    QGridLayout *gl3 = new QGridLayout();
    gl3->addWidget(new QLabel( tr("Columns")), 0, 0);
    gl3->addWidget(boxColumns, 0, 1);
    gl3->addWidget(new QLabel(tr("Rows")), 1, 0);
    gl3->addWidget(boxRows, 1, 1);
    gl3->setRowStretch(2, 1);
    gb3->setLayout(gl3);

	QBoxLayout *bl2 = new QBoxLayout (QBoxLayout::LeftToRight);
	bl2->addWidget(gb1);
	bl2->addWidget(gb2);
	bl2->addWidget(gb3);

	parametricPage = new QWidget();

	QVBoxLayout* vl = new QVBoxLayout(parametricPage);
    vl->addWidget(gb);
	vl->addLayout(bl2);

	optionStack->addWidget(parametricPage);
}

void SurfaceDialog::clearList()
{
    ApplicationWindow *app = (ApplicationWindow *)this->parent();

    if (app && boxType->currentIndex()){
        app->d_param_surface_func.clear();
    }else{
        boxFunction->clear();
        if (app)
            app->clearSurfaceFunctionsList();
    }
}

void SurfaceDialog::setFunction(Graph3D *g)
{
	if (!g)
		return;

	d_graph = g;
	UserFunction *f = d_graph->userFunction();
	if (!f)
		return;

	boxFunction->setCurrentText(f->function());
	boxFuncColumns->setValue(f->columns());
	boxFuncRows->setValue(f->rows());

	boxXFrom->setText(QString::number(g->xStart()));
	boxXTo->setText(QString::number(g->xStop()));
	boxYFrom->setText(QString::number(g->yStart()));
	boxYTo->setText(QString::number(g->yStop()));
	boxZFrom->setText(QString::number(g->zStart()));
	boxZTo->setText(QString::number(g->zStop()));
}

void SurfaceDialog::accept()
{
	if (boxType->currentIndex())
		acceptParametricSurface();
	else
		acceptFunction();
}

void SurfaceDialog::acceptParametricSurface()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();

	MyParser parser;
	double u = 1.0, v = 1.0;
	parser.DefineVar("u", &u);
	parser.DefineVar("v", &v);

    int list_size = 15;
    QString x_formula = boxX->text();
	try {
		parser.SetExpr(x_formula.ascii());
		parser.Eval();
	} catch(mu::ParserError &e){
		QMessageBox::critical(app, tr("MantidPlot - X Formula Error"), QString::fromStdString(e.GetMsg()));
		boxX->setFocus();
		return;
	}

    app->d_param_surface_func.remove(x_formula);
	app->d_param_surface_func.push_front(x_formula);
	while ((int)app->d_param_surface_func.size() > list_size)
		app->d_param_surface_func.pop_back();

    QString y_formula = boxY->text();
	try {
		parser.SetExpr(y_formula.ascii());
		parser.Eval();
	} catch(mu::ParserError &e){
		QMessageBox::critical(app, tr("MantidPlot - Y Formula Error"), QString::fromStdString(e.GetMsg()));
		boxY->setFocus();
		return;
	}

    app->d_param_surface_func.remove(y_formula);
	app->d_param_surface_func.push_front(y_formula);
	while ((int)app->d_param_surface_func.size() > list_size)
		app->d_param_surface_func.pop_back();

    QString z_formula = boxZ->text();
	try {
		parser.SetExpr(z_formula.ascii());
		parser.Eval();
	} catch(mu::ParserError &e){
		QMessageBox::critical(app, tr("MantidPlot - Z Formula Error"), QString::fromStdString(e.GetMsg()));
		boxZ->setFocus();
		return;
	}

    app->d_param_surface_func.remove(z_formula);
	app->d_param_surface_func.push_front(z_formula);
	while ((int)app->d_param_surface_func.size() > list_size)
		app->d_param_surface_func.pop_back();

	QString ufrom = boxUFrom->text().lower();
	QString uto = boxUTo->text().lower();
	QString vfrom = boxVFrom->text().lower();
	QString vto = boxVTo->text().lower();
	double ul, ur, vl, vr;
	try{
		parser.SetExpr(ufrom.ascii());
		ul = parser.Eval();
	}
	catch(mu::ParserError &e){
		QMessageBox::critical(app, tr("MantidPlot - u start limit error"), QString::fromStdString(e.GetMsg()));
		boxUFrom->setFocus();
		return;
	}

	try{
		parser.SetExpr(uto.ascii());
		ur = parser.Eval();
	}
	catch(mu::ParserError &e){
		QMessageBox::critical(app, tr("MantidPlot - u end limit error"), QString::fromStdString(e.GetMsg()));
		boxUTo->setFocus();
		return;
	}

	try{
		parser.SetExpr(vfrom.ascii());
		vl = parser.Eval();
	}
	catch(mu::ParserError &e){
		QMessageBox::critical(app, tr("MantidPlot - v start limit error"), QString::fromStdString(e.GetMsg()));
		boxVFrom->setFocus();
		return;
	}

	try{
		parser.SetExpr(vto.ascii());
		vr = parser.Eval();
	}
	catch(mu::ParserError &e){
		QMessageBox::critical(app, tr("MantidPlot - u end limit error"), QString::fromStdString(e.GetMsg()));
		boxVTo->setFocus();
		return;
	}

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	if (!d_graph)
		app->plotParametricSurface(x_formula, y_formula, z_formula,
							   ul, ur, vl, vr, boxColumns->value(), boxRows->value(),
							   boxUPeriodic->isChecked(), boxVPeriodic->isChecked());
	else
		d_graph->addParametricSurface(x_formula, y_formula, z_formula,
							   ul, ur, vl, vr, boxColumns->value(), boxRows->value(),
							   boxUPeriodic->isChecked(), boxVPeriodic->isChecked());
    QApplication::restoreOverrideCursor();
	close();
}

void SurfaceDialog::acceptFunction()
{
ApplicationWindow *app = (ApplicationWindow *)this->parent();

QString Xfrom=boxXFrom->text().lower();
QString Xto=boxXTo->text().lower();
QString Yfrom=boxYFrom->text().lower();
QString Yto=boxYTo->text().lower();
QString Zfrom=boxZFrom->text().lower();
QString Zto=boxZTo->text().lower();

double fromX, toX, fromY,toY, fromZ,toZ;
try
	{
	MyParser parser;
	parser.SetExpr(Xfrom.ascii());
	fromX=parser.Eval();
	}
catch(mu::ParserError &e)
	{
	QMessageBox::critical(app, tr("MantidPlot - X Start limit error"), QString::fromStdString(e.GetMsg()));
	boxXFrom->setFocus();
	return;
	}
try
	{
	MyParser parser;
	parser.SetExpr(Xto.ascii());
	toX=parser.Eval();
	}
catch(mu::ParserError &e)
	{
	QMessageBox::critical(app, tr("MantidPlot - X End limit error"), QString::fromStdString(e.GetMsg()));
	boxXTo->setFocus();
	return;
	}

try
	{
	MyParser parser;
	parser.SetExpr(Yfrom.ascii());
	fromY=parser.Eval();
	}
catch(mu::ParserError &e)
	{
	QMessageBox::critical(app, tr("MantidPlot - Y Start limit error"), QString::fromStdString(e.GetMsg()));
	boxYFrom->setFocus();
	return;
	}
try
	{
	MyParser parser;
	parser.SetExpr(Yto.ascii());
	toY=parser.Eval();
	}
catch(mu::ParserError &e)
	{
	QMessageBox::critical(app, tr("MantidPlot - Y End limit error"), QString::fromStdString(e.GetMsg()));
	boxYTo->setFocus();
	return;
	}
try
	{
	MyParser parser;
	parser.SetExpr(Zfrom.ascii());
	fromZ=parser.Eval();
	}
catch(mu::ParserError &e)
	{
	QMessageBox::critical(app, tr("MantidPlot - Z Start limit error"), QString::fromStdString(e.GetMsg()));
	boxZFrom->setFocus();
	return;
	}
try
	{
	MyParser parser;
	parser.SetExpr(Zto.ascii());
	toZ=parser.Eval();
	}
catch(mu::ParserError &e)
	{
	QMessageBox::critical(app, tr("MantidPlot - Z End limit error"), QString::fromStdString(e.GetMsg()));
	boxZTo->setFocus();
	return;
	}

if (fromX >= toX || fromY >= toY || fromZ >= toZ)
	{
	QMessageBox::critical(app, tr("MantidPlot - Input error"),
				tr("Please enter limits that satisfy: from < end!"));
	boxXTo->setFocus();
	return;
	}

double x,y;
QString formula=boxFunction->currentText();
bool error=false;
try
	{
	MyParser parser;
	parser.DefineVar("x", &x);
	parser.DefineVar("y", &y);
	parser.SetExpr(formula.ascii());

	x=fromX; y=fromY;
	parser.Eval();
	x=toX; y=toY;
	parser.Eval();
	}
catch(mu::ParserError &e)
	{
	QMessageBox::critical(0, tr("MantidPlot - Input function error"), QString::fromStdString(e.GetMsg()));
	boxFunction->setFocus();
	error=true;
	}

if (!error){
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	if (!d_graph){
		app->plotSurface(boxFunction->currentText(),fromX, toX, fromY, toY, fromZ, toZ,
					 boxFuncColumns->value(), boxFuncRows->value());
	} else
		d_graph->addFunction(boxFunction->currentText(),fromX, toX, fromY, toY, fromZ, toZ,
					 boxFuncColumns->value(), boxFuncRows->value());

	app->updateSurfaceFuncList(boxFunction->currentText());
    QApplication::restoreOverrideCursor();
	close();
	}
}

void SurfaceDialog::setParametricSurface(Graph3D *g)
{
	if (!g)
		return;

	d_graph = g;
	UserParametricSurface *s = d_graph->parametricSurface();

	boxType->setCurrentIndex(1);
	optionStack->setCurrentIndex(1);

	boxX->setText(s->xFormula());
	boxY->setText(s->yFormula());
	boxZ->setText(s->zFormula());

	boxUFrom->setText(QString::number(s->uStart()));
	boxUTo->setText(QString::number(s->uEnd()));
	boxVFrom->setText(QString::number(s->vStart()));
	boxVTo->setText(QString::number(s->vEnd()));

	boxColumns->setValue(s->columns());
	boxRows->setValue(s->rows());

	boxUPeriodic->setChecked(s->uPeriodic());
	boxVPeriodic->setChecked(s->vPeriodic());
}
