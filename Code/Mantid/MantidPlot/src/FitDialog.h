/***************************************************************************
    File                 : FitDialog.h
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
#ifndef FITDIALOG_H
#define FITDIALOG_H

#include "Graph.h"
#include <QDoubleSpinBox>
#include <QCheckBox>

class QPushButton;
class QLineEdit;
class QComboBox;
class QStackedWidget;
class QWidget;
class QTextEdit;
class QListWidget;
class QTableWidget;
class QSpinBox;
class QLabel;
class QRadioButton;
class QLineEdit;
class ColorBox;
class Fit;
class Table;
class DoubleSpinBox;
class QwtPlotCurve;

//! Fit Wizard
class FitDialog : public QDialog
{
    Q_OBJECT

public:
    FitDialog(Graph *g, QWidget* parent = 0, Qt::WFlags fl = 0 );

    void setSrcTables(QList<MdiSubWindow*> tables);

protected:
	void closeEvent (QCloseEvent * e );
    void initFitPage();
	void initEditPage();
	void initAdvancedPage();

private slots:
	void accept();
    //! Clears the function editor, the parameter names and the function name
    void resetFunction();
	void showFitPage();
	void showEditPage();
	void showAdvancedPage();
	void showFunctionsList(int category);
	void showParseFunctions();
	void showExpression(int function);
	void addFunction();
	void addFunctionName();
	void setFunction(bool ok);
	void saveUserFunction();
	void removeUserFunction();
	void setGraph(Graph *g);
	void activateCurve(const QString& curveName);
	void chooseFolder();
	void changeDataRange();
	void selectSrcTable(int tabnr);
	void enableWeightingParameters(int index);
	void showPointsBox(bool);
	void showParametersTable();
	void showCovarianceMatrix();

	//! Applies the user changes to the numerical format of the output results
	void applyChanges();

	//! Deletes the result fit curves from the plot
	void deleteFitCurves();

    //! Enable the "Apply" button
	void enableApplyChanges(int = 0);
	void setNumPeaks(int peaks);
	void saveInitialGuesses();
	void returnToFitPage();
	void updatePreview();
	void showParameterRange(bool);

private:
	void loadPlugins();
    void loadUserFunctions();
	void initBuiltInFunctions();
	void modifyGuesses(double* initVal);
	QStringList builtInFunctionNames();
	QStringList userFunctionNames();
	QStringList plugInNames();
	QString parseFormula(const QString& s);

    Fit *d_current_fit;
	Graph *d_graph;
	Table *d_param_table;
	QList <Fit*> d_user_functions, d_built_in_functions, d_plugins;
	QList <MdiSubWindow*> srcTables;
	QwtPlotCurve *d_preview_curve;

    QCheckBox* boxUseBuiltIn;
	QStackedWidget* tw;
    QPushButton* buttonOk;
	QPushButton* buttonCancel1;
	QPushButton* buttonCancel2;
	QPushButton* buttonCancel3;
	QPushButton* buttonAdvanced;
	QPushButton* buttonClear;
	QPushButton* buttonPlugins;
	QPushButton* btnBack;
	QPushButton* btnSaveGuesses;
	QComboBox* boxCurve;
	QComboBox* boxAlgorithm;
	QTableWidget* boxParams;
	DoubleSpinBox* boxFrom;
	DoubleSpinBox* boxTo;
	DoubleSpinBox* boxTolerance;
	QSpinBox* boxPoints, *generatePointsBox, *boxPrecision, *polynomOrderBox;
	QWidget *fitPage, *editPage, *advancedPage;
	QTextEdit *editBox, *explainBox, *boxFunction;
	QListWidget *categoryBox, *funcBox;
	QLineEdit *boxName, *boxParam;
	QLabel *lblFunction, *lblPoints, *polynomOrderLabel;
	QPushButton *btnAddFunc, *btnDelFunc, *btnContinue, *btnApply;
	QPushButton *buttonEdit, *btnAddTxt, *btnAddName, *btnDeleteFitCurves;
	ColorBox* boxColor;
	QComboBox *boxWeighting, *tableNamesBox, *colNamesBox;
	QRadioButton *generatePointsBtn, *samePointsBtn;
	QPushButton *btnParamTable, *btnCovMatrix, *btnParamRange;
	QLineEdit *covMatrixName, *paramTableName;
	QCheckBox *plotLabelBox, *logBox, *scaleErrorsBox, *globalParamTableBox;
	QCheckBox *previewBox;
};
#endif // FITDIALOG_H
