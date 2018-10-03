// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2006 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : LineDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "LineDialog.h"
#include "ApplicationWindow.h"
#include "ArrowMarker.h"
#include "ColorButton.h"
#include "Graph.h"
#include "MantidQtWidgets/Common/DoubleSpinBox.h"
#include "Plot.h"

#include <qwt_plot.h>

#include <QComboBox>
#include <QGroupBox>
#include <QSpinBox>

LineDialog::LineDialog(ArrowMarker *line, QWidget *parent, Qt::WFlags fl)
    : QDialog(parent, fl) {
  unitBox = nullptr;

  setWindowTitle(tr("MantidPlot - Line options"));
  setAttribute(Qt::WA_DeleteOnClose);

  lm = line;

  QGroupBox *gb1 = new QGroupBox();
  QGridLayout *gl1 = new QGridLayout();

  gl1->addWidget(new QLabel(tr("Color")), 0, 0);
  colorBox = new ColorButton();
  colorBox->setColor(lm->color());
  gl1->addWidget(colorBox, 0, 1);

  gl1->addWidget(new QLabel(tr("Line type")), 1, 0);
  styleBox = new QComboBox();
  styleBox->addItem("_____");
  styleBox->addItem("- - -");
  styleBox->addItem(".....");
  styleBox->addItem("_._._");
  styleBox->addItem("_.._..");
  gl1->addWidget(styleBox, 1, 1);

  setLineStyle(lm->style());

  gl1->addWidget(new QLabel(tr("Line width")), 2, 0);
  widthBox = new DoubleSpinBox('f');
  ApplicationWindow *appWin = dynamic_cast<ApplicationWindow *>(this->parent());
  if (appWin) {
    widthBox->setLocale(appWin->locale());
  }
  widthBox->setSingleStep(0.1);
  widthBox->setRange(0, 100);
  widthBox->setValue(lm->width());
  gl1->addWidget(widthBox, 2, 1);

  startBox = new QCheckBox();
  startBox->setText(tr("Arrow at &start"));
  startBox->setChecked(lm->hasStartArrow());
  gl1->addWidget(startBox, 3, 0);

  endBox = new QCheckBox();
  endBox->setText(tr("Arrow at &end"));
  endBox->setChecked(lm->hasEndArrow());
  gl1->addWidget(endBox, 3, 1);
  gl1->setRowStretch(4, 1);

  gb1->setLayout(gl1);

  QHBoxLayout *hl1 = new QHBoxLayout();
  hl1->addWidget(gb1);

  options = new QWidget();
  options->setLayout(hl1);

  tw = new QTabWidget();
  tw->addTab(options, tr("Opti&ons"));

  QGroupBox *gb2 = new QGroupBox();
  QGridLayout *gl2 = new QGridLayout();

  gl2->addWidget(new QLabel(tr("Length")), 0, 0);
  boxHeadLength = new QSpinBox();
  boxHeadLength->setValue(lm->headLength());
  gl2->addWidget(boxHeadLength, 0, 1);

  gl2->addWidget(new QLabel(tr("Angle")), 1, 0);
  boxHeadAngle = new QSpinBox();
  boxHeadAngle->setRange(0, 85);
  boxHeadAngle->setSingleStep(5);
  boxHeadAngle->setValue(lm->headAngle());
  gl2->addWidget(boxHeadAngle, 1, 1);

  filledBox = new QCheckBox();
  filledBox->setText(tr("&Filled"));
  filledBox->setChecked(lm->filledArrowHead());
  gl2->addWidget(filledBox, 2, 1);
  gl2->setRowStretch(3, 1);

  gb2->setLayout(gl2);

  QHBoxLayout *hl2 = new QHBoxLayout();
  hl2->addWidget(gb2);

  head = new QWidget();
  head->setLayout(hl2);
  tw->addTab(head, tr("Arrow &Head"));

  initGeometryTab();

  buttonDefault = new QPushButton(tr("Set &Default"));
  btnApply = new QPushButton(tr("&Apply"));
  btnOk = new QPushButton(tr("&Ok"));
  btnOk->setDefault(true);

  QBoxLayout *bl1 = new QBoxLayout(QBoxLayout::LeftToRight);
  bl1->addStretch();
  bl1->addWidget(buttonDefault);
  bl1->addWidget(btnApply);
  bl1->addWidget(btnOk);

  QVBoxLayout *vl = new QVBoxLayout();
  vl->addWidget(tw);
  vl->addLayout(bl1);
  setLayout(vl);

  enableHeadTab();

  connect(btnOk, SIGNAL(clicked()), this, SLOT(accept()));
  connect(btnApply, SIGNAL(clicked()), this, SLOT(apply()));
  connect(tw, SIGNAL(currentChanged(QWidget *)), this,
          SLOT(enableButtonDefault(QWidget *)));
  connect(buttonDefault, SIGNAL(clicked()), this, SLOT(setDefaultValues()));
}

void LineDialog::initGeometryTab() {
  if (unitBox == nullptr)
    unitBox = new QComboBox();
  unitBox->addItem(tr("Scale Coordinates"));
  unitBox->addItem(tr("Pixels"));

  QBoxLayout *bl1 = new QBoxLayout(QBoxLayout::LeftToRight);
  bl1->addWidget(new QLabel(tr("Unit")));
  bl1->addWidget(unitBox);

  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parent());
  QLocale locale = QLocale();
  if (app)
    locale = app->locale();

  QGroupBox *gb1 = new QGroupBox(tr("Start Point"));
  xStartBox = new DoubleSpinBox();
  xStartBox->setLocale(locale);
  xStartBox->setDecimals(6);
  yStartBox = new DoubleSpinBox();
  yStartBox->setLocale(locale);
  yStartBox->setDecimals(6);

  xStartPixelBox = new QSpinBox();
  xStartPixelBox->setRange(-INT_MAX, INT_MAX);
  yStartPixelBox = new QSpinBox();
  yStartPixelBox->setRange(-INT_MAX, INT_MAX);

  QGridLayout *gl1 = new QGridLayout();
  gl1->addWidget(new QLabel(tr("X")), 0, 0);
  gl1->addWidget(xStartBox, 0, 1);
  gl1->addWidget(xStartPixelBox, 0, 1);
  gl1->addWidget(new QLabel(tr("To")), 1, 0);
  gl1->addWidget(yStartBox, 1, 1);
  gl1->addWidget(yStartPixelBox, 1, 1);
  gl1->setColumnStretch(1, 10);
  gl1->setRowStretch(2, 1);
  gb1->setLayout(gl1);

  QGroupBox *gb2 = new QGroupBox(tr("End Point"));
  xEndBox = new DoubleSpinBox();
  xEndBox->setLocale(locale);
  xEndBox->setDecimals(6);
  yEndBox = new DoubleSpinBox();
  yEndBox->setLocale(locale);
  yEndBox->setDecimals(6);

  xEndPixelBox = new QSpinBox();
  xEndPixelBox->setRange(-INT_MAX, INT_MAX);
  yEndPixelBox = new QSpinBox();
  yEndPixelBox->setRange(-INT_MAX, INT_MAX);

  QGridLayout *gl2 = new QGridLayout();
  gl2->addWidget(new QLabel(tr("X")), 0, 0);
  gl2->addWidget(xEndBox, 0, 1);
  gl2->addWidget(xEndPixelBox, 0, 1);

  gl2->addWidget(new QLabel(tr("To")), 1, 0);
  gl2->addWidget(yEndBox, 1, 1);
  gl2->addWidget(yEndPixelBox, 1, 1);
  gl2->setColumnStretch(1, 10);
  gl2->setRowStretch(2, 1);
  gb2->setLayout(gl2);

  QBoxLayout *bl2 = new QBoxLayout(QBoxLayout::LeftToRight);
  bl2->addWidget(gb1);
  bl2->addWidget(gb2);

  QVBoxLayout *vl = new QVBoxLayout();
  vl->addLayout(bl1);
  vl->addLayout(bl2);

  geometry = new QWidget();
  geometry->setLayout(vl);
  tw->addTab(geometry, tr("&Geometry"));

  connect(unitBox, SIGNAL(activated(int)), this, SLOT(displayCoordinates(int)));
  displayCoordinates(0);
}

void LineDialog::displayCoordinates(int unit) {
  if (unit == ScaleCoordinates) {
    QwtDoublePoint sp = lm->startPointCoord();
    xStartBox->setValue(sp.x());
    xStartBox->show();
    xStartPixelBox->hide();
    yStartBox->setValue(sp.y());
    yStartBox->show();
    yStartPixelBox->hide();

    QwtDoublePoint ep = lm->endPointCoord();
    xEndBox->setValue(ep.x());
    xEndBox->show();
    xEndPixelBox->hide();
    yEndBox->setValue(ep.y());
    yEndBox->show();
    yEndPixelBox->hide();
  } else {
    QPoint startPoint = lm->startPoint();
    QPoint endPoint = lm->endPoint();

    xStartBox->hide();
    xStartPixelBox->setValue(startPoint.x());
    xStartPixelBox->show();

    yStartBox->hide();
    yStartPixelBox->setValue(startPoint.y());
    yStartPixelBox->show();

    xEndBox->hide();
    xEndPixelBox->setValue(endPoint.x());
    xEndPixelBox->show();

    yEndBox->hide();
    yEndPixelBox->setValue(endPoint.y());
    yEndPixelBox->show();
  }
}

void LineDialog::setCoordinates(int unit) {
  if (unit == ScaleCoordinates) {
    lm->setStartPoint(xStartBox->value(), yStartBox->value());
    lm->setEndPoint(xEndBox->value(), yEndBox->value());
  } else {
    lm->setStartPoint(QPoint(xStartPixelBox->value(), yStartPixelBox->value()));
    lm->setEndPoint(QPoint(xEndPixelBox->value(), yEndPixelBox->value()));
  }
}

void LineDialog::apply() {
  if (tw->currentWidget() == dynamic_cast<QWidget *>(options)) {
    lm->setStyle(Graph::getPenStyle(styleBox->currentIndex()));
    lm->setColor(colorBox->color());
    lm->setWidth(widthBox->value());
    lm->drawEndArrow(endBox->isChecked());
    lm->drawStartArrow(startBox->isChecked());
  } else if (tw->currentWidget() == dynamic_cast<QWidget *>(head)) {
    if (lm->headLength() != boxHeadLength->value())
      lm->setHeadLength(boxHeadLength->value());

    if (lm->headAngle() != boxHeadAngle->value())
      lm->setHeadAngle(boxHeadAngle->value());

    if (lm->filledArrowHead() != filledBox->isChecked())
      lm->fillArrowHead(filledBox->isChecked());
  } else if (tw->currentWidget() == dynamic_cast<QWidget *>(geometry))
    setCoordinates(unitBox->currentIndex());

  QwtPlot *plot = lm->plot();
  Graph *g = dynamic_cast<Graph *>(plot->parent());
  plot->replot();
  g->notifyChanges();

  enableHeadTab();
}

void LineDialog::accept() {
  apply();
  close();
}

void LineDialog::setLineStyle(Qt::PenStyle style) {
  if (style == Qt::SolidLine)
    styleBox->setCurrentIndex(0);
  else if (style == Qt::DashLine)
    styleBox->setCurrentIndex(1);
  else if (style == Qt::DotLine)
    styleBox->setCurrentIndex(2);
  else if (style == Qt::DashDotLine)
    styleBox->setCurrentIndex(3);
  else if (style == Qt::DashDotDotLine)
    styleBox->setCurrentIndex(4);
}

void LineDialog::enableHeadTab() {
  if (startBox->isChecked() || endBox->isChecked())
    tw->setTabEnabled(tw->indexOf(head), true);
  else
    tw->setTabEnabled(tw->indexOf(head), false);
}

void LineDialog::setDefaultValues() {
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parent());
  if (!app)
    return;

  app->setArrowDefaultSettings(widthBox->value(), colorBox->color(),
                               Graph::getPenStyle(styleBox->currentIndex()),
                               boxHeadLength->value(), boxHeadAngle->value(),
                               filledBox->isChecked());
}

void LineDialog::enableButtonDefault(QWidget *w) {
  if (w == geometry)
    buttonDefault->setEnabled(false);
  else
    buttonDefault->setEnabled(true);
}
