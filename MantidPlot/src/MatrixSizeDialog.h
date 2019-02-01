// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2004 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : MatrixSizeDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef MATRIXSIZEDIALOG_H
#define MATRIXSIZEDIALOG_H

#include "Matrix.h"
#include <QDialog>

class QGroupBox;
class QPushButton;
class QSpinBox;
class DoubleSpinBox;

//! Matrix dimensions dialog
class MatrixSizeDialog : public QDialog {
  Q_OBJECT

public:
  //! Constructor
  /**
   * @param parent :: parent widget
   * @param fl :: window flags
   */
  MatrixSizeDialog(Matrix *m, QWidget *parent = nullptr,
                   Qt::WFlags fl = nullptr);

private slots:
  //! Accept changes and quit
  void accept() override;
  //! Apply changes
  void apply();

private:
  QPushButton *buttonOk, *buttonApply;
  QPushButton *buttonCancel;
  QGroupBox *groupBox1, *groupBox2;
  QSpinBox *boxCols, *boxRows;
  DoubleSpinBox *boxXStart, *boxYStart, *boxXEnd, *boxYEnd;
  Matrix *d_matrix;
};

#endif // MATRIXSIZEDIALOG_H
