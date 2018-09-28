#include "MantidQtWidgets/MplCpp/ColorbarWidget.h"
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>

#include <climits>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
// These values control the dimensions of the axes used
// to hold the colorbar. The aspect ratio is set to give
// the usual long thin colobar
constexpr double AXES_LEFT = 0.4;
constexpr double AXES_BOTTOM = 0.05;
constexpr double AXES_WIDTH = 0.2;
constexpr double AXES_HEIGHT = 0.9;
} // namespace

/**
 * @brief Construct a default color bar with a linear scale. The default limits
 * are set to [0, 1] so setRange would need to be called at a minimum
 * @param parent A pointer to the parent widget
 */
ColorbarWidget::ColorbarWidget(QWidget *parent)
    : QWidget(parent), m_ui(),
      m_mappable(Normalize(0, 1), getCMap(defaultCMapName())), m_colorbar() {
  // Create figure and colorbar
  Figure fig{false};
  fig.pyobj().attr("set_facecolor")("w");
  Axes cbAxes{fig.addAxes(AXES_LEFT, AXES_BOTTOM, AXES_WIDTH, AXES_HEIGHT)};
  m_colorbar = fig.colorbar(m_mappable, cbAxes);

  // Create widget layout including canvas to draw the figure
  m_ui.setupUi(this);
  // remove placeholder widget and add figure canvas
  delete m_ui.mplColorbar;
  m_ui.mplColorbar = new FigureCanvasQt(fig, this);
  m_ui.verticalLayout->insertWidget(1, m_ui.mplColorbar);
}

/**
 * Update the ange of the scale
 * @param vmin An optional new minimum of the scale
 * @param vmax An optional new maximum of the scale
 */
void ColorbarWidget::setRange(boost::optional<double> vmin,
                              boost::optional<double> vmax) {
  m_mappable.setCLim(vmin, vmax);
  if (vmin.is_initialized()) {

    m_ui.scaleMinEdit->setText(QString::number(vmin.get()));
    emit minValueChanged(vmin.get());
  }
  if (vmax.is_initialized()) {
    m_ui.scaleMaxEdit->setText(QString::number(vmax.get()));
    emit maxValueChanged(vmax.get());
  }
} // namespace MplCpp

/**
 * Update the minimum value of the normalization scale
 * @param vmin New minimum of the scale
 */
void ColorbarWidget::setMinValue(double vmin) { setRange(vmin, boost::none); }

/**
 * Update the maximum value of the normalization scale
 * @param vmin New maximum of the scale
 */
void ColorbarWidget::setMaxValue(double vmax) { setRange(boost::none, vmax); }

QString ColorbarWidget::getMinValue() const {
  return QString::number(PyFloat_AsDouble(
      Python::Object(m_colorbar.attr("mappable").attr("norm").attr("vmin"))
          .ptr()));
}

QString ColorbarWidget::getMaxValue() const {
  return QString::number(PyFloat_AsDouble(
      Python::Object(m_colorbar.attr("mappable").attr("norm").attr("vmax"))
          .ptr()));
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
