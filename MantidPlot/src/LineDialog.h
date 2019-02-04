// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2006 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : LineDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef LINEDIALOG_H
#define LINEDIALOG_H

#include <qdialog.h>

class QCheckBox;
class QComboBox;
class QPushButton;
class QTabWidget;
class QWidget;
class QSpinBox;
class DoubleSpinBox;
class ColorButton;
class ArrowMarker;
class DoubleSpinBox;

//! Line options dialog
class LineDialog : public QDialog {
  Q_OBJECT

public:
  LineDialog(ArrowMarker *line, QWidget *parent = nullptr,
             Qt::WFlags fl = nullptr);

  enum Unit { ScaleCoordinates, Pixels };

  void initGeometryTab();
  void enableHeadTab();
  void setCoordinates(int unit);

public slots:
  void enableButtonDefault(QWidget *w);
  void setDefaultValues();
  void displayCoordinates(int unit);
  void setLineStyle(Qt::PenStyle style);
  void accept() override;
  void apply();

private:
  ArrowMarker *lm;

  ColorButton *colorBox;
  QComboBox *styleBox;
  DoubleSpinBox *widthBox;
  QComboBox *unitBox;
  QPushButton *btnOk;
  QPushButton *btnApply;
  QPushButton *buttonDefault;
  QCheckBox *endBox;
  QCheckBox *startBox, *filledBox;
  QTabWidget *tw;
  QWidget *options, *geometry, *head;
  DoubleSpinBox *xStartBox, *yStartBox, *xEndBox, *yEndBox;
  QSpinBox *xStartPixelBox, *yStartPixelBox, *xEndPixelBox, *yEndPixelBox;
  QSpinBox *boxHeadAngle, *boxHeadLength;
};

#endif // LINEDIALOG_H
