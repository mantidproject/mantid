/***************************************************************************
    File                 : MatrixValuesDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief,
                           Tilman Hoener zu Siederdissen,
                           Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
                           knut.franke*gmx.de
    Description          : Set matrix values dialog

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
#include "MatrixValuesDialog.h"
#include "MatrixCommand.h"

#include <QLayout>
#include <QSpinBox>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTextEdit>
#include <QTableWidget>
#include <QTableWidgetSelectionRange>
#ifdef SCRIPTING_PYTHON
#include <QCheckBox>
#endif

MatrixValuesDialog::MatrixValuesDialog( ScriptingEnv *env, QWidget* parent, Qt::WFlags fl )
: QDialog( parent, fl ), Scripted(env)
{
    setName( "MatrixValuesDialog" );
	setWindowTitle( tr( "MantidPlot - Set Matrix Values" ) );
	setSizeGripEnabled(true);

	QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel(tr("For row (i)")), 0, 0);
	startRow = new QSpinBox();
	startRow->setRange(1, 1000000);
    gl1->addWidget(startRow, 0, 1);
	gl1->addWidget(new QLabel(tr("to")), 0, 2);
	endRow =  new QSpinBox();
	endRow->setRange(1, 1000000);
	gl1->addWidget(endRow, 0, 3);
	gl1->addWidget(new QLabel(tr("For col (j)")), 1, 0);
	startCol = new QSpinBox();
	startCol->setRange(1, 1000000);
	gl1->addWidget(startCol, 1, 1);
	gl1->addWidget(new QLabel(tr("to")), 1, 2);
	endCol = new QSpinBox();
	endCol->setRange(1, 1000000);
	gl1->addWidget(endCol, 1, 3);

	functions = new QComboBox();
	btnAddFunction = new QPushButton(tr( "Add &Function" ));
	btnAddCell = new QPushButton(tr( "Add Ce&ll" ));

	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->addWidget(functions);
	hbox1->addWidget(btnAddFunction);
	hbox1->addWidget(btnAddCell);

	QVBoxLayout *vbox1 = new QVBoxLayout();
    vbox1->addLayout(gl1);
	vbox1->addLayout(hbox1);
	QGroupBox *gb = new QGroupBox();
    gb->setLayout(vbox1);
    gb->setSizePolicy(QSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred));

	explain = new QTextEdit();
	explain->setReadOnly(true);
	explain->setSizePolicy(QSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred));
    QPalette palette = explain->palette();
    palette.setColor(QPalette::Active, QPalette::Base, Qt::lightGray);
    explain->setPalette(palette);

	QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->addWidget(explain);
	hbox2->addWidget(gb);

	QHBoxLayout *hbox3 = new QHBoxLayout();

	commands = new ScriptEdit( scriptingEnv());
	commands->setFocus();
	hbox3->addWidget(commands);

	QVBoxLayout *vbox2 = new QVBoxLayout();
	btnApply = new QPushButton(tr( "&Apply" ));
    vbox2->addWidget(btnApply);
	btnCancel = new QPushButton(tr( "&Close" ));
    vbox2->addWidget(btnCancel);
    vbox2->addStretch();

	hbox3->addLayout(vbox2);

	QVBoxLayout* vbox3 = new QVBoxLayout(this);
	vbox3->addLayout(hbox2);
#ifdef SCRIPTING_PYTHON	
	boxMuParser = NULL;
	if (scriptingEnv()->name() != QString("muParser")){
		boxMuParser = new QCheckBox(tr("Use built-in muParser (much faster)"));
		boxMuParser->setChecked(true);
		vbox3->addWidget(boxMuParser);
	}
#endif
	vbox3->addWidget(new QLabel(tr( "Cell(i,j)=" )));
	vbox3->addLayout(hbox3);

	functions->insertStringList(scriptingEnv()->mathFunctions(), -1);
	insertExplain(0);

	connect(btnAddCell, SIGNAL(clicked()), this, SLOT(addCell()));
	connect(btnAddFunction, SIGNAL(clicked()), this, SLOT(insertFunction()));
	connect(btnApply, SIGNAL(clicked()), this, SLOT(apply()));
	connect(btnCancel, SIGNAL(clicked()), this, SLOT(close()));
	connect(functions, SIGNAL(activated(int)), this, SLOT(insertExplain(int)));
}

QSize MatrixValuesDialog::sizeHint() const
{
	return QSize( 400, 190 );
}

void MatrixValuesDialog::customEvent(QEvent *e)
{
	if (e->type() == SCRIPTING_CHANGE_EVENT)
		scriptingChangeEvent((ScriptingChangeEvent*)e);
}

bool MatrixValuesDialog::apply()
{
	QString formula = commands->text();
	QString oldFormula = matrix->formula();

	matrix->setFormula(formula);
	
	bool useMuParser = true;
#ifdef SCRIPTING_PYTHON
	if (boxMuParser)
		useMuParser = boxMuParser->isChecked();
#endif
	
	if (matrix->canCalculate(useMuParser)){
		matrix->undoStack()->push(new MatrixSetFormulaCommand(matrix, oldFormula, formula,
							tr("Set New Formula") + " \"" + formula + "\""));

		if (matrix->calculate(startRow->value()-1, endRow->value()-1, 
			startCol->value()-1, endCol->value()-1, useMuParser))
			return true;
	}
	matrix->setFormula(oldFormula);
	return false;
}

void MatrixValuesDialog::setMatrix(Matrix* m)
{
    if (!m)
        return;

	matrix = m;
	commands->setText(m->formula());
	commands->setContext(m);

    endCol->setValue(m->numCols());
    endRow->setValue(m->numRows());

    if (m->viewType() == Matrix::TableView){
        QItemSelectionModel *selModel = m->selectionModel();
        if (selModel->hasSelection()){
            QItemSelectionRange selection = selModel->selection().first();
            if (selection.width() > 1 || selection.height() > 1){
                startCol->setValue(selection.left()+1);
                startRow->setValue(selection.top()+1);
                endCol->setValue(selection.right()+1);
                endRow->setValue(selection.bottom()+1);
            }
        }
    }
}

void MatrixValuesDialog::insertExplain(int index)
{
	explain->setText(scriptingEnv()->mathFunctionDoc(functions->text(index)));
}

void MatrixValuesDialog::insertFunction()
{
	commands->insertFunction(functions->currentText());
}

void MatrixValuesDialog::addCell()
{
	commands->insert("cell(i, j)");
}
