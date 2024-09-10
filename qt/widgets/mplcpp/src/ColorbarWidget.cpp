// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/ColorbarWidget.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/MantidColorMap.h"

#include <QComboBox>
#include <QDoubleValidator>
#include <QLineEdit>
#include <QVBoxLayout>

using Mantid::PythonInterface::GlobalInterpreterLock;
using namespace MantidQt::Widgets::Common;

namespace MantidQt::Widgets::MplCpp {

namespace {
// These values control the dimensions of the axes used
// to hold the colorbar. The aspect ratio is set to give
// the usual long thin colobar
constexpr double AXES_LEFT = 0.4;
constexpr double AXES_BOTTOM = 0.05;
constexpr double AXES_WIDTH = 0.2;
constexpr double AXES_HEIGHT = 0.9;

// Background color for figure
constexpr const char *FIGURE_FACECOLOR = "w";

// Define the available normalization option labels and tooltips
// The order defines the order in the combo box. The index
// values is used as an integer representation. See the setScaleType
// method if this is changed.
QStringList NORM_OPTS = {"Linear", "SymmetricLog10", "Power"};

} // namespace

/**
 * @brief Construct a default color bar with a linear scale. The default limits
 * are set to [0, 1] so setRange would need to be called at a minimum
 * @param parent A pointer to the parent widget
 */
ColorbarWidget::ColorbarWidget(QWidget *parent)
    : QWidget(parent), m_ui(), m_mappable(Normalize(0, 1), getCMap(defaultCMapName())),
      m_eventFilter(std::make_unique<FigureEventFilter>()) {
  initLayout();
  connectSignals();
  m_canvas->installEventFilterToMplCanvas(m_eventFilter.get());
}

/**
 * Set the normalization instance
 * @param norm An instance of NormalizeBase. See Colors.h
 */
void ColorbarWidget::setNorm(const NormalizeBase &norm) {
  m_mappable.setNorm(norm);
  // matplotlib requires creating a brand new colorbar if the
  // normalization type changes
  createColorbar(norm.tickLocator(), norm.labelFormatter());
  m_canvas->draw();
}

/**
 * Update the range of the scale
 * @param vmin An optional new minimum of the scale
 * @param vmax An optional new maximum of the scale
 */
void ColorbarWidget::setClim(std::optional<double> vmin, std::optional<double> vmax) {
  m_mappable.setClim(vmin, vmax);
  m_canvas->draw();

  if (vmin.has_value()) {
    m_ui.scaleMinEdit->setText(QString::number(vmin.value()));
    emit minValueChanged(vmin.value());
  }
  if (vmax.has_value()) {
    m_ui.scaleMaxEdit->setText(QString::number(vmax.value()));
    emit maxValueChanged(vmax.value());
  }
}

/**
 * @return A tuple giving the current colorbar scale limits
 */
std::tuple<double, double> ColorbarWidget::clim() const {
  return std::make_tuple<double, double>(m_ui.scaleMinEdit->text().toDouble(), m_ui.scaleMaxEdit->text().toDouble());
}

/**
 * @brief Called to setup the widget based on the MantidColorMap instance
 * @param mtdCMap A reference to the MantidColorMap wrapper
 */
void ColorbarWidget::setupColorBarScaling(const MantidColorMap &mtdCMap) {
  // Sync the colormap first as resetting the scale type forces a redraw
  // anyway
  m_mappable.setCmap(mtdCMap.cmap());
  // block signals to avoid infinite loop while setting scale type
  this->blockSignals(true);
  setScaleType(static_cast<int>(mtdCMap.getScaleType()));
  this->blockSignals(false);
}

// ------------------------------ Legacy API -----------------------------------

/**
 * Update the minimum value of the normalization scale
 * @param vmin New minimum of the scale
 */
void ColorbarWidget::setMinValue(double vmin) { setClim(vmin, std::nullopt); }

/**
 * Update the maximum value of the normalization scale
 * @param vmin New maximum of the scale
 */
void ColorbarWidget::setMaxValue(double vmax) { setClim(std::nullopt, vmax); }

/**
 * @return The minimum color scale value as a string
 */
QString ColorbarWidget::getMinValue() const { return QString::number(std::get<0>(clim())); }

/**
 * @return The maximum color scale value as a string
 */
QString ColorbarWidget::getMaxValue() const { return QString::number(std::get<1>(clim())); }

/**
 * @return The power value as a string
 */
QString ColorbarWidget::getNthPower() const { return m_ui.powerEdit->text(); }

/**
 * @return The scale type choice as an integer.
 */
int ColorbarWidget::getScaleType() const { return m_ui.normTypeOpt->currentIndex(); }

/**
 * @brief Set the scale type from an integer representation
 * Linear=0, Log=1, Power=2, which is backwards compatible
 * with the original Qwt version
 * @param index The scale type as an integer
 */
void ColorbarWidget::setScaleType(int index) {
  // Protection against a bad index.
  if (index < 0 || index > 2)
    return;
  // Some ranges will be invalid for some scale types, e.g. x < 0 for PowerNorm.
  // Compute a valid range and reset user-specified range if necessary
  auto autoscaleAndSetNorm = [this](auto norm) {
    auto validRange = norm.autoscale(clim());
    setNorm(std::move(norm));
    return validRange;
  };

  std::tuple<double, double> validRange;
  switch (index) {
  case 0:
    validRange = autoscaleAndSetNorm(Normalize());
    break;
  case 1:
    validRange = autoscaleAndSetNorm(SymLogNorm(SymLogNorm::DefaultLinearThreshold, SymLogNorm::DefaultLinearScale));
    break;
  case 2:
    validRange = autoscaleAndSetNorm(PowerNorm(getNthPower().toDouble()));
    break;
  }
  setClim(std::get<0>(validRange), std::get<1>(validRange));
  m_ui.normTypeOpt->setCurrentIndex(index);
  emit scaleTypeChanged(index);
}

/**
 * @brief Set the power for the power scale
 * @param gamma The value of the exponent
 */
void ColorbarWidget::setNthPower(double gamma) {
  if (gamma == 0) {
    // A power can not be 0.
    throw std::runtime_error("Power can not be 0");
  }
  m_ui.powerEdit->setText(QString::number(gamma));
  auto range = clim();
  setNorm(PowerNorm(gamma, std::get<0>(range), std::get<1>(range)));
  emit nthPowerChanged(gamma);
}

// --------------------------- Private slots -----------------------------------
/**
 * Called when a user has edited the minimum scale value
 */
void ColorbarWidget::scaleMinimumEdited() {
  // The validator ensures the text is a double
  const double value = m_ui.scaleMinEdit->text().toDouble();
  emit minValueEdited(value);
  setClim(value, std::nullopt);
}

/**
 * Called when a user has edited the maximum scale value
 */
void ColorbarWidget::scaleMaximumEdited() {
  // The validator ensures the text is a double
  const double value = m_ui.scaleMaxEdit->text().toDouble();
  emit maxValueEdited(value);
  setClim(std::nullopt, value);
}

/**
 * Called when a new selection in the scale type box is made
 */
void ColorbarWidget::scaleTypeSelectionChanged(int index) {
  if (index == 2) { // Power
    m_ui.powerEdit->show();
  } else {
    m_ui.powerEdit->hide();
  }
  m_ui.normTypeOpt->blockSignals(true);
  setScaleType(index);
  m_ui.normTypeOpt->blockSignals(false);
}

/**
 * Called when the power exponent input has been edited
 */
void ColorbarWidget::powerExponentEdited() {
  setScaleType(2);
  // power edit has double validator so this should always be valid
  emit nthPowerChanged(m_ui.powerEdit->text().toDouble());
}

// --------------------------- Private methods --------------------------------

/**
 * Setup the layout of the child widgets
 */
void ColorbarWidget::initLayout() {
  // Create colorbar (and figure if necessary)
  Figure fig{false};
  fig.setFaceColor(FIGURE_FACECOLOR);
  m_ui.setupUi(this);
  // remove placeholder widget and add figure canvas
  delete m_ui.mplColorbar;
  m_canvas = new FigureCanvasQt(std::move(fig), this);
  m_ui.mplColorbar = m_canvas;
  m_ui.verticalLayout->insertWidget(1, m_canvas);
  createColorbar();

  // Set validators on the scale inputs
  m_ui.scaleMinEdit->setValidator(new QDoubleValidator());
  m_ui.scaleMaxEdit->setValidator(new QDoubleValidator());
  m_ui.powerEdit->setValidator(new QDoubleValidator());
  // Setup normalization options
  m_ui.normTypeOpt->addItems(NORM_OPTS);
  scaleTypeSelectionChanged(0);
}

/**
 * (Re)-create a colorbar around the current mappable. It assumes the figure
 * and canvas have been created
 * @param ticks An optional matplotlib.ticker.*Locator object. Default=None to
 * autoselect the most appropriate
 * @param format An optional matplotlib.ticker.*Format object. Default=None to
 * autoselect the most appropriate
 */
void ColorbarWidget::createColorbar(const Python::Object &ticks, const Python::Object &format) {
  assert(m_canvas);
  GlobalInterpreterLock lock;
  auto cb = Python::Object(m_mappable.pyobj().attr("colorbar"));
  if (!cb.is_none()) {
    cb.attr("remove")();
  }
  // create the new one
  auto fig = m_canvas->gcf();
  Axes cbAxes{fig.addAxes(AXES_LEFT, AXES_BOTTOM, AXES_WIDTH, AXES_HEIGHT)};
  fig.colorbar(m_mappable, cbAxes, ticks, format);
}

/**
 * Wire up the signals for the child widgets
 */
void ColorbarWidget::connectSignals() {
  connect(m_ui.scaleMinEdit, SIGNAL(editingFinished()), this, SLOT(scaleMinimumEdited()));
  connect(m_ui.scaleMaxEdit, SIGNAL(editingFinished()), this, SLOT(scaleMaximumEdited()));

  connect(m_ui.normTypeOpt, SIGNAL(currentIndexChanged(int)), this, SLOT(scaleTypeSelectionChanged(int)));
  connect(m_ui.powerEdit, SIGNAL(editingFinished()), this, SLOT(powerExponentEdited()));
}

} // namespace MantidQt::Widgets::MplCpp
