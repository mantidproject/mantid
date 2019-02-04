// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2006 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : MatrixDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
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
