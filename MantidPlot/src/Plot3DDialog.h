// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2004 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : Plot3DDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef PLOT3DDIALOG_H
#define PLOT3DDIALOG_H

#include "Graph3D.h"
#include <QCheckBox>

class QComboBox;
class QLabel;
class QLineEdit;
class QTextEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTabWidget;
class QWidget;
class QStringList;
class QStackedWidget;
class QDoubleSpinBox;
class ColorButton;
class TextFormatButtons;

//! Surface plot options dialog
class Plot3DDialog : public QDialog {
  Q_OBJECT

public:
  Plot3DDialog(QWidget *parent, Qt::WFlags fl = nullptr);
  void setPlot(Graph3D *);

  void showTitleTab();
  void showAxisTab();
  void showGeneralTab();

private slots:
  void accept() override;
  bool updatePlot();

  void pickTitleFont();
  void viewAxisOptions(int axis);
  QFont axisFont(int axis);
  void pickAxisLabelFont();
  void pickNumbersFont();

  QStringList scaleOptions(int axis, double start, double end,
                           const QString &majors, const QString &minors);
  void viewScaleLimits(int axis);
  void disableMeshOptions();
  void showBarsTab(double rad);
  void showPointsTab(double rad, bool smooth);
  void showConesTab(double rad, int quality);
  void showCrossHairTab(double rad, double linewidth, bool smooth, bool boxed);

  void worksheet();

  void initPointsOptionsStack();
  void changeZoom(int);
  void changeTransparency(int val);
  void pickDataColorMap();

private:
  void initScalesPage();
  void initAxesPage();
  void initTitlePage();
  void initColorsPage();
  void initGeneralPage();

  Graph3D *d_plot;
  QFont titleFont, xAxisFont, yAxisFont, zAxisFont, numbersFont;
  QStringList labels, scales, tickLengths;
  QDoubleSpinBox *boxMeshLineWidth;
  QPushButton *buttonApply;
  QPushButton *buttonOk;
  QPushButton *buttonCancel;
  QPushButton *btnTitleFont, *btnLabelFont;
  QPushButton *btnNumbersFont, *btnTable, *btnColorMap;
  ColorButton *btnBackground, *btnMesh, *btnAxes, *btnTitleColor, *btnLabels;
  ColorButton *btnFromColor, *btnToColor, *btnNumbers, *btnGrid;
  QTabWidget *generalDialog;
  QWidget *scale, *colors, *general, *axes, *title, *bars, *points;
  QLineEdit *boxFrom, *boxTo;
  QTextEdit *boxTitle, *boxLabel;
  QSpinBox *boxMajors, *boxMinors;
  QGroupBox *TicksGroupBox, *AxesColorGroupBox;
  QSpinBox *boxResolution, *boxDistance, *boxTransparency;
  QCheckBox *boxLegend, *boxSmooth, *boxBoxed, *boxCrossSmooth, *boxOrthogonal;
  QListWidget *axesList, *axesList2;
  QComboBox *boxType, *boxPointStyle;
  QLineEdit *boxMajorLength, *boxMinorLength, *boxConesRad;
  QSpinBox *boxZoom, *boxXScale, *boxYScale, *boxZScale, *boxQuality;
  QLineEdit *boxSize, *boxBarsRad, *boxCrossRad, *boxCrossLinewidth;
  QStackedWidget *optionStack;
  QWidget *dotsPage, *conesPage, *crossPage;
  TextFormatButtons *titleFormatButtons, *axisTitleFormatButtons;
  double xScale;
  double yScale;
  double zScale;
  double zoom;
};

#endif
