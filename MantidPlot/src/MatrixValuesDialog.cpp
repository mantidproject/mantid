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
#include "MantidQtWidgets/Common/ScriptEditor.h"
#include "MatrixCommand.h"

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetSelectionRange>
#include <QTextEdit>
#ifdef SCRIPTING_PYTHON
#include <QCheckBox>
#endif

MatrixValuesDialog::MatrixValuesDialog(ScriptingEnv *env, QWidget *parent,
                                       Qt::WFlags fl)
    : QDialog(parent, fl), Scripted(env), matrix(nullptr) {
  setObjectName("MatrixValuesDialog");
  setWindowTitle(tr("MantidPlot - Set Matrix Values"));
  setSizeGripEnabled(true);

  QGridLayout *gl1 = new QGridLayout();
  gl1->addWidget(new QLabel(tr("For row (i)")), 0, 0);
  startRow = new QSpinBox();
  startRow->setRange(1, 1000000);
  gl1->addWidget(startRow, 0, 1);
  gl1->addWidget(new QLabel(tr("to")), 0, 2);
  endRow = new QSpinBox();
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

  QVBoxLayout *vbox1 = new QVBoxLayout();
  vbox1->addLayout(gl1);
  QGroupBox *gb = new QGroupBox();
  gb->setLayout(vbox1);
  gb->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred));

  QHBoxLayout *hbox3 = new QHBoxLayout();
  commands = new ScriptEditor(this, scriptingEnv()->createCodeLexer());
  commands->setFocus();
  hbox3->addWidget(commands);

  QVBoxLayout *vbox2 = new QVBoxLayout();
  btnApply = new QPushButton(tr("&Apply"));
  vbox2->addWidget(btnApply);
  btnCancel = new QPushButton(tr("&Close"));
  vbox2->addWidget(btnCancel);
  vbox2->addStretch();

  QHBoxLayout *hbox2 = new QHBoxLayout();
  hbox2->addWidget(gb);
  hbox2->addLayout(vbox2);

  QVBoxLayout *vbox3 = new QVBoxLayout(this);
  vbox3->addLayout(hbox2);
  vbox3->addWidget(new QLabel(tr("Cell(i,j)=")));
  vbox3->addLayout(hbox3);

  connect(btnApply, SIGNAL(clicked()), this, SLOT(apply()));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(close()));
}

QSize MatrixValuesDialog::sizeHint() const { return QSize(400, 190); }

void MatrixValuesDialog::customEvent(QEvent *e) {
  if (e->type() == SCRIPTING_CHANGE_EVENT) {
    if (auto sce = dynamic_cast<ScriptingChangeEvent *>(e)) {
      scriptingChangeEvent(sce);
    }
  }
}

bool MatrixValuesDialog::apply() {
  QString formula = commands->text();
  QString oldFormula = matrix->formula();

  matrix->setFormula(formula);

  bool useMuParser = true;

  if (matrix->canCalculate(useMuParser)) {
    matrix->undoStack()->push(new MatrixSetFormulaCommand(
        matrix, oldFormula, formula,
        tr("Set New Formula") + " \"" + formula + "\""));

    if (matrix->calculate(startRow->value() - 1, endRow->value() - 1,
                          startCol->value() - 1, endCol->value() - 1,
                          useMuParser))
      return true;
  }
  matrix->setFormula(oldFormula);
  return false;
}

void MatrixValuesDialog::setMatrix(Matrix *m) {
  if (!m)
    return;

  matrix = m;
  commands->setText(m->formula());

  endCol->setValue(m->numCols());
  endRow->setValue(m->numRows());

  if (m->viewType() == Matrix::TableView) {
    QItemSelectionModel *selModel = m->selectionModel();
    if (selModel->hasSelection()) {
      QItemSelectionRange selection = selModel->selection().first();
      if (selection.width() > 1 || selection.height() > 1) {
        startCol->setValue(selection.left() + 1);
        startRow->setValue(selection.top() + 1);
        endCol->setValue(selection.right() + 1);
        endRow->setValue(selection.bottom() + 1);
      }
    }
  }
}
