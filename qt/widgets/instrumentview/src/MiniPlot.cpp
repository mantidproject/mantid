#include "MantidQtWidgets/InstrumentView/MiniPlot.h"
#include "MantidQtWidgets/MplCpp/PythonErrors.h"
#include "MantidQtWidgets/Common/PythonThreading.h"

#include <QContextMenuEvent>
#include <QVBoxLayout>

#include <array>

#include "MantidQtWidgets/Common/QtCompat.h"

namespace {

/// Active curve format
const char *ACTIVE_CURVE_FORMAT = "k-";
/// Smallest positive cut-off value for plotting on a log axis.
const double SMALLEST_POSITIVE_VALUE = 0.5;

/**
 * Return a color for a stored line at the given index
 * @param index Index for a curve on the canvas. Indices higher than the
 * number of available colors will cause a cycle
 * @return Matplotlib color format string
 */
const char *storedLineColor(size_t index) {
  // See http://doc.qt.io/qt-5/qt.html#GlobalColor-enum for color definitions
  static std::array<const char *, 13> colors = {
      {"r", "g", "b", "c", "m", "y", "#a0a0a4", "#800000", "#000080", "#008080",
       "#800080", "#808000", "#808080"}};
  if (index >= colors.size()) {
    // cycle the color
    index = index % colors.size();
  }
  // cppcheck-suppress CastIntegerToAddressAtReturn
  return colors[index];
}
} // namespace

namespace MantidQt {
using Widgets::MplCpp::Axes;
using Widgets::MplCpp::MplFigureCanvas;
namespace MantidWidgets {

/**
 * Construct an empty plot
 * @param parent A widget to be the parent
 */
MiniPlot::MiniPlot(QWidget *parent)
    : MplFigureCanvas(111, parent), m_activeCurveLabel(), m_storedCurveLabels(),
      m_xunit() {
  setCanvasFaceColor("white");
  toggleZoomMode();
  const auto &font = parent->font();
  setTickLabelFontSize(Axes::Scale::Both, font.pointSizeF());
}

/**
 * Check if any curves have been stored
 * @return True if a curve has been stored, false otherwise
 */
bool MiniPlot::hasStoredCurves() const { return !m_storedCurveLabels.empty(); }

/**
 * @return A tuple of the current Y-axis limits
 */
std::tuple<double, double> MiniPlot::getYLimits() const {
  return limits(Axes::Scale::Y);
}

/**
 * Check if an active curve is present
 * @return True if an active curve is present
 */
bool MiniPlot::hasActiveCurve() const { return !m_activeCurveLabel.isEmpty(); }

/**
 * Redraw the canvas based on the current data
 */
void MiniPlot::update() {
  if (nlines() > 0) {
    rescaleToData(Axes::Scale::Both, true);
  } else {
    draw();
  }
}

/**
 * Set the given data as the active curve on the plot
 * @param x X axis data for the plot
 * @param y Y axis data for the plot
 * @param xunit String defining unit for the X axis
 * @param curveLabel A label for the curve
 */
void MiniPlot::setActiveCurve(std::vector<double> x, std::vector<double> y,
                              QString xunit, QString curveLabel) {
  std::transform(y.begin(), y.end(), y.begin(),
                 [](double v) { return v > 0 ? v : SMALLEST_POSITIVE_VALUE; });
  removeActiveCurve();
  plotLine(std::move(x), std::move(y), ACTIVE_CURVE_FORMAT);
  setLabel(Axes::Label::X, xunit.toAscii().data());
  m_activeCurveLabel = curveLabel;
  m_xunit = xunit;
}

/**
 * Set the given data as the active curve on the plot. See MiniPlotCurveData
 * for required data
 * @param data The data to be plotted
 */
void MiniPlot::setActiveCurve(MiniPlotCurveData data) {
  setActiveCurve(std::move(data.x), std::move(data.y), data.xunit, data.label);
}

/**
 * Deletes any active curve on the canvas
 */
void MiniPlot::removeActiveCurve() {
  // The active curve will always be the last thing that was added to
  // the canvas so we want to remove the index of the last curve, which
  // should always be the size of the number of stored curves.
  removeLine(m_storedCurveLabels.size());
  m_activeCurveLabel.clear();
  m_xunit.clear();
  if (!m_peakLabels.empty()) {
    ScopedPythonGIL gil;
    for (auto &label : m_peakLabels) {
      if (!label.isNone()) {
        label.callMethod("remove");
      }
    }
    m_peakLabels.clear();
  }
}

/**
 * Remove a curve with the given label.
 * @param label The label attached to the curve
 */
void MiniPlot::removeCurve(QString label) {
  auto lineIndex = m_storedCurveLabels.indexOf(label);
  if (lineIndex >= 0) {
    removeLine(static_cast<size_t>(lineIndex));
    m_storedCurveLabels.removeAt(lineIndex);
    update();
  }
}

/**
 * Add some text at the specified position
 * @param x X in data coordinates
 * @param y Y in data coordinates
 * @param label The text label to attach
 */
void MiniPlot::addPeakLabel(double x, double y, QString label) {
  m_peakLabels.append(addText(x, y, label.toAscii().constData(), "center"));
}

/**
 * If there is an active curve then store it internally if we don't already have
 * it stored. It also changes the line color of the stored curve
 */
void MiniPlot::storeCurve() {
  if (m_activeCurveLabel.isEmpty() ||
      m_storedCurveLabels.contains(m_activeCurveLabel))
    return;

  const size_t lineIndex = static_cast<size_t>(m_storedCurveLabels.size());
  // store label
  m_storedCurveLabels.insert(m_storedCurveLabels.end(), m_activeCurveLabel);
  m_activeCurveLabel.clear();
  // switch color
  setLineColor(lineIndex, storedLineColor(lineIndex));
}

/**
 * Switch the Y scale to linear
 */
void MiniPlot::setYScaleLinear() { setScale(Axes::Scale::Y, "linear", true); }

/**
 * Switch the Y scale to logarithmic
 */
void MiniPlot::setYScaleLog() { setScale(Axes::Scale::Y, "log", true); }

/**
 * Called by the base class when a context menu is requested. We handle this
 * with mouseReleaseEvent and ignore the event here.
 * @param evt A pointer to the event source
 */
void MiniPlot::contextMenuEvent(QContextMenuEvent *evt) { evt->ignore(); }

/**
 * Called by the base class when a mouse click is released
 * @param evt A pointer to the QMouseEvent source (unused)
 */
void MiniPlot::mouseReleaseEvent(QMouseEvent *evt) {
  if (evt->button() == Qt::LeftButton) {
    const auto dataPos = toDataCoordinates(evt->pos());
    emit clickedAtDataCoord(dataPos.x(), dataPos.y());
    evt->accept();
  }
  if (evt->button() == Qt::RightButton) {
    if (isZoomed()) {
      home();
    } else {
      emit contextMenuRequested(evt->globalPos());
    }
    evt->accept();
  }
}
} // namespace MantidWidgets
} // namespace MantidQt
