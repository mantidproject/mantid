#include "MantidQtWidgets/InstrumentView/MiniPlot.h"

#include "MantidQtWidgets/MplCpp/MplPlotWidget.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QVBoxLayout>

namespace {

/// Active curve format
const char *ACTIVE_CURVE_FORMAT = "k-";

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
  return colors[index];
}
}

namespace MantidQt {
using Widgets::MplCpp::Axes;
using Widgets::MplCpp::MplFigureCanvas;
namespace MantidWidgets {

/**
 * Construct an empty plot
 * @param parent A widget to be the parent
 */
MiniPlot::MiniPlot(QWidget *parent)
    : QWidget(parent), m_canvas(new MplFigureCanvas(111, this)),
      m_activeCurveLabel(), m_storedCurveLabels() {
  setLayout(new QVBoxLayout);
  layout()->addWidget(m_canvas);
}

/**
 * Check if any curves have been stored
 * @return True if a curve has been stored, false otherwise
 */
bool MiniPlot::hasStoredCurves() const { return !m_storedCurveLabels.empty(); }

/**
 * Check if an active curve is present
 * @return True if an active curve is present
 */
bool MiniPlot::hasActiveCurve() const { return !m_activeCurveLabel.isEmpty(); }

/**
 * Redraw the canvas based on the current data
 */
void MiniPlot::update() {
  m_canvas->rescaleToData(Axes::Scale::Both);
  m_canvas->draw();
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
  removeActiveCurve();
  m_canvas->plotLine(std::move(x), std::move(y), ACTIVE_CURVE_FORMAT);
  m_canvas->setLabel(Axes::Label::X, xunit.toAscii().data());
  m_activeCurveLabel = curveLabel;
}

/**
 * Set the given data as the active curve on the plot. See MiniPlotCurveData
 * for required data
 */
void MiniPlot::setActiveCurve(MiniPlotCurveData data) {
  removeActiveCurve();
  m_canvas->plotLine(std::move(data.x), std::move(data.y), ACTIVE_CURVE_FORMAT);
  m_canvas->setLabel(Axes::Label::X, data.xunit.toAscii().data());
  m_activeCurveLabel = data.label;
}

/**
 * Deletes any active curve on the canvas
 */
void MiniPlot::removeActiveCurve() {
  // The active curve will always be the last thing that was added to
  // the canvas so we want to remove the index of the last curve, which
  // should always be the size of the number of stored curves.
  m_canvas->removeLine(m_storedCurveLabels.size());
  m_activeCurveLabel.clear();
}

/**
 * Respond to a context menu request
 * @param event A pointer to the defined context menu event
 */
void MiniPlot::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu(this);
  menu.exec(event->globalPos());
}

/**
 * If there is an active curve then store it internally. It also changes the
 * line color of the stored curve
 */
void MiniPlot::storeCurve() {
  const size_t lineIndex = static_cast<size_t>(m_storedCurveLabels.size());
  // store label
  m_storedCurveLabels.insert(m_storedCurveLabels.end(), m_activeCurveLabel);
  m_activeCurveLabel.clear();
  // switch color
  m_canvas->setLineColor(lineIndex, storedLineColor(lineIndex));
}
}
}
