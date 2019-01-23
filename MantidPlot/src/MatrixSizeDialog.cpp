// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2004 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : MatrixSizeDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "MatrixSizeDialog.h"
#include "MantidQtWidgets/Common/DoubleSpinBox.h"
#include "MatrixCommand.h"

#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

MatrixSizeDialog::MatrixSizeDialog(Matrix *m, QWidget *parent, Qt::WFlags fl)
    : QDialog(parent, fl), d_matrix(m) {
  setWindowTitle(tr("MantidPlot - Matrix Dimensions"));

  groupBox1 = new QGroupBox(tr("Dimensions"));
  QHBoxLayout *topLayout = new QHBoxLayout(groupBox1);
  topLayout->addWidget(new QLabel(tr("Rows")));
  boxRows = new QSpinBox();
  boxRows->setRange(1, 1000000);
  topLayout->addWidget(boxRows);
  topLayout->addStretch();
  topLayout->addWidget(new QLabel(tr("Columns")));
  boxCols = new QSpinBox();
  boxCols->setRange(1, 1000000);
  topLayout->addWidget(boxCols);

  groupBox2 = new QGroupBox(tr("Coordinates"));
  QGridLayout *centerLayout = new QGridLayout(groupBox2);
  centerLayout->addWidget(new QLabel(tr("X (Columns)")), 0, 1);
  centerLayout->addWidget(new QLabel(tr("Y (Rows)")), 0, 2);

  centerLayout->addWidget(new QLabel(tr("First")), 1, 0);

  QLocale locale = m->locale();
  boxXStart = new DoubleSpinBox();
  boxXStart->setLocale(locale);
  centerLayout->addWidget(boxXStart, 1, 1);

  boxYStart = new DoubleSpinBox();
  boxYStart->setLocale(locale);
  centerLayout->addWidget(boxYStart, 1, 2);

  centerLayout->addWidget(new QLabel(tr("Last")), 2, 0);
  boxXEnd = new DoubleSpinBox();
  boxXEnd->setLocale(locale);
  centerLayout->addWidget(boxXEnd, 2, 1);

  boxYEnd = new DoubleSpinBox();
  boxYEnd->setLocale(locale);
  centerLayout->addWidget(boxYEnd, 2, 2);
  centerLayout->setRowStretch(3, 1);

  QHBoxLayout *bottomLayout = new QHBoxLayout();
  bottomLayout->addStretch();
  buttonApply = new QPushButton(tr("&Apply"));
  buttonApply->setDefault(true);
  bottomLayout->addWidget(buttonApply);
  buttonOk = new QPushButton(tr("&OK"));
  bottomLayout->addWidget(buttonOk);
  buttonCancel = new QPushButton(tr("&Cancel"));
  bottomLayout->addWidget(buttonCancel);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(groupBox1);
  mainLayout->addWidget(groupBox2);
  mainLayout->addLayout(bottomLayout);

  boxRows->setValue(m->numRows());
  boxCols->setValue(m->numCols());

  boxXStart->setValue(m->xStart());
  boxYStart->setValue(m->yStart());
  boxXEnd->setValue(m->xEnd());
  boxYEnd->setValue(m->yEnd());

  connect(buttonApply, SIGNAL(clicked()), this, SLOT(apply()));
  connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
  connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void MatrixSizeDialog::apply() {
  double fromX = boxXStart->value();
  double toX = boxXEnd->value();
  double fromY = boxYStart->value();
  double toY = boxYEnd->value();
  double oxs = d_matrix->xStart();
  double oxe = d_matrix->xEnd();
  double oys = d_matrix->yStart();
  double oye = d_matrix->yEnd();
  if (oxs != fromX || oxe != toX || oys != fromY || oye != toY) {
    d_matrix->undoStack()->push(new MatrixSetCoordinatesCommand(
        d_matrix, oxs, oxe, oys, oye, fromX, toX, fromY, toY,
        tr("Set Coordinates x[%1 : %2], y[%3 : %4]")
            .arg(fromX)
            .arg(toX)
            .arg(fromY)
            .arg(toY)));
    d_matrix->setCoordinates(fromX, toX, fromY, toY);
  }
  d_matrix->setDimensions(boxRows->value(), boxCols->value());
}

void MatrixSizeDialog::accept() {
  apply();
  close();
}
