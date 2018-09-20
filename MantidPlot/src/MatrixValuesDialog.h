/***************************************************************************
    File                 : MatrixValuesDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief,
                           Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
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
#ifndef MVALUESDIALOG_H
#define MVALUESDIALOG_H

#include "Matrix.h"
#include "Script.h"
#include "ScriptingEnv.h"
#include <QDialog>

#ifdef SCRIPTING_PYTHON
class QCheckBox;
#endif
class QComboBox;
class QTextEdit;
class QSpinBox;
class QPushButton;
class ScriptEditor;
class Matrix;

//! Set matrix values dialog
class MatrixValuesDialog : public QDialog, public Scripted {
  Q_OBJECT

public:
  MatrixValuesDialog(ScriptingEnv *env, QWidget *parent = nullptr,
                     Qt::WFlags fl = nullptr);
  void setMatrix(Matrix *m);

private slots:
  bool apply();

private:
  Matrix *matrix;

  QSize sizeHint() const override;
  void customEvent(QEvent *e) override;

  ScriptEditor *commands;
  QPushButton *btnCancel;
  QSpinBox *startRow, *endRow, *startCol, *endCol;
  QPushButton *btnApply;
};

#endif //
