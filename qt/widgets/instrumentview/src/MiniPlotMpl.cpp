#include "MantidQtWidgets/InstrumentView/MiniPlotMpl.h"
#include "MantidQtWidgets/MplCpp/ColorConverter.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

#include "MantidKernel/Logger.h"
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QVBoxLayout>

namespace {
const char *ACTIVE_CURVE_FORMAT = "k-";
const char *STORED_LINE_COLOR_CYCLE = "bgrcmyk";
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
using Widgets::MplCpp::ColorConverter;
using Widgets::MplCpp::FigureCanvasQt;
using Widgets::MplCpp::cycler;

namespace MantidWidgets {

/**
 * Construct a blank miniplot with a single subplot
 * @param parent A pointer to its parent widget
 */
MiniPlotMpl::MiniPlotMpl(QWidget *parent)
    : QWidget(parent), m_canvas(new FigureCanvasQt(111)), m_lines(),
      m_colorCycler(cycler("color", STORED_LINE_COLOR_CYCLE)), m_xunit(),
      m_activeCurveLabel(), m_storedCurveLabels() {
  setLayout(new QVBoxLayout);
  layout()->addWidget(m_canvas);
  m_canvas->installEventFilterToMplCanvas(this);
}

void MiniPlotMpl::setData(std::vector<double> x, std::vector<double> y,
                          QString xunit, QString curveLabel) {
  if (warnDataInvalid(x, y))
    return;

  clearCurve();
  auto axes = m_canvas->gca();
  axes.relim();
  // plot automatically calls "scalex=True, scaley=True"
  m_lines.emplace_back(
      axes.plot(std::move(x), std::move(y), ACTIVE_CURVE_FORMAT));
  m_xunit = std::move(xunit);
  m_activeCurveLabel = curveLabel;
  axes.setXLabel(m_xunit.toLatin1().constData());
  replot();
}

/**
 * @return True if an active curve exists
 */
bool MiniPlotMpl::hasCurve() const { return !m_activeCurveLabel.isEmpty(); }

/**
 * Store the active curve so it is not overridden
 * by future plotting. The curve's color is updated using the color cycler
 */
void MiniPlotMpl::store() {
  m_storedCurveLabels.append(m_activeCurveLabel);
  m_activeCurveLabel.clear();
  m_lines.back().set(m_colorCycler());
}

/**
 * @return True if the plot has stored curves
 */
bool MiniPlotMpl::hasStored() const { return !m_storedCurveLabels.isEmpty(); }

/**
 * Remove the stored curve with the given label. If the label is not found this
 * does nothing
 * @param label A string label for a curve
 */
void MiniPlotMpl::removeCurve(const QString &label) {
  auto labelIndex = m_storedCurveLabels.indexOf(label);
  if (labelIndex < 0)
    return;
  // The line is at the same position in the vector
  m_lines.erase(std::next(std::begin(m_lines), labelIndex));
}

/**
 * Retrieve the color of the curve with the given label
 * @param label
 * @return A QColor defining the color of the curve
 */
QColor MiniPlotMpl::getCurveColor(const QString &label) const {
  auto labelIndex = m_storedCurveLabels.indexOf(label);
  if (labelIndex < 0)
    return QColor();
  return ColorConverter::toRGB(m_lines[labelIndex].pyobj().attr("get_color")());
}

/**
 * Redraws the canvas
 */
void MiniPlotMpl::replot() { m_canvas->draw(); }

/**
 * Remove the active curve, keeping any stored curves
 */
void MiniPlotMpl::clearCurve() {
  // setData places the latest curve at the back of the vector
  if (hasCurve()) {
    m_lines.pop_back();
  }
  m_activeCurveLabel.clear();
}

/**
 * Clear all artists from the canvas
 */
void MiniPlotMpl::clearAll() {
  m_lines.clear();
  replot();
}

/**
 * Override the contextMenuEvent handler. Ignores the events as they are handled
 * by mousePressEvent as right click also controls zoom level
 * @param evt A pointer to a QContextMenuEvent describing the event
 */
void MiniPlotMpl::contextMenuEvent(QContextMenuEvent *evt) { evt->accept(); }

/**
 * Filter events from the underlying matplotlib canvas
 * @param watched A pointer to the object being watched
 * @param evt A pointer to the generated event
 * @return True if the event was filtered, false otherwise
 */
bool MiniPlotMpl::eventFilter(QObject *watched, QEvent *evt) {
  Q_UNUSED(watched);
  bool filtered{false};
  switch (evt->type()) {
  case QEvent::MouseButtonPress:
    mousePressEvent(static_cast<QMouseEvent *>(evt));
    filtered = true;
    break;
  default:
    filtered = false;
  }

  return filtered;
}

/**
 * Override the contextMenuEvent handler. Ignores the events as they are handled
 * by mousePressEvent as right click also controls zoom level
 * @param evt A pointer to a QMouseEvent describing the event
 */
void MiniPlotMpl::mousePressEvent(QMouseEvent *evt) {
  if (evt->buttons() & Qt::RightButton) {
    evt->accept();
    // plot owner will display and process context menu
    emit showContextMenu();
  }
}


} // namespace MantidWidgets
} // namespace MantidQt
