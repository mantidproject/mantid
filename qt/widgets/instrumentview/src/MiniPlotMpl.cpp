// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/MiniPlotMpl.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/VersionCompat.h"
#include "MantidQtWidgets/InstrumentView/PeakMarker2D.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

#include "MantidKernel/Logger.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QGridLayout>
#include <QIcon>
#include <QMouseEvent>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <utility>

using Mantid::PythonInterface::GlobalInterpreterLock;
using MantidQt::Widgets::MplCpp::cycler;
using MantidQt::Widgets::MplCpp::FigureCanvasQt;

namespace {
const char *ACTIVE_CURVE_FORMAT = "k-";
const char *STORED_LINE_COLOR_CYCLE = "bgrcmyk";
const char *LIN_SCALE_NAME = "linear";
const char *LOG_SCALE_NAME = "symlog";
Mantid::Kernel::Logger g_log("MiniPlotMpl");

QPushButton *createHomeButton() {
  using MantidQt::Widgets::Common::Python::NewRef;
  using MantidQt::Widgets::Common::Python::Object;

  auto mpl(NewRef(PyImport_ImportModule("matplotlib")));
  QDir dataPath(TO_CSTRING(Object(mpl.attr("get_data_path")()).ptr()));
  dataPath.cd("images");
  QIcon icon(dataPath.absoluteFilePath("home.png"));
  auto iconSize(icon.availableSizes().front());
  auto button = new QPushButton(icon, "");
  button->setMaximumSize(iconSize + QSize(5, 5));
  return button;
}

/**
 * Check if size(X)==size(Y) and both are not empty
 * @param x A reference to the X data vector
 * @param y A reference to the Y data vector
 * @return True if a warning was produced, false otherwise
 */
bool warnDataInvalid(const std::vector<double> &x, const std::vector<double> &y) {
  if (x.size() != y.size()) {
    g_log.warning(
        std::string("setData(): X/Y size mismatch! X=" + std::to_string(x.size()) + ", Y=" + std::to_string(y.size())));
    return true;
  }
  if (x.empty()) {
    g_log.warning("setData(): X & Y arrays are empty!");
    return true;
  }
  return false;
}

} // namespace

namespace MantidQt::MantidWidgets {

/**
 * Construct a blank miniplot with a single subplot
 * @param parent A pointer to its parent widget
 */
MiniPlotMpl::MiniPlotMpl(QWidget *parent)
    : QWidget(parent), m_canvas(new FigureCanvasQt(111)), m_homeBtn(createHomeButton()), m_lines(), m_peakLabels(),
      m_colorCycler(cycler("color", STORED_LINE_COLOR_CYCLE)), m_xunit(), m_activeCurveLabel(), m_storedCurveLabels(),
      m_zoomTool(m_canvas), m_mousePressPt() {
  auto plotLayout = new QGridLayout(this);
  plotLayout->setContentsMargins(0, 0, 0, 0);
  plotLayout->setSpacing(0);
  // We intentionally place the canvas and home button in the same location
  // in the grid layout so that they overlap and take up less space.
  plotLayout->addWidget(m_canvas, 0, 0);
  plotLayout->addWidget(m_homeBtn, 0, 0, Qt::AlignLeft | Qt::AlignBottom);
  setLayout(plotLayout);
  // Avoid squishing the plot to the point mpl outputs loads of warnings about the tight layout
  setMinimumSize(50, 125);

  // Capture mouse events destined for the plot canvas
  m_canvas->installEventFilterToMplCanvas(this);
  // Mouse events cause zooming by default. See mouseReleaseEvent
  // for exceptions
  m_zoomTool.enableZoom(true);
  connect(m_homeBtn, SIGNAL(clicked()), this, SLOT(zoomOutOnPlot()));
}

/**
 * Set data and metadata for a new curve
 * @param x The X-axis data
 * @param y The Y-axis data
 * @param xunit The X unit a label
 * @param curveLabel A label for the curve data
 */
void MiniPlotMpl::setData(std::vector<double> x, std::vector<double> y, QString xunit, QString curveLabel) {
  if (warnDataInvalid(x, y))
    return;

  clearCurve();
  auto axes = m_canvas->gca();
  // plot automatically calls "scalex=True, scaley=True"
  m_lines.emplace_back(axes.plot(std::move(x), std::move(y), ACTIVE_CURVE_FORMAT));
  m_activeCurveLabel = std::move(curveLabel);
  setXLabel(std::move(xunit));
  // If the current axis limits can fit the data then matplotlib
  // won't change the axis scale. If the intensity of different plots
  // is very different we need ensure the scale is tight enough to
  // see newer plots so we force a recalculation from the data
  axes.relim();
  axes.autoscaleView();
  replot();
}

/**
 * Se the X unit label on the axis
 * @param xunit A string giving the X unit
 */
void MiniPlotMpl::setXLabel(QString xunit) {
  m_canvas->gca().setXLabel(xunit.toLatin1().constData());
  m_xunit = std::move(xunit);
}

/**
 * Add a label to mark a peak to the plot
 * @param peakMarker A pointer to a PeakMarker2D object defining
 * the marker added to the instrument widget
 */
void MiniPlotMpl::addPeakLabel(const PeakMarker2D *peakMarker) {
  if (m_xunit.isEmpty())
    return;
  const auto &peak = peakMarker->getPeak();
  double peakX(0.0);
  if (m_xunit == "dSpacing") {
    peakX = peak.getDSpacing();
  } else if (m_xunit == "Wavelength") {
    peakX = peak.getWavelength();
  } else {
    peakX = peak.getTOF();
  }
  // Place the label in axes coordinates on the Y axis but data coordinates on the X axis.
  // Uses the transform to accomplish this.
  // This keeps the label visible as you zoom. Arbitrarily picked value.
  const double axesY(0.95);
  const QString label(peakMarker->getLabel());
  auto axes = m_canvas->gca();
  m_peakLabels.emplace_back(axes.text(peakX, axesY, label, "center", axes.getXAxisTransform()));
  m_peakLabels.back().set("clip_on", true);
}

/**
 * Clear all peak labels from the canvas
 */
void MiniPlotMpl::clearPeakLabels() {
  for (auto &label : m_peakLabels) {
    label.remove();
  }
  m_peakLabels.clear();
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
  // lock required when the dict returned by cycler iterator
  // is destroyed
  GlobalInterpreterLock lock;
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
  m_storedCurveLabels.removeAt(labelIndex);
  m_lines.erase(std::next(std::begin(m_lines), labelIndex));
  m_canvas->gca().relim();
  m_canvas->gca().autoscaleView();
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
  auto lineIter = std::next(std::begin(m_lines), labelIndex);
  return lineIter->getColor();
}

/**
 * @return True if the Y scale is logarithmic
 */
bool MiniPlotMpl::isYLogScale() const { return m_canvas->gca().getYScale() == LOG_SCALE_NAME; }

/**
 * Redraws the canvas only if in the GUI event loop. This avoids a crash when calling draw() on a plot which is on a
 * different tab to the currently selected tab.
 */
void MiniPlotMpl::replot() { m_canvas->drawIdle(); }

/**
 * Remove the active curve, keeping any stored curves
 */
void MiniPlotMpl::clearCurve() {
  // setData places the latest curve at the back of the vector
  if (hasCurve()) {
    m_lines.pop_back();
  }
  m_activeCurveLabel.clear();
  clearPeakLabels();
}

/**
 * Set the Y scale to logarithmic
 */
void MiniPlotMpl::setYLogScale() { m_canvas->gca().setYScale(LOG_SCALE_NAME); }

/**
 * Set the Y scale to linear
 */
void MiniPlotMpl::setYLinearScale() { m_canvas->gca().setYScale(LIN_SCALE_NAME); }

/**
 * Clear all artists from the canvas
 */
void MiniPlotMpl::clearAll() {
  // active curve, labels etc
  clearCurve();
  // any stored curves and labels
  m_lines.clear();
  m_storedCurveLabels.clear();
  replot();
}

/**
 * Filter events from the underlying matplotlib canvas
 * @param watched A pointer to the object being watched
 * @param evt A pointer to the generated event
 * @return True if the event was filtered, false otherwise
 */
bool MiniPlotMpl::eventFilter(QObject *watched, QEvent *evt) {
  Q_UNUSED(watched);
  bool stopEvent{false};
  switch (evt->type()) {
  case QEvent::ContextMenu:
    // handled by mouse press events below as we need to
    // stop the canvas getting mouse events in some circumstances
    // to disable zooming
    stopEvent = true;
    break;
  case QEvent::MouseButtonPress:
    stopEvent = handleMousePressEvent(static_cast<QMouseEvent *>(evt));
    break;
  case QEvent::MouseButtonRelease:
    stopEvent = handleMouseReleaseEvent(static_cast<QMouseEvent *>(evt));
    break;
  default:
    break;
  }
  return stopEvent;
}

/**
 * Handler called when the event filter recieves a mouse press event
 * @param evt A pointer to the event
 * @return True if the event propagation should be stopped, false otherwise
 */
bool MiniPlotMpl::handleMousePressEvent(QMouseEvent *evt) {
  bool stopEvent(false);
  // right-click events are reserved for the context menu
  // show when the mouse click is released
  if (evt->buttons() & Qt::LeftButton) {
    m_mousePressPt = evt->pos();
  } else if (evt->buttons() & Qt::RightButton) {
    m_mousePressPt = QPoint();
    stopEvent = true;
  }
  return stopEvent;
}

/**
 * Handler called when the event filter recieves a mouse release event
 * @param evt A pointer to the event
 * @return True if the event propagation should be stopped, false otherwise
 */
bool MiniPlotMpl::handleMouseReleaseEvent(QMouseEvent *evt) {
  bool stopEvent(false);
  if (evt->button() == Qt::LeftButton) {
    auto mouseReleasePt = evt->pos();
    // A click and release at the same point implies picking on the canvas
    // and not a zoom so stop matplotlib getting hold it it
    if (mouseReleasePt == m_mousePressPt) {
      const auto dataCoords = m_canvas->toDataCoords(mouseReleasePt);
      emit clickedAt(dataCoords.x(), dataCoords.y());
    }
  } else if (evt->button() == Qt::RightButton) {
    stopEvent = true;
    emit showContextMenu();
  }
  return stopEvent;
}

/**
 * Wire to the home button click
 */
void MiniPlotMpl::zoomOutOnPlot() { m_zoomTool.zoomOut(); }

} // namespace MantidQt::MantidWidgets
