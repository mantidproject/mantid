#include "MantidQtWidgets/InstrumentView/MiniPlotMpl.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

#include "MantidKernel/Logger.h"

#include <QVBoxLayout>

namespace {
// Active curve format
const char *ACTIVE_CURVE_FORMAT = "k-";
// Logger
Mantid::Kernel::Logger g_log("MiniPlotQwt");

/**
 * Check if size(X)==size(Y) and both are not empty
 * @param x A reference to the X data vector
 * @param y A reference to the Y data vector
 * @return True if a warning was produced, false otherwise
 */
bool warnDataInvalid(const std::vector<double> &x,
                     const std::vector<double> &y) {
  if (x.size() != y.size()) {
    g_log.warning(std::string(
        "setData(): X/Y size mismatch! X=" + std::to_string(x.size()) +
        ", Y=" + std::to_string(y.size())));
    return true;
  }
  if (x.empty()) {
    g_log.warning("setData(): X & Y arrays are empty!");
    return true;
  }
  return false;
}

} // namespace

namespace MantidQt {
using Widgets::MplCpp::FigureCanvasQt;

namespace MantidWidgets {

/**
 * Construct a blank miniplot with a single subplot
 * @param parent A pointer to its parent widget
 */
MiniPlotMpl::MiniPlotMpl(QWidget *parent)
    : QWidget(parent), m_canvas(new FigureCanvasQt(111)), m_lines(), m_xunit(),
      m_activeCurveLabel() {
  setLayout(new QVBoxLayout);
  layout()->addWidget(m_canvas);
}

void MiniPlotMpl::setData(std::vector<double> x, std::vector<double> y,
                          QString xunit, QString curveLabel) {
  if (warnDataInvalid(x, y))
    return;

  m_lines.clear();
  auto axes = m_canvas->gca();
  m_lines.emplace_back(
      axes.plot(std::move(x), std::move(y), ACTIVE_CURVE_FORMAT));
  m_xunit = std::move(xunit);
  m_activeCurveLabel = curveLabel;
  axes.setXLabel(m_xunit.toLatin1().constData());
  axes.relim();
  axes.autoscaleView();
  replot();
}

/**
 * Redraws the canvas
 */
void MiniPlotMpl::replot() { m_canvas->draw(); }

/**
 * Remove the active curve, keeping any stored curves
 */
void MiniPlotMpl::clearCurve() {
  m_lines.clear();
  m_activeCurveLabel.clear();
}

/**
 * Clear all artists from the canvas
 */
void MiniPlotMpl::clearAll() {
  m_lines.clear();
  replot();
}

} // namespace MantidWidgets
} // namespace MantidQt
