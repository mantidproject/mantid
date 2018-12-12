// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2006 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : SurfaceDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef SURFACEDIALOG_H
#define SURFACEDIALOG_H

#include <QDialog>

class QPushButton;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QStackedWidget;
class QSpinBox;
class Graph3D;

//! Define surface plot dialog
class SurfaceDialog : public QDialog {
  Q_OBJECT

public:
  SurfaceDialog(QWidget *parent = nullptr, Qt::WFlags fl = nullptr);

public slots:
  void accept() override;
  void setFunction(Graph3D *);
  void setParametricSurface(Graph3D *);

private slots:
  void clearList();

private:
  Graph3D *d_graph;

  void acceptParametricSurface();
  void acceptFunction();
  void initFunctionPage();
  void initParametricSurfacePage();

  QWidget *functionPage;
  QWidget *parametricPage;
  QStackedWidget *optionStack;
  QPushButton *buttonOk;
  QPushButton *buttonCancel;
  QPushButton *buttonClear;
  QComboBox *boxType;
  QComboBox *boxFunction;
  QLineEdit *boxXFrom;
  QLineEdit *boxXTo;
  QLineEdit *boxYFrom;
  QLineEdit *boxYTo;
  QLineEdit *boxZFrom;
  QLineEdit *boxZTo;

  QLineEdit *boxX;
  QLineEdit *boxY;
  QLineEdit *boxZ;

  QLineEdit *boxUFrom;
  QLineEdit *boxUTo;
  QLineEdit *boxVFrom;
  QLineEdit *boxVTo;

  QCheckBox *boxUPeriodic, *boxVPeriodic;
  QSpinBox *boxColumns, *boxRows, *boxFuncColumns, *boxFuncRows;
};

#endif
