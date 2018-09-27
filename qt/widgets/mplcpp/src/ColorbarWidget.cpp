#include "MantidQtWidgets/MplCpp/ColorbarWidget.h"
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

#include "MantidPythonInterface/core/ErrorHandling.h"

#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>

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
 * @brief Constructor to specify the scale type as an integer
 * @param type An integer (0=Linear, 1=Log10, 2=Power)
 * @param parent A pointer to the parent widget
 */
ColorbarWidget::ColorbarWidget(int type, QWidget *parent)
    : QWidget(parent), m_ui(), m_colorbar() {
  // Create figure and colorbar
  Figure fig{false};
  fig.pyobj().attr("set_facecolor")("w");
  Axes cbAxes{fig.addAxes(AXES_LEFT, AXES_BOTTOM, AXES_WIDTH, AXES_HEIGHT)};
  m_colorbar = fig.colorbar(
      ScalarMappable{Normalize(-1.0, 1.0), getCMap("viridis")}, cbAxes);

  // Create widget layout including canvas to draw the figure
  m_ui.setupUi(this);
  // remove placeholder widget and add figure canvas
  delete m_ui.mplColorbar;
  m_ui.mplColorbar = new FigureCanvasQt(fig, this);
  m_ui.verticalLayout->insertWidget(1, m_ui.mplColorbar);
}

/**
 * Update the minimum value of the normalization scale
 * @param vmin New minimum of the scale
 */
void ColorbarWidget::setMinValue(double vmin) {
  m_colorbar.attr("mappable").attr("norm").attr("vmin") = vmin;
}

/**
 * Update the maximum value of the normalization scale
 * @param vmin New maximum of the scale
 */
void ColorbarWidget::setMaxValue(double vmax) {
  m_colorbar.attr("mappable").attr("norm").attr("vmax") = vmax;
}

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
