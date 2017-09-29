/***************************************************************************
    File                 : MatrixDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 - 2008 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Matrix properties dialog

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
#ifndef MATRIXDIALOG_H
#define MATRIXDIALOG_H

#include <QDialog>

class Matrix;
class QPushButton;
class QSpinBox;
class QComboBox;

//! Matrix properties dialog
class MatrixDialog : public QDialog {
  Q_OBJECT

public:
  //! Constructor
  /**
   * @param parent :: parent widget
   * @param fl :: window flags
   */
  MatrixDialog(QWidget *parent = nullptr, Qt::WFlags fl = nullptr);
  void setMatrix(Matrix *m);

private slots:
  //! Accept changes and quit
  void accept() override;
  //! Apply changes
  void apply();
  //! Activate the numeric precision choice box
  void showPrecisionBox(int item);

private:
  Matrix *d_matrix;

  QPushButton *buttonOk;
  QPushButton *buttonCancel, *buttonApply;
  QSpinBox *boxColWidth, *boxPrecision;
  QComboBox *boxFormat, *boxNumericDisplay;
};

#endif // MATRIXDIALOG_H
