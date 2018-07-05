/***************************************************************************
    File                 : SetColValuesDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief,
                           Tilman Hoener zu Siederdissen,
                           Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
                           knut.franke*gmx.de
    Description          : Set column values dialog

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
#include "SetColValuesDialog.h"
#include "MantidQtWidgets/Common/ScriptEditor.h"
#include "Table.h"

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetSelectionRange>
#include <QTextCursor>
#include <QTextEdit>
#ifdef SCRIPTING_PYTHON
#include <QCheckBox>
#endif

SetColValuesDialog::SetColValuesDialog(ScriptingEnv *env, Table *t,
                                       Qt::WFlags fl)
    : QDialog(t, fl), Scripted(env) {
  setObjectName("SetColValuesDialog");
  setWindowTitle(tr("MantidPlot - Set column values"));
  setSizeGripEnabled(true);

  QHBoxLayout *hbox1 = new QHBoxLayout();
  hbox1->addWidget(new QLabel(tr("For row (i)")));
  start = new QSpinBox();
  start->setMinimum(1);
  hbox1->addWidget(start);

  hbox1->addWidget(new QLabel(tr("to")));

  end = new QSpinBox();
  end->setMinimum(1);
  hbox1->addWidget(end);

  // Ideally this would be checked at compile time. Until we have 'constexpr if`
  // on all platforms, the added complexity and minimal cost isn't worthwhile.
  if (sizeof(int) == 2) { // 16 bit signed integer
    start->setMaximum(0x7fff);
    end->setMaximum(0x7fff);
  } else { // 32 bit signed integer
    start->setMaximum(0x7fffffff);
    end->setMaximum(0x7fffffff);
  }

  QGridLayout *gl1 = new QGridLayout();
  boxColumn = new QComboBox();
  gl1->addWidget(boxColumn, 1, 0);
  btnAddCol = new QPushButton(tr("Add column"));
  gl1->addWidget(btnAddCol, 1, 1);

  QHBoxLayout *hbox3 = new QHBoxLayout();
  hbox3->addStretch();
  buttonPrev = new QPushButton("&<<");
  hbox3->addWidget(buttonPrev);
  buttonNext = new QPushButton("&>>");
  hbox3->addWidget(buttonNext);
  gl1->addLayout(hbox3, 2, 0);
  addCellButton = new QPushButton(tr("Add cell"));
  gl1->addWidget(addCellButton, 2, 1);

  QGroupBox *gb = new QGroupBox();
  QVBoxLayout *vbox1 = new QVBoxLayout();
  vbox1->addLayout(hbox1);
  vbox1->addLayout(gl1);
  gb->setLayout(vbox1);
  gb->setSizePolicy(
      QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));

  explain = new QTextEdit();
  explain->setReadOnly(true);
  explain->setSizePolicy(
      QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
  QPalette palette = explain->palette();
  palette.setColor(QPalette::Active, QPalette::Base, Qt::lightGray);
  explain->setPalette(palette);

  QHBoxLayout *hbox2 = new QHBoxLayout();
  hbox2->addWidget(explain);
  hbox2->addWidget(gb);

  commands = new ScriptEditor(this, scriptingEnv()->createCodeLexer());

  QVBoxLayout *vbox2 = new QVBoxLayout();
  btnApply = new QPushButton(tr("&Apply"));
  vbox2->addWidget(btnApply);
  btnCancel = new QPushButton(tr("&Close"));
  vbox2->addWidget(btnCancel);
  vbox2->addStretch();

  QHBoxLayout *hbox4 = new QHBoxLayout();
  hbox4->addWidget(commands);
  hbox4->addLayout(vbox2);

  QVBoxLayout *vbox3 = new QVBoxLayout();
  vbox3->addLayout(hbox2);

  colNameLabel = new QLabel();
  vbox3->addWidget(colNameLabel);
  vbox3->addLayout(hbox4);

  setLayout(vbox3);
  setFocusProxy(commands);
  commands->setFocus();

  connect(btnAddCol, SIGNAL(clicked()), this, SLOT(insertCol()));
  connect(addCellButton, SIGNAL(clicked()), this, SLOT(insertCell()));
  connect(btnApply, SIGNAL(clicked()), this, SLOT(apply()));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(close()));
  connect(buttonPrev, SIGNAL(clicked()), this, SLOT(prevColumn()));
  connect(buttonNext, SIGNAL(clicked()), this, SLOT(nextColumn()));

  setTable(t);
}

void SetColValuesDialog::prevColumn() {
  int sc = table->selectedColumn();
  updateColumn(--sc);
}

void SetColValuesDialog::nextColumn() {
  int sc = table->selectedColumn();
  updateColumn(++sc);
}

void SetColValuesDialog::updateColumn(int sc) {
  if (sc < 0 || sc > table->numCols() - 1)
    return;

  if (sc == 0)
    buttonPrev->setEnabled(false);
  else
    buttonPrev->setEnabled(true);

  if (sc == table->numCols() - 1)
    buttonNext->setEnabled(false);
  else
    buttonNext->setEnabled(true);

  table->setSelectedCol(sc);
  table->table()->clearSelection();
  table->table()->selectColumn(sc);
  colNameLabel->setText("col(\"" + table->colLabel(sc) + "\")= ");

  QStringList com = table->getCommands();

  // Mantid - Changes due to ScriptEdit changes
  commands->setText(com[sc]);
  commands->ensureCursorVisible();
}

QSize SetColValuesDialog::sizeHint() const { return QSize(400, 190); }

void SetColValuesDialog::customEvent(QEvent *e) {
  if (e->type() == SCRIPTING_CHANGE_EVENT)
    scriptingChangeEvent(static_cast<ScriptingChangeEvent *>(e));
}

bool SetColValuesDialog::apply() {
  int col = table->selectedColumn();
  if (col < 0 || col > table->numCols() - 1)
    return false;

  QString formula = commands->text();
  QString oldFormula = table->getCommands()[col];

  table->setCommand(col, formula);
  bool useMuParser = true;
  if (table->calculate(col, start->value() - 1, end->value() - 1, useMuParser))
    return true;

  table->setCommand(col, oldFormula);
  return false;
}

void SetColValuesDialog::insertCol() {
  // Mantid - Changes due to ScriptEdit changes
  commands->append(boxColumn->currentText());
}

void SetColValuesDialog::insertCell() {
  QString f = boxColumn->currentText().remove(")") + ", i)";
  // Mantid - Changes due to ScriptEdit changes
  commands->append(f);
}

void SetColValuesDialog::setTable(Table *w) {
  table = w;
  QStringList colNames = w->colNames();
  int cols = w->numCols();
  for (int i = 0; i < cols; i++)
    boxColumn->insertItem(i, "col(\"" + colNames[i] + "\")");

  if (w->hasSelection()) {
    w->setSelectedCol(w->leftSelectedColumn());

    start->setValue(w->topSelectedRow() + 1);
    end->setValue(w->bottomSelectedRow() + 1);
  } else {
    start->setValue(1);
    end->setValue(w->numRows());
  }

  updateColumn(w->selectedColumn());
}
