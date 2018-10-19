// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : CurveRangeDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef CURVERANGEDIALOG_H
#define CURVERANGEDIALOG_H

#include <QDialog>

class QPushButton;
class QLabel;
class QSpinBox;
class Graph;
class DataCurve;

//! Curve range dialog
class CurveRangeDialog : public QDialog {
  Q_OBJECT

public:
  CurveRangeDialog(QWidget *parent = nullptr, Qt::WFlags fl = nullptr);

public slots:
  void setCurveToModify(Graph *g, int curve);
  void accept() override;

private:
  DataCurve *d_curve;
  Graph *d_graph;

  QPushButton *buttonOK;
  QPushButton *buttonCancel;
  QLabel *boxName;
  QSpinBox *boxStart;
  QSpinBox *boxEnd;
};

#endif
