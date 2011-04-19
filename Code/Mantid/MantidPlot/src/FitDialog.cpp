/***************************************************************************
    File                 : FitDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2004-2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Fit Wizard

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
#include "FitDialog.h"
#include "pixmaps.h"
#include "MyParser.h"
#include "ApplicationWindow.h"
#include "ColorBox.h"
#include "Fit.h"
#include "MultiPeakFit.h"
#include "ExponentialFit.h"
#include "PolynomialFit.h"
#include "PluginFit.h"
#include "NonLinearFit.h"
#include "SigmoidalFit.h"
#include "LogisticFit.h"
#include "Matrix.h"
#include "DoubleSpinBox.h"

#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QWidget>
#include <QMessageBox>
#include <QComboBox>
#include <QWidgetList>
#include <QRadioButton>
#include <QFileDialog>
#include <QGroupBox>
#include <QLibrary>
#include <QLocale>
#include <stdio.h>

#include <qwt_plot_curve.h>

FitDialog::FitDialog(Graph *g, QWidget* parent, Qt::WFlags fl )
: QDialog( parent, fl )
{
    setObjectName("FitDialog");
	setWindowTitle(tr("MantidPlot - Fit Wizard"));
	setSizeGripEnabled(true);

	d_param_table = 0;
	d_current_fit = 0;
	d_preview_curve = 0;

	tw = new QStackedWidget();

	initEditPage();
	initFitPage();
	initAdvancedPage();

	QVBoxLayout* vl = new QVBoxLayout();
	vl->addWidget(tw);
    setLayout(vl);

	categoryBox->setCurrentRow (2);
	funcBox->setCurrentRow (0);

	setGraph(g);
	initBuiltInFunctions();
    loadPlugins();
    loadUserFunctions();
}

void FitDialog::initFitPage()
{
    ApplicationWindow *app = (ApplicationWindow *)parent();

    QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel(tr("Curve")), 0, 0);
	boxCurve = new QComboBox();
    gl1->addWidget(boxCurve, 0, 1);
    gl1->addWidget(new QLabel(tr("Function")), 1, 0);
	lblFunction = new QLabel();
    gl1->addWidget(lblFunction, 1, 1);
	boxFunction = new QTextEdit();
	boxFunction->setReadOnly(true);
    QPalette palette = boxFunction->palette();
    palette.setColor(QPalette::Active, QPalette::Base, Qt::lightGray);
    boxFunction->setPalette(palette);
	boxFunction->setMaximumHeight(50);
    gl1->addWidget(boxFunction, 2, 1);

    QVBoxLayout *vb = new QVBoxLayout();
    vb->addWidget(new QLabel(tr("Initial guesses")));
    btnSaveGuesses = new QPushButton(tr( "&Save" ));
    connect(btnSaveGuesses, SIGNAL(clicked()), this, SLOT(saveInitialGuesses()));
    vb->addWidget(btnSaveGuesses);
    btnParamRange = new QPushButton();
    btnParamRange->setIcon(QIcon(getQPixmap("param_range_btn_xpm")));
    btnParamRange->setCheckable(true);
    connect(btnParamRange, SIGNAL(toggled(bool)), this, SLOT(showParameterRange(bool)));
    vb->addWidget(btnParamRange);
    previewBox = new QCheckBox(tr("&Preview"));
    connect(previewBox, SIGNAL(stateChanged(int)), this, SLOT(updatePreview()));
    vb->addWidget(previewBox);
    vb->addStretch();
	gl1->addLayout(vb, 3, 0);

	boxParams = new QTableWidget();
    boxParams->setColumnCount(5);
    boxParams->horizontalHeader()->setClickable(false);
    boxParams->horizontalHeader()->setResizeMode (0, QHeaderView::ResizeToContents);
	boxParams->horizontalHeader()->setResizeMode (1, QHeaderView::Stretch);
    boxParams->horizontalHeader()->setResizeMode (2, QHeaderView::Stretch);
    boxParams->horizontalHeader()->setResizeMode (3, QHeaderView::Stretch);
	boxParams->horizontalHeader()->setResizeMode (4, QHeaderView::ResizeToContents);
    QStringList header = QStringList() << tr("Parameter") << tr("From") << tr("Value") << tr("To") << tr("Constant");
    boxParams->setHorizontalHeaderLabels(header);
    boxParams->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    boxParams->verticalHeader()->hide();
    boxParams->hideColumn(1);
    boxParams->hideColumn(3);
    gl1->addWidget(boxParams, 3, 1);

	gl1->addWidget(new QLabel( tr("Algorithm")), 4, 0 );
	boxAlgorithm = new QComboBox();
	boxAlgorithm->addItem(tr("Scaled Levenberg-Marquardt"));
	boxAlgorithm->addItem(tr("Unscaled Levenberg-Marquardt"));
	boxAlgorithm->addItem(tr("Nelder-Mead Simplex"));
    gl1->addWidget(boxAlgorithm, 4, 1);

	gl1->addWidget(new QLabel( tr("Color")), 5, 0);
	boxColor = new ColorBox();
	boxColor->setColor(QColor(Qt::red));
    gl1->addWidget(boxColor, 5, 1);

    QGroupBox *gb1 = new QGroupBox();
    gb1->setLayout(gl1);

    QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel(tr("From x=")), 0, 0);

	boxFrom = new DoubleSpinBox();
//    boxFrom->setLocale(app->locale());
    boxFrom->setDecimals(app->d_decimal_digits);
    connect(boxFrom, SIGNAL(valueChanged(double)), this, SLOT(updatePreview()));
    gl2->addWidget(boxFrom, 0, 1);

	gl2->addWidget(new QLabel( tr("To x=")), 1, 0);

	boxTo = new DoubleSpinBox();
//    boxTo->setLocale(app->locale());
    boxTo->setDecimals(app->d_decimal_digits);
    connect(boxTo, SIGNAL(valueChanged(double)), this, SLOT(updatePreview()));
    gl2->addWidget(boxTo, 1, 1);

    QGroupBox *gb2 = new QGroupBox();
    gb2->setLayout(gl2);

    QGridLayout *gl3 = new QGridLayout();
    gl3->addWidget(new QLabel(tr("Iterations")), 0, 0);
	boxPoints = new QSpinBox();
    boxPoints->setRange(10, 10000);
	boxPoints->setSingleStep(50);
	boxPoints->setValue(1000);
    gl3->addWidget(boxPoints, 0, 1);
	gl3->addWidget(new QLabel( tr("Tolerance")), 1, 0);

	boxTolerance = new DoubleSpinBox();
	boxTolerance->setRange(0.0, 1.0);
	boxTolerance->setSingleStep(1e-4);
//    boxTolerance->setLocale(app->locale());
    boxTolerance->setDecimals(13);
    boxTolerance->setValue(1e-4);
	gl3->addWidget(boxTolerance, 1, 1);

    QGroupBox *gb3 = new QGroupBox();
    gb3->setLayout(gl3);

    QHBoxLayout *hbox1 = new QHBoxLayout();
    hbox1->addWidget(gb2);
    hbox1->addWidget(gb3);

    QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->addWidget(new QLabel(tr( "Weighting Method" )));
	boxWeighting = new QComboBox();
	boxWeighting->addItem(tr("No weighting"));
	boxWeighting->addItem(tr("Instrumental"));
	boxWeighting->addItem(tr("Statistical"));
	boxWeighting->addItem(tr("Arbitrary Dataset"));
    hbox2->addWidget(boxWeighting);
    QGroupBox *gb4 = new QGroupBox();
    gb4->setLayout(hbox2);

	tableNamesBox = new QComboBox();
	tableNamesBox->setEnabled(false);
    hbox2->addWidget(tableNamesBox);
	colNamesBox = new QComboBox();
	colNamesBox->setEnabled(false);
    hbox2->addWidget(colNamesBox);

    QHBoxLayout *hbox3 = new QHBoxLayout();
	buttonEdit = new QPushButton(tr( "<< &Edit function" ) );
    hbox3->addWidget(buttonEdit);
	btnDeleteFitCurves = new QPushButton(tr( "&Delete Fit Curves" ));
    hbox3->addWidget(btnDeleteFitCurves);
	buttonOk = new QPushButton(tr( "&Fit" ) );
	buttonOk->setDefault( true );
    hbox3->addWidget(buttonOk);
	buttonCancel1 = new QPushButton(tr( "&Close" ));
    hbox3->addWidget(buttonCancel1);
	buttonAdvanced = new QPushButton(tr( "Custom &Output >>" ));
    hbox3->addWidget(buttonAdvanced);
    hbox3->addStretch();

    QVBoxLayout *vbox1 = new QVBoxLayout();
    vbox1->addWidget(gb1);
    vbox1->addLayout(hbox1);
    vbox1->addWidget(gb4);
    vbox1->addLayout(hbox3);

    fitPage = new QWidget();
    fitPage->setLayout(vbox1);
    tw->addWidget(fitPage);

	connect( boxCurve, SIGNAL( activated(const QString&) ), this, SLOT( activateCurve(const QString&) ) );
	connect( buttonOk, SIGNAL( clicked() ), this, SLOT(accept()));
	connect( buttonCancel1, SIGNAL( clicked() ), this, SLOT(close()));
	connect( buttonEdit, SIGNAL( clicked() ), this, SLOT(showEditPage()));
	connect( btnDeleteFitCurves, SIGNAL( clicked() ), this, SLOT(deleteFitCurves()));
	connect( boxWeighting, SIGNAL( activated(int) ), this, SLOT( enableWeightingParameters(int) ) );
	connect( buttonAdvanced, SIGNAL(clicked()), this, SLOT(showAdvancedPage() ) );
    connect( tableNamesBox, SIGNAL( activated(int) ), this, SLOT( selectSrcTable(int) ) );

	setFocusProxy(boxFunction);
}

void FitDialog::initEditPage()
{
    QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel(tr("Category")), 0, 0);
    gl1->addWidget(new QLabel(tr("Function")), 0, 1);
    gl1->addWidget(new QLabel(tr("Expression")), 0, 2);

	categoryBox = new QListWidget();
	categoryBox->addItem(tr("User defined"));
	categoryBox->addItem(tr("Built-in"));
	categoryBox->addItem(tr("Basic"));
	categoryBox->addItem(tr("Plugins"));

    gl1->addWidget(categoryBox, 1, 0);
	funcBox = new QListWidget();
    gl1->addWidget(funcBox, 1, 1);
	explainBox = new QTextEdit();
	explainBox->setReadOnly(true);
    gl1->addWidget(explainBox, 1, 2);

	boxUseBuiltIn = new QCheckBox(tr("Fit with &built-in function"));
	boxUseBuiltIn->hide();

    QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->addWidget(boxUseBuiltIn);
    hbox1->addStretch();

	polynomOrderLabel = new QLabel( tr("Polynomial Order"));
	polynomOrderLabel->hide();
    hbox1->addWidget(polynomOrderLabel);

	polynomOrderBox = new QSpinBox();
    polynomOrderBox->setMinValue(1);
	polynomOrderBox->setValue(1);
	polynomOrderBox->hide();
	connect(polynomOrderBox, SIGNAL(valueChanged(int)), this, SLOT(setNumPeaks(int)));
    hbox1->addWidget(polynomOrderBox);

	buttonPlugins = new QPushButton(tr( "Choose plug&ins folder..." ) );
    hbox1->addWidget(buttonPlugins);
	buttonPlugins->hide();

    QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel(tr("Name")), 0, 0);
	boxName = new QLineEdit(tr("user1"));
    gl2->addWidget(boxName, 0, 1);
	btnAddFunc = new QPushButton(tr( "&Save" ));
    gl2->addWidget(btnAddFunc, 0, 2);
    gl2->addWidget(new QLabel(tr("Parameters")), 1, 0);
	boxParam = new QLineEdit("a, b");
    gl2->addWidget(boxParam, 1, 1);
	btnDelFunc = new QPushButton( tr( "&Remove" ));
    gl2->addWidget(btnDelFunc, 1, 2);

    QGroupBox *gb = new QGroupBox();
    gb->setLayout(gl2);

	editBox = new QTextEdit();
	editBox->setTextFormat(Qt::PlainText);
	editBox->setFocus();

    QVBoxLayout *vbox1 = new QVBoxLayout();
	btnAddTxt = new QPushButton(tr( "Add &expression" ) );
    vbox1->addWidget(btnAddTxt);
	btnAddName = new QPushButton(tr( "Add &name" ));
    vbox1->addWidget(btnAddName);
	buttonClear = new QPushButton(tr( "Rese&t" ));
    vbox1->addWidget(buttonClear);
	buttonCancel2 = new QPushButton(tr( "&Close" ));
    vbox1->addWidget(buttonCancel2);
    btnContinue = new QPushButton(tr( "&Fit >>" ));
    vbox1->addWidget(btnContinue);
    vbox1->addStretch();

    QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->addWidget(editBox);
    hbox2->addLayout(vbox1);

    QVBoxLayout *vbox2 = new QVBoxLayout();
    vbox2->addLayout(gl1);
    vbox2->addLayout(hbox1);
    vbox2->addWidget(gb);
    vbox2->addLayout(hbox2);

    editPage = new QWidget();
    editPage->setLayout(vbox2);
    tw->addWidget(editPage);

	connect( buttonPlugins, SIGNAL(clicked()), this, SLOT(chooseFolder()));
    connect( buttonClear, SIGNAL(clicked()), this, SLOT(resetFunction()));
	connect( categoryBox, SIGNAL(currentRowChanged (int)), this, SLOT(showFunctionsList(int)));
	connect( funcBox, SIGNAL(currentRowChanged(int)), this, SLOT(showExpression(int)));
	connect( boxUseBuiltIn, SIGNAL(toggled(bool)), this, SLOT(setFunction(bool)));
	connect( btnAddName, SIGNAL(clicked()), this, SLOT(addFunctionName()));
	connect( btnAddTxt, SIGNAL(clicked()), this, SLOT(addFunction()));
	connect( btnContinue, SIGNAL(clicked()), this, SLOT(showFitPage()));
	connect( btnAddFunc, SIGNAL(clicked()), this, SLOT(saveUserFunction()));
	connect( btnDelFunc, SIGNAL(clicked()), this, SLOT(removeUserFunction()));
	connect( buttonCancel2, SIGNAL(clicked()), this, SLOT(close()));
}


void FitDialog::initAdvancedPage()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();

	generatePointsBtn = new QRadioButton (tr("&Uniform X Function"));
	generatePointsBtn->setChecked(app->generateUniformFitPoints);
	connect( generatePointsBtn, SIGNAL(clicked()), this, SLOT(enableApplyChanges()));

    QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(generatePointsBtn, 0, 0);

	lblPoints = new QLabel( tr("Points"));

	generatePointsBox = new QSpinBox ();
    generatePointsBox->setRange(0, 1000000);
	generatePointsBox->setSingleStep(10);
	generatePointsBox->setValue(app->fitPoints);
	connect( generatePointsBox, SIGNAL(valueChanged(int)), this, SLOT(enableApplyChanges(int)));
    showPointsBox(!app->generateUniformFitPoints);

    QHBoxLayout *hb = new QHBoxLayout();
    hb->addStretch();
    hb->addWidget(lblPoints);
    hb->addWidget(generatePointsBox);
	gl1->addLayout(hb, 0, 1);

	samePointsBtn = new QRadioButton(tr( "Same X as Fitting &Data" ));
    gl1->addWidget(samePointsBtn, 1, 0);
	samePointsBtn->setChecked(!app->generateUniformFitPoints);
	connect( samePointsBtn, SIGNAL(clicked()), this, SLOT(enableApplyChanges()));

    QGroupBox *gb1 = new QGroupBox(tr("Generated Fit Curve"));
    gb1->setLayout(gl1);

    QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel( tr("Significant Digits")), 0, 1);
	boxPrecision = new QSpinBox ();
    boxPrecision->setRange(0, 13);
	boxPrecision->setValue (app->fit_output_precision);
	connect( boxPrecision, SIGNAL(valueChanged (int)), this, SLOT(enableApplyChanges(int)));

    gl2->addWidget(boxPrecision, 0, 2);
	btnParamTable = new QPushButton(tr( "Parameters &Table" ));
    gl2->addWidget(btnParamTable, 1, 0);
	gl2->addWidget(new QLabel( tr("Name: ")), 1, 1);
	paramTableName = new QLineEdit(tr( "Parameters" ));
    gl2->addWidget(paramTableName, 1, 2);
	globalParamTableBox = new QCheckBox (tr("&One table for all fits"));
	gl2->addWidget(globalParamTableBox, 1, 3);
	btnCovMatrix = new QPushButton(tr( "Covariance &Matrix" ));
    gl2->addWidget(btnCovMatrix, 2, 0);
    gl2->addWidget(new QLabel( tr("Name: ")), 2, 1);
	covMatrixName = new QLineEdit( tr( "CovMatrix" ) );
    gl2->addWidget(covMatrixName, 2, 2);

	scaleErrorsBox = new QCheckBox(tr("Scale Errors with sqrt(Chi^2/doF)"));
	scaleErrorsBox->setChecked(app->fit_scale_errors);
	connect( scaleErrorsBox, SIGNAL(stateChanged (int)), this, SLOT(enableApplyChanges(int)));

    QGroupBox *gb2 = new QGroupBox(tr("Parameters Output"));
    gb2->setLayout(gl2);

	logBox = new QCheckBox (tr("&Write Parameters to Result Log"));
	logBox->setChecked(app->writeFitResultsToLog);
	connect( logBox, SIGNAL(stateChanged(int)), this, SLOT(enableApplyChanges(int)));

	plotLabelBox = new QCheckBox (tr("&Paste Parameters to Plot"));
	plotLabelBox->setChecked(app->pasteFitResultsToPlot);
	connect( plotLabelBox, SIGNAL(stateChanged (int)), this, SLOT(enableApplyChanges(int)));

    QHBoxLayout *hbox1 = new QHBoxLayout();

	btnBack = new QPushButton(tr( "<< &Fit" ));
	connect( btnBack, SIGNAL(clicked()), this, SLOT(returnToFitPage()));
    hbox1->addWidget(btnBack);

	btnApply = new QPushButton(tr( "&Apply" ));
	btnApply->setEnabled(false);
	connect( btnApply, SIGNAL(clicked()), this, SLOT(applyChanges()));
    hbox1->addWidget(btnApply);

	buttonCancel3 = new QPushButton(tr( "&Close" ));
    hbox1->addWidget(buttonCancel3);
    hbox1->addStretch();

    QVBoxLayout *vbox1 = new QVBoxLayout();
    vbox1->addWidget(gb1);
    vbox1->addWidget(gb2);
	vbox1->addWidget(scaleErrorsBox);
    vbox1->addWidget(logBox);
    vbox1->addWidget(plotLabelBox);
    vbox1->addStretch();
    vbox1->addLayout(hbox1);

    advancedPage = new QWidget();
	advancedPage->setLayout(vbox1);
    tw->addWidget(advancedPage);

	connect(btnParamTable, SIGNAL(clicked()), this, SLOT(showParametersTable()));
	connect(btnCovMatrix, SIGNAL(clicked()), this, SLOT(showCovarianceMatrix()));
	connect(samePointsBtn, SIGNAL(toggled(bool)), this, SLOT(showPointsBox(bool)));
	connect(generatePointsBtn, SIGNAL(toggled(bool)), this, SLOT(showPointsBox(bool)));
	connect(buttonCancel3, SIGNAL(clicked()), this, SLOT(close()));
}

void FitDialog::applyChanges()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	int prec = boxPrecision->value();
	app->fit_output_precision = prec;
	if (d_current_fit)
		d_current_fit->setOutputPrecision(prec);
	for (int i=0; i<boxParams->rowCount(); i++){
		((DoubleSpinBox*)boxParams->cellWidget(i, 2))->setDecimals(prec);
		if (d_current_fit->type() != Fit::BuiltIn){
			((RangeLimitBox*)boxParams->cellWidget(i, 1))->setDecimals(prec);
			((RangeLimitBox*)boxParams->cellWidget(i, 3))->setDecimals(prec);
		}
	}

	app->pasteFitResultsToPlot = plotLabelBox->isChecked();
	app->writeFitResultsToLog = logBox->isChecked();
	app->fitPoints = generatePointsBox->value();
	app->generateUniformFitPoints = generatePointsBtn->isChecked();
	if (d_current_fit && !d_current_fit->isA("PolynomialFit") && 
		!d_current_fit->isA("LinearFit") && !d_current_fit->isA("LinearSlopeFit"))
		app->fit_scale_errors = scaleErrorsBox->isChecked();
	app->saveSettings();
	btnApply->setEnabled(false);
}

void FitDialog::showParametersTable()
{
	QString tableName = paramTableName->text();
	if (tableName.isEmpty()){
		QMessageBox::critical(this, tr("MantidPlot - Error"),
				tr("Please enter a valid name for the parameters table."));
		return;
	}

	if (!d_current_fit){
		QMessageBox::critical(this, tr("MantidPlot - Error"),
				tr("Please perform a fit first and try again."));
		return;
	}

	d_param_table = d_current_fit->parametersTable(tableName);
}

void FitDialog::showCovarianceMatrix()
{
	QString matrixName = covMatrixName->text();
	if (matrixName.isEmpty()){
		QMessageBox::critical(this, tr("MantidPlot - Error"),
				tr("Please enter a valid name for the covariance matrix."));
		return;
	}

	if (!d_current_fit){
		QMessageBox::critical(this, tr("MantidPlot - Error"),
				tr("Please perform a fit first and try again."));
		return;
	}

	d_current_fit->covarianceMatrix(matrixName);
}

void FitDialog::showPointsBox(bool)
{
	if (generatePointsBtn->isChecked()){
		lblPoints->show();
		generatePointsBox->show();
	} else {
		lblPoints->hide();
		generatePointsBox->hide();
	}
}

void FitDialog::setGraph(Graph *g)
{
	if (!g)
		return;

	d_graph = g;
	boxCurve->clear();
	boxCurve->addItems(d_graph->analysableCurvesList());

    QString selectedCurve = g->selectedCurveTitle();
	if (!selectedCurve.isEmpty()){
	    int index = boxCurve->findText (selectedCurve);
		boxCurve->setCurrentItem(index);
	}
    activateCurve(boxCurve->currentText());

	connect (d_graph, SIGNAL(closedGraph()), this, SLOT(close()));
	connect (d_graph, SIGNAL(dataRangeChanged()), this, SLOT(changeDataRange()));
};

void FitDialog::activateCurve(const QString& curveName)
{
	QwtPlotCurve *c = d_graph->curve(curveName);
	if (!c)
		return;

	double start, end;
    d_graph->range(d_graph->curveIndex(curveName), &start, &end);
    boxFrom->setValue(QMIN(start, end));
    boxTo->setValue(QMAX(start, end));
	//Set the same color as the data curve chosen for fit (Feature Request #4031)
	boxColor->setColor(c->pen().color());
};

void FitDialog::saveUserFunction()
{
	if (editBox->text().isEmpty()){
		QMessageBox::critical(this, tr("MantidPlot - Input function error"), tr("Please enter a valid function!"));
		editBox->setFocus();
		return;
	} else if (boxName->text().isEmpty()) {
		QMessageBox::critical(this, tr("MantidPlot - Input function error"),
				tr("Please enter a function name!"));
		boxName->setFocus();
		return;
	} else if (boxParam->text().remove(QRegExp("[,;\\s]")).isEmpty()){
		QMessageBox::critical(this, tr("MantidPlot - Input function error"),
				tr("Please enter at least one parameter name!"));
		boxParam->setFocus();
		return;
	}

	if (builtInFunctionNames().contains(boxName->text())){
		QMessageBox::critical(this, tr("MantidPlot - Error: function name"),
				"<p><b>" + boxName->text() + "</b>" + tr(" is a built-in function name"
					"<p>You must choose another name for your function!"));
		editBox->setFocus();
		return;
	}

	if (editBox->text().contains(boxName->text())){
		QMessageBox::critical(this, tr("MantidPlot - Input function error"),
				tr("You can't define functions recursively!"));
		editBox->setFocus();
		return;
	}

	QString name = boxName->text();
    QStringList lst = userFunctionNames();
	QString formula = parseFormula(editBox->text().simplified());
	if (lst.contains(name)){
		int index = lst.findIndex(name);
		d_current_fit = (NonLinearFit *)d_user_functions[index];
		d_current_fit->setParametersList(boxParam->text().split(QRegExp("[,;]+[\\s]*"), QString::SkipEmptyParts));
        d_current_fit->setFormula(formula);
        d_current_fit->save(d_current_fit->fileName());

		if (funcBox->currentItem()->text() == name)
			showExpression(index);
	} else {
	    ApplicationWindow *app = (ApplicationWindow *)this->parent();
		QString filter = tr("MantidPlot fit model")+" (*.fit);;";
		filter += tr("All files")+" (*.*)";
		QString fn = QFileDialog::getSaveFileName(app, tr("MantidPlot") + " - " + tr("Save Fit Model As"),
                                app->fitModelsPath + "/" + name, filter);
		if (!fn.isEmpty()){
            QFileInfo fi(fn);
            app->fitModelsPath = fi.dir().absolutePath();
            QString baseName = fi.fileName();
            if (!baseName.contains("."))
                fn.append(".fit");

            d_current_fit = new NonLinearFit(app, d_graph);
            d_current_fit->setObjectName(name);
            d_current_fit->setParametersList(boxParam->text().split(QRegExp("[,;]+[\\s]*"), QString::SkipEmptyParts));
            d_current_fit->setFormula(formula);
            if (d_current_fit->save(fn)){
                QStringList lst = userFunctionNames();
                lst << name;
                lst.sort();
                int index = int(lst.indexOf(name));
                d_user_functions.insert (index, d_current_fit);

                if (categoryBox->currentRow() == 0){
                    funcBox->clear();
                    funcBox->addItems(lst);
                    funcBox->setCurrentRow(index);
                }

                if (d_user_functions.count()>0 && !boxUseBuiltIn->isEnabled() && categoryBox->currentRow() == 0)
                    boxUseBuiltIn->setEnabled(true);
            }
		}
	}
}

void FitDialog::removeUserFunction()
{
    QStringList lst = userFunctionNames();
    if (lst.isEmpty())
        return;

    QString s = tr("Are you sure you want to remove fit model file:\n %1 ?").arg(d_current_fit->fileName());
    if (QMessageBox::Yes != QMessageBox::question (this, tr("MantidPlot") + " - " + tr("Remove Fit Model"), s, QMessageBox::Yes, QMessageBox::Cancel))
        return;

	QString name = funcBox->currentItem()->text();
	if (lst.contains(name)){
		explainBox->setText(QString());

		int index = lst.indexOf(name);
		if (0 <= index && index < d_user_functions.size()){
            QFile f((d_user_functions[index])->fileName());
            f.remove();
            d_user_functions.removeAt(index);
		}

        lst.remove(name);
		funcBox->clear();
		funcBox->addItems(lst);
		funcBox->setCurrentRow(0);

		if (!lst.count())
			boxUseBuiltIn->setEnabled(false);
	}
}

void FitDialog::showFitPage()
{
	QString formula = editBox->text().simplified();
	if (formula.isEmpty()){
		QMessageBox::critical(this, tr("MantidPlot - Input function error"), tr("Please enter a valid function!"));
		editBox->setFocus();
		return;
	}

	ApplicationWindow *app = (ApplicationWindow *)parent();
    if (!boxUseBuiltIn->isChecked()){
        d_current_fit = new NonLinearFit(app, d_graph);
		d_current_fit->setParametersList(boxParam->text().split(QRegExp("[,;]+[\\s]*"), QString::SkipEmptyParts));
		formula = parseFormula(formula);
		d_current_fit->setFormula(formula);
    }

	if (d_current_fit->error())
		return;

    if (d_current_fit->type() == Fit::BuiltIn && 
		(d_current_fit->isA("PolynomialFit") || d_current_fit->isA("LinearFit")
		|| d_current_fit->isA("LinearSlopeFit"))){
        btnParamRange->setEnabled(false);
        boxAlgorithm->setEnabled(false);
		boxPoints->setEnabled(false);
		boxTolerance->setEnabled(false);
    } else {
        btnParamRange->setEnabled(true);
        boxAlgorithm->setEnabled(true);
		boxPoints->setEnabled(true);
		boxTolerance->setEnabled(true);
    }

	QStringList paramList = d_current_fit->parameterNames();
	int parameters = d_current_fit->numParameters();
	boxParams->clearContents();
	boxParams->setRowCount(parameters);
    boxParams->hideColumn(4);

    int aux = parameters;
	if (aux > 7)
		aux = 7;
	boxParams->setMinimumHeight(4 + (aux + 1)*boxParams->horizontalHeader()->height());

	QLocale locale = app->locale();
	int prec = boxPrecision->value();
    for (int i = 0; i<parameters; i++){
        QTableWidgetItem *it = new QTableWidgetItem(paramList[i]);
        it->setFlags(it->flags() & (~Qt::ItemIsEditable));
        it->setBackground(QBrush(Qt::lightGray));
        it->setForeground(QBrush(Qt::darkRed));
        QFont font = it->font();
        font.setBold(true);
        it->setFont(font);
        boxParams->setItem(i, 0, it);

		if (d_current_fit->type() != Fit::BuiltIn){
			RangeLimitBox *rbl = new RangeLimitBox(RangeLimitBox::LeftLimit);
//			rbl->setLocale(locale);
			rbl->setDecimals(prec);
			boxParams->setCellWidget(i, 1, rbl);
			
			RangeLimitBox *rbr = new RangeLimitBox(RangeLimitBox::RightLimit);
//			rbr->setLocale(locale);
			rbr->setDecimals(prec);
			boxParams->setCellWidget(i, 3, rbr);
		}

		DoubleSpinBox *sb = new DoubleSpinBox();
//		sb->setLocale(locale);
		sb->setDecimals(prec);
		sb->setValue(d_current_fit->initialGuess(i));
        connect(sb, SIGNAL(valueChanged(double)), this, SLOT(updatePreview()));
        boxParams->setCellWidget(i, 2, sb);
	}
    for (int i = 0; i<parameters; i++)
        boxParams->item (i, 0)->setText(paramList[i]);

	if (d_current_fit->type() == Fit::User){
        boxParams->showColumn(4);
		for (int i = 0; i<boxParams->rowCount(); i++ ){
            QTableWidgetItem *it = new QTableWidgetItem();
            it->setFlags(it->flags() & (~Qt::ItemIsEditable));
            it->setBackground(QBrush(Qt::lightGray));
            boxParams->setItem(i, 4, it);

			QCheckBox *cb = new QCheckBox();
            boxParams->setCellWidget(i, 4, cb);
		}
	}

	boxFunction->setText(formula);
	lblFunction->setText(boxName->text() +" (x, " + boxParam->text().simplified() + ")");

	tw->setCurrentWidget (fitPage);
	if (previewBox->isChecked())
		updatePreview();
}

void FitDialog::showEditPage()
{
    if (d_current_fit)
        d_current_fit->freeMemory();
	tw->setCurrentWidget(editPage);
}

void FitDialog::showAdvancedPage()
{
	tw->setCurrentWidget (advancedPage);
	if (d_current_fit && (d_current_fit->isA("PolynomialFit") || 
		d_current_fit->isA("LinearFit") || d_current_fit->isA("LinearSlopeFit"))){
		scaleErrorsBox->setChecked(false);
		scaleErrorsBox->setEnabled(false);
	} else {
		ApplicationWindow *app = (ApplicationWindow *)this->parent();
		if (app)
			scaleErrorsBox->setChecked(app->fit_scale_errors);
		scaleErrorsBox->setEnabled(true);
	}
}

void FitDialog::setFunction(bool ok)
{
	editBox->setEnabled(!ok);
	boxParam->setEnabled(!ok);
	boxName->setEnabled(!ok);
	btnAddFunc->setEnabled(!ok);
	btnAddName->setEnabled(!ok);
	btnAddTxt->setEnabled(!ok);
	buttonClear->setEnabled(!ok);

	if (ok){
		boxName->setText(funcBox->currentItem()->text());
		editBox->setText(explainBox->text());
		boxParam->setText(d_current_fit->parameterNames().join(", "));
	}
}

void FitDialog::showFunctionsList(int category)
{
	boxUseBuiltIn->setChecked(false);
	boxUseBuiltIn->setEnabled(false);
	boxUseBuiltIn->hide();
	buttonPlugins->hide();
	btnDelFunc->setEnabled(false);
    funcBox->blockSignals(true);
	funcBox->clear();
    explainBox->clear();
	polynomOrderLabel->hide();
	polynomOrderBox->hide();

	switch (category)
	{
		case 0:
			if (d_user_functions.count() > 0){
				boxUseBuiltIn->setEnabled(true);

                QStringList lst;
                foreach(Fit *fit, d_user_functions)
                    lst << fit->objectName();
                funcBox->addItems(lst);
			}
            buttonPlugins->setText(tr("Choose &models folder..."));
            buttonPlugins->show();
			boxUseBuiltIn->setText(tr("Fit with selected &user function"));
			boxUseBuiltIn->show();
			btnDelFunc->setEnabled(true);
        break;

		case 1:
			boxUseBuiltIn->setText(tr("Fit using &built-in function"));
			boxUseBuiltIn->show();
			boxUseBuiltIn->setEnabled(true);
			funcBox->addItems(builtInFunctionNames());
        break;

		case 2:
			showParseFunctions();
        break;

		case 3:
            buttonPlugins->setText(tr("Choose plug&ins folder..."));
            buttonPlugins->show();
			boxUseBuiltIn->setText(tr("Fit using &plugin function"));
			boxUseBuiltIn->show();
            boxUseBuiltIn->setEnabled(d_plugins.size() > 0);
            QStringList lst;
            foreach(Fit *fit, d_plugins)
                lst << fit->objectName();
            funcBox->addItems(lst);
        break;
	}
    funcBox->blockSignals(false);
	funcBox->setCurrentRow (0);
}

void FitDialog::chooseFolder()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (categoryBox->currentRow() == 3){//plugins
        QString dir = QFileDialog::getExistingDirectory(this, tr("Choose the plugins folder"), app->fitPluginsPath);
        if (!dir.isEmpty()){
            funcBox->clear();
            explainBox->clear();
            app->fitPluginsPath = dir;
            loadPlugins();
        }
	} else if (!categoryBox->currentRow()){//user-defined
	    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose the fit models folder"), app->fitModelsPath);
        if (!dir.isEmpty()){
            funcBox->clear();
            explainBox->clear();
            app->fitModelsPath = dir;
            loadUserFunctions();

            QString path = app->fitModelsPath + "/";
            foreach(Fit *fit, d_built_in_functions)
                fit->setFileName(path + fit->objectName() + ".fit");
        }
	}
}

void FitDialog::loadPlugins()
{
    d_plugins.clear();
	typedef char* (*fitFunc)();

	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	QString path = app->fitPluginsPath + "/";
	QString modelsDirPath = app->fitModelsPath + "/";
	QDir dir(path);
	QStringList lst = dir.entryList(QDir::Files|QDir::NoSymLinks, QDir::Name);
    QStringList names;
	for (int i=0; i<lst.count(); i++){
	    QString file = lst[i];
	    if (QLibrary::isLibrary (file)){
            QLibrary lib(path + file);
            PluginFit *fit = new PluginFit(app, d_graph);
            fit->load(lib.library());
            d_plugins << fit;
            names << fit->objectName();
            fit->setFileName(modelsDirPath + fit->objectName() + ".fit");
	    }
	}

    if (d_plugins.size() > 0){
        funcBox->addItems(names);
        funcBox->setCurrentRow(0);
        boxUseBuiltIn->setEnabled(true);
    } else
        boxUseBuiltIn->setEnabled(false);
}

void FitDialog::showParseFunctions()
{
	funcBox->addItems(MyParser::functionsList());
}

void FitDialog::showExpression(int function)
{
    if (function < 0)
        return;

	if (categoryBox->currentRow() == 2)
		explainBox->setText(MyParser::explainFunction(function));
	else if (categoryBox->currentRow() == 1){
		polynomOrderLabel->hide();
		polynomOrderBox->hide();
		if (funcBox->currentItem()->text() == tr("Gauss") ||
			funcBox->currentItem()->text() == tr("Lorentz")){
			polynomOrderLabel->setText(tr("Peaks"));
			polynomOrderLabel->show();
			polynomOrderBox->show();
		} else if (funcBox->currentItem()->text() == tr("Polynomial")){
			polynomOrderLabel->setText(tr("Polynomial Order"));
			polynomOrderLabel->show();
			polynomOrderBox->show();
		}

		d_current_fit = d_built_in_functions[function];
		explainBox->setText(d_current_fit->formula());
		setFunction(boxUseBuiltIn->isChecked());
	} else if (categoryBox->currentRow() == 0){
		if (d_user_functions.size() > function){
			d_current_fit = d_user_functions[function];
			explainBox->setText(d_current_fit->formula());
		} else
			explainBox->clear();
		setFunction(boxUseBuiltIn->isChecked());
	} else if (categoryBox->currentRow() == 3){
		if (d_plugins.size() > 0){
		    d_current_fit = d_plugins[function];
			explainBox->setText(d_current_fit->formula());
			setFunction(boxUseBuiltIn->isChecked());
		} else
			explainBox->clear();
	}
}

void FitDialog::addFunction()
{
	QString f = explainBox->text();
	if (categoryBox->currentRow() == 2){//basic parser function
		f = f.left(f.find("(", 0)+1);
		if (editBox->hasSelectedText()){
			QString markedText=editBox->selectedText();
			editBox->insert(f+markedText+")");
		} else
			editBox->insert(f+")");
	}else
		editBox->insert(f);

	editBox->setFocus();
}

void FitDialog::addFunctionName()
{
	if (funcBox->count() > 0){
		editBox->insert(funcBox->currentItem()->text());
		editBox->setFocus();
	}
}

void FitDialog::accept()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();

	QString curve = boxCurve->currentText();
	QStringList curvesList = d_graph->curvesList();
	if (curvesList.contains(curve) <= 0){
		QMessageBox::critical(app, tr("MantidPlot - Warning"),
				tr("The curve <b> %1 </b> doesn't exist anymore! Operation aborted!").arg(curve));
		boxCurve->clear();
		boxCurve->addItems(curvesList);
		return;
	}

	double start = boxFrom->value();
	double end = boxTo->value();
	double eps = boxTolerance->value();

	if (start >= end){
		QMessageBox::critical(app, tr("MantidPlot - Input error"),
				tr("Please enter x limits that satisfy: from < end!"));
		boxTo->setFocus();
		return;
	}

	int n = 0, rows = boxParams->rowCount();
	if (!boxParams->isColumnHidden(4)){
		for (int i=0; i<rows; i++){//count the non-constant parameters
            QCheckBox *cb = (QCheckBox*)boxParams->cellWidget(i, 4);
			if (!cb->isChecked())
				n++;
		}
	} else
		n = rows;

	QStringList parameters = QStringList();
	MyParser parser;
	bool error = false;
	QVarLengthArray<double> paramsInit(n);
	QString formula = boxFunction->text();
	try {
		if (!boxParams->isColumnHidden(4)){
			int j = 0;
			for (int i=0; i<rows; i++){
                QCheckBox *cb = (QCheckBox*)boxParams->cellWidget(i, 4);
				if (!cb->isChecked()){
					paramsInit[j] = ((DoubleSpinBox*)boxParams->cellWidget(i, 2))->value();
					parser.DefineVar(boxParams->item(i, 0)->text().ascii(), &paramsInit[j]);
					parameters << boxParams->item(i, 0)->text();

					if (d_current_fit->type() != Fit::BuiltIn){
						double left = ((RangeLimitBox*)boxParams->cellWidget(j, 1))->value();
						double right = ((RangeLimitBox*)boxParams->cellWidget(j, 3))->value();
						d_current_fit->setParameterRange(j, left, right);
					}
					j++;
				} else {
					double val = ((DoubleSpinBox*)boxParams->cellWidget(i, 2))->value();
					formula.replace(boxParams->item(i, 0)->text(), QString::number(val, 'e', app->fit_output_precision));
				}
			}
		} else {
			for (int i=0; i<n; i++) {
				paramsInit[i] = ((DoubleSpinBox*)boxParams->cellWidget(i, 2))->value();
				parser.DefineVar(boxParams->item(i, 0)->text().ascii(), &paramsInit[i]);
				parameters << boxParams->item(i, 0)->text();

				if (d_current_fit->type() != Fit::BuiltIn){
					double left = ((RangeLimitBox*)boxParams->cellWidget(i, 1))->value();
					double right = ((RangeLimitBox*)boxParams->cellWidget(i, 3))->value();
					d_current_fit->setParameterRange(i, left, right);
				}
			}
		}

		parser.SetExpr(formula.ascii());
		double x = start;
		parser.DefineVar("x", &x);
		parser.Eval();
	} catch(mu::ParserError &e) {
		QString errorMsg = boxFunction->text() + " = " + formula + "\n" + QString::fromStdString(e.GetMsg()) + "\n" +
			tr("Please verify that you have initialized all the parameters!");

		QMessageBox::critical(app, tr("MantidPlot - Input function error"), errorMsg);
		boxFunction->setFocus();
		error = true;
	}

	if (!error){
		if (d_current_fit->type() == Fit::BuiltIn)
			modifyGuesses (paramsInit.data());
		if (d_current_fit->type() == Fit::User){
			d_current_fit->setParametersList(parameters);
			d_current_fit->setFormula(formula);
		}

		d_current_fit->setInitialGuesses(paramsInit.data());

		if (!d_current_fit->setDataFromCurve(curve, start, end) ||
			!d_current_fit->setWeightingData ((Fit::WeightingMethod)boxWeighting->currentIndex(),
					       tableNamesBox->currentText()+"_"+colNamesBox->currentText())) return;

		d_current_fit->setTolerance(eps);
		d_current_fit->setOutputPrecision(app->fit_output_precision);
		d_current_fit->setAlgorithm((Fit::Algorithm)boxAlgorithm->currentIndex());
		d_current_fit->setColor(boxColor->currentIndex());
		d_current_fit->generateFunction(generatePointsBtn->isChecked(), generatePointsBox->value());
		d_current_fit->setMaximumIterations(boxPoints->value());
		if (!d_current_fit->isA("PolynomialFit") && !d_current_fit->isA("LinearFit") && !d_current_fit->isA("LinearSlopeFit"))
			d_current_fit->scaleErrors(scaleErrorsBox->isChecked());
		d_current_fit->fit();
		double *res = d_current_fit->results();
		if (!boxParams->isColumnHidden(4)){
			int j = 0;
			for (int i=0; i<rows; i++){
                QCheckBox *cb = (QCheckBox*)boxParams->cellWidget(i, 4);
				if (!cb->isChecked())
					((DoubleSpinBox*)boxParams->cellWidget(i, 2))->setValue(res[j++]);
			}
		} else {
			for (int i=0; i<rows; i++)
				((DoubleSpinBox*)boxParams->cellWidget(i, 2))->setValue(res[i]);
		}

		if (globalParamTableBox->isChecked() && d_param_table)
			d_current_fit->writeParametersToTable(d_param_table, true);
	}
}

void FitDialog::modifyGuesses(double* initVal)
{
	if (!d_current_fit)
		return;

	QString fitName = d_current_fit->objectName();
	if (fitName == tr("ExpDecay1"))
		initVal[1] = 1/initVal[1];
	else if (fitName == tr("ExpGrowth"))
		initVal[1] = -1/initVal[1];
	else if (fitName == tr("ExpDecay2")){
		initVal[1] = 1/initVal[1];
		initVal[3] = 1/initVal[3];
	} else if (fitName == tr("ExpDecay3")){
		initVal[1] = 1/initVal[1];
		initVal[3] = 1/initVal[3];
		initVal[5] = 1/initVal[5];
	}
}

void FitDialog::changeDataRange()
{
	double start = d_graph->selectedXStartValue();
	double end = d_graph->selectedXEndValue();
	boxFrom->setValue(QMIN(start, end));
	boxTo->setValue(QMAX(start, end));
}

void FitDialog::setSrcTables(QList<MdiSubWindow*> tables)
{
	if (tables.isEmpty()){
		tableNamesBox->addItem(tr("No data tables"));
		colNamesBox->addItem(tr("No data tables"));
		return;
	}

	srcTables = tables;
	tableNamesBox->clear();
	foreach(MdiSubWindow *w, srcTables)
		tableNamesBox->addItem(w->objectName());

	tableNamesBox->setCurrentIndex(tableNamesBox->findText(boxCurve->currentText().split("_", QString::SkipEmptyParts)[0]));
	selectSrcTable(tableNamesBox->currentIndex());
}

void FitDialog::selectSrcTable(int tabnr)
{
	colNamesBox->clear();

	if (tabnr >= 0 && tabnr < srcTables.count()){
		Table *t = (Table*)srcTables.at(tabnr);
		if (t)
			colNamesBox->addItems(t->colNames());
	}
}

void FitDialog::enableWeightingParameters(int index)
{
	if (index == Fit::Dataset){
		tableNamesBox->setEnabled(true);
		colNamesBox->setEnabled(true);
	} else {
		tableNamesBox->setEnabled(false);
		colNamesBox->setEnabled(false);
	}
}

void FitDialog::closeEvent (QCloseEvent * e)
{							
    if (d_preview_curve){
        d_preview_curve->detach();
        d_graph->replot();
        delete d_preview_curve;
    }

	if(d_current_fit && plotLabelBox->isChecked())
		d_current_fit->showLegend();

	e->accept();
}

void FitDialog::enableApplyChanges(int)
{
	btnApply->setEnabled(true);
}

void FitDialog::deleteFitCurves()
{
	d_graph->deleteFitCurves();
	boxCurve->clear();
	boxCurve->addItems(d_graph->curvesList());
}

void FitDialog::resetFunction()
{
	boxName->clear();
	boxParam->clear();
	editBox->clear();
}

void FitDialog::initBuiltInFunctions()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();

	d_built_in_functions << new SigmoidalFit(app, d_graph);
	d_built_in_functions << new ExponentialFit(app, d_graph);
	d_built_in_functions << new TwoExpFit(app, d_graph);
	d_built_in_functions << new ThreeExpFit(app, d_graph);
	d_built_in_functions << new ExponentialFit(app, d_graph, true);

	MultiPeakFit *fit = new MultiPeakFit(app, d_graph, MultiPeakFit::Gauss);
	fit->enablePeakCurves(app->generatePeakCurves);
	fit->setPeakCurvesColor(app->peakCurvesColor);
	d_built_in_functions << fit;

	d_built_in_functions << new GaussAmpFit(app, d_graph);
	d_built_in_functions << new LinearFit(app, d_graph);
	d_built_in_functions << new LinearSlopeFit(app, d_graph);
	d_built_in_functions << new LogisticFit(app, d_graph);

	fit = new MultiPeakFit(app, d_graph, MultiPeakFit::Lorentz);
	fit->enablePeakCurves(app->generatePeakCurves);
	fit->setPeakCurvesColor(app->peakCurvesColor);
	d_built_in_functions << fit;

	d_built_in_functions << new PolynomialFit(app, d_graph, 1);

    QString path = app->fitModelsPath + "/";
    foreach(Fit *fit, d_built_in_functions)
        fit->setFileName(path + fit->objectName() + ".fit");
}

void FitDialog::setNumPeaks(int peaks)
{
	if (d_current_fit->objectName() == tr("Gauss") ||
		d_current_fit->objectName() == tr("Lorentz"))
		((MultiPeakFit *)d_current_fit)->setNumPeaks(peaks);
	else if (d_current_fit->objectName() == tr("Polynomial"))
		((PolynomialFit *)d_current_fit)->setOrder(peaks);

	int index = funcBox->currentRow();
	d_built_in_functions[index] = d_current_fit;
	showExpression(index);
}

QStringList FitDialog::builtInFunctionNames()
{
	QStringList lst;
	foreach(Fit *fit, d_built_in_functions)
		lst << fit->objectName();
	return lst;
}

void FitDialog::loadUserFunctions()
{
    d_user_functions.clear();
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	QString path = app->fitModelsPath + "/";
	QDir dir(path);
	QStringList lst = dir.entryList(QDir::Files|QDir::NoSymLinks, QDir::Name);
	QStringList names;
	for (int i=0; i<lst.count(); i++){
        NonLinearFit *fit = new NonLinearFit(app, d_graph);
        if (fit->load(path + lst[i])){
            if (fit->type() == Fit::User){
                d_user_functions << fit;
                names << fit->objectName();
            } else if (fit->type() == Fit::BuiltIn){
                QStringList lst = builtInFunctionNames();
                int index = lst.indexOf(fit->objectName());
                if (index >= 0 && index < d_built_in_functions.size()){
                    Fit *f = d_built_in_functions[index];
                    f->setFileName(fit->fileName());
                    for (int i=0; i<f->numParameters(); i++)
                        f->setInitialGuess(i, fit->initialGuess(i));
                }
            } else if (fit->type() == Fit::Plugin){
                QStringList lst = plugInNames();
                int index = lst.indexOf(fit->objectName());
                if (index >= 0 && index < d_plugins.size()){
                    Fit *f = d_plugins[index];
                    f->setFileName(fit->fileName());
                    for (int i=0; i<f->numParameters(); i++)
                        f->setInitialGuess(i, fit->initialGuess(i));
                }
            }
        }
    }

    if (d_user_functions.size() > 0){
        funcBox->addItems(names);
        funcBox->setCurrentRow(0);
        boxUseBuiltIn->setEnabled(true);
    } else
        boxUseBuiltIn->setEnabled(false);
}

QStringList FitDialog::userFunctionNames()
{
	QStringList lst;
	foreach(Fit *fit, d_user_functions)
		lst << fit->objectName();
	return lst;
}

void FitDialog::saveInitialGuesses()
{
    if (!d_current_fit)
        return;

	int rows = boxParams->rowCount();
    for (int i=0; i<rows; i++)
        d_current_fit->setInitialGuess(i, ((DoubleSpinBox*)boxParams->cellWidget(i, 2))->value());

    QString fileName = d_current_fit->fileName();
    if (!fileName.isEmpty())
        d_current_fit->save(fileName);
    else {
	    ApplicationWindow *app = (ApplicationWindow *)this->parent();
		QString filter = tr("MantidPlot fit model") + " (*.fit);;";
		filter += tr("All files") + " (*.*)";
		QString fn = QFileDialog::getSaveFileName(app, tr("MantidPlot") + " - " + tr("Save Fit Model As"),
                                app->fitModelsPath + "/" + d_current_fit->objectName(), filter);
		if (!fn.isEmpty()){
            QFileInfo fi(fn);
            QString baseName = fi.fileName();
            if (!baseName.contains("."))
                fn.append(".fit");

            if (d_current_fit->save(fn))
                d_user_functions.append(d_current_fit);
          }
    }
}

QStringList FitDialog::plugInNames()
{
	QStringList lst;
	foreach(Fit *fit, d_plugins)
		lst << fit->objectName();
	return lst;
}

void FitDialog::returnToFitPage()
{
	applyChanges();
	tw->setCurrentWidget(fitPage);
}

void FitDialog::updatePreview()
{
    if (!previewBox->isChecked() && d_preview_curve){
        d_preview_curve->detach();
        d_graph->replot();
        delete d_preview_curve;
        d_preview_curve = 0;
        return;
    }

    if (!d_current_fit || !previewBox->isChecked())
        return;

    int d_points = generatePointsBox->value();
    QVarLengthArray<double> X(d_points), Y(d_points);//double X[d_points], Y[d_points];
    int p = boxParams->rowCount();
    QVarLengthArray<double> parameters(p);//double parameters[p];
    for (int i=0; i<p; i++)
        parameters[i] = ((DoubleSpinBox*)boxParams->cellWidget(i, 2))->value();
    if (d_current_fit->type() == Fit::BuiltIn)
        modifyGuesses(parameters.data());//modifyGuesses(parameters);

    double x0 = boxFrom->value();
    double step = (boxTo->value() - x0)/(d_points - 1);
    for (int i=0; i<d_points; i++){
        double x = x0 + i*step;
        X[i] = x;
        Y[i] = d_current_fit->eval(parameters.data(), x);//Y[i] = d_current_fit->eval(parameters, x);
    }

	if (!d_preview_curve){
        d_preview_curve = new QwtPlotCurve();
		d_preview_curve->setRenderHint(QwtPlotItem::RenderAntialiased, d_graph->antialiasing());
        d_preview_curve->attach(d_graph->plotWidget());
	}

    d_preview_curve->setPen(QPen(ColorBox::color(boxColor->currentIndex()), 1));
    d_preview_curve->setData(X.data(), Y.data(), d_points);//d_preview_curve->setData(X, Y, d_points);
    d_graph->replot();
}

void FitDialog::showParameterRange(bool on)
{
    if (on){
        boxParams->showColumn(1);
        boxParams->showColumn(3);
    } else {
        boxParams->hideColumn(1);
        boxParams->hideColumn(3);
    }
}

QString FitDialog::parseFormula(const QString& s)
{
	QString formula = s;

	QStringList lst = userFunctionNames();
	for (int i=0; i<lst.count(); i++){
		if (formula.contains(lst[i]))
			formula.replace(lst[i], "(" + (d_user_functions[i])->formula() + ")");
	}

	QStringList builtInFunctions = builtInFunctionNames();
	for (int i=0; i<(int)builtInFunctions.count(); i++){
		if (formula.contains(builtInFunctions[i])){
			Fit *fit = d_built_in_functions[i];
			formula.replace(builtInFunctions[i], "(" + fit->formula() + ")");
		}
	}
	return formula;
}
