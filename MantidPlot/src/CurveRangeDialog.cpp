// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : CurveRangeDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "CurveRangeDialog.h"
#include "Graph.h"
#include "PlotCurve.h"
#include "Table.h"

#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

CurveRangeDialog::CurveRangeDialog(QWidget *parent, Qt::WFlags fl)
    : QDialog(parent, fl), d_curve(nullptr), d_graph(nullptr) {
  setWindowTitle(tr("MantidPlot - Plot range"));
  setObjectName("CurveRangeDialog");

  QGroupBox *gb1 = new QGroupBox();
  QGridLayout *gl1 = new QGridLayout(gb1);
  gl1->addWidget(new QLabel(tr("Data set: ")), 0, 0);

  boxName = new QLabel();
  gl1->addWidget(boxName, 0, 1);

  gl1->addWidget(new QLabel(tr("From row number")), 1, 0);
  boxStart = new QSpinBox();
  boxStart->setMinimum(1);
  gl1->addWidget(boxStart, 1, 1);

  gl1->addWidget(new QLabel(tr("To row number")), 2, 0);
  boxEnd = new QSpinBox();
  boxEnd->setMinimum(1);
  gl1->addWidget(boxEnd, 2, 1);
  gl1->setRowStretch(3, 1);

  buttonOK = new QPushButton(tr("&OK"));
  buttonOK->setDefault(true);
  buttonCancel = new QPushButton(tr("&Close"));

  QHBoxLayout *hl = new QHBoxLayout();
  hl->addStretch();
  hl->addWidget(buttonOK);
  hl->addWidget(buttonCancel);

  QVBoxLayout *vb = new QVBoxLayout(this);
  vb->addWidget(gb1);
  vb->addLayout(hl);

  connect(buttonOK, SIGNAL(clicked()), this, SLOT(accept()));
  connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void CurveRangeDialog::accept() {
  if (!d_curve)
    return;

  int start = boxStart->value() - 1;
  int end = boxEnd->value() - 1;
  d_curve->setRowRange(qMin(start, end), qMax(start, end));
  d_graph->updatePlot();
  d_graph->notifyChanges();
  close();
}

void CurveRangeDialog::setCurveToModify(Graph *g, int curve) {
  if (!g)
    return;

  d_graph = g;
  d_curve = dynamic_cast<DataCurve *>(d_graph->curve(curve));
  if (!d_curve)
    return;

  Table *t = d_curve->table();
  if (t) {
    boxStart->setMaximum(t->numRows());
    boxEnd->setMaximum(t->numRows());
  }

  boxName->setText(d_curve->title().text());
  boxStart->setValue(d_curve->startRow() + 1);
  boxEnd->setValue(d_curve->endRow() + 1);
}
