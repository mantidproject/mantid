#include "MantidQtWidgets/InstrumentView/MiniPlotMpl.h"
#include "MantidQtWidgets/MplCpp/ColorConverter.h"
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
#include <QtGlobal>

using MantidQt::Widgets::MplCpp::ColorConverter;
using MantidQt::Widgets::MplCpp::FigureCanvasQt;
using MantidQt::Widgets::MplCpp::cycler;
namespace Python = MantidQt::Widgets::MplCpp::Python;

namespace {
const char *ACTIVE_CURVE_FORMAT = "k-";
const char *STORED_LINE_COLOR_CYCLE = "bgrcmyk";
const char *LIN_SCALE_NAME = "linear";
const char *LOG_SCALE_NAME = "symlog";
Mantid::Kernel::Logger g_log("MiniPlotMpl");

QPushButton *createHomeButton() {
  auto mpl(Python::NewRef(PyImport_ImportModule("matplotlib")));
  QDir dataPath(
      PyString_AsString(Python::Object(mpl.attr("get_data_path")()).ptr()));
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
namespace MantidWidgets {

/**
 * Construct a blank miniplot with a single subplot
 * @param parent A pointer to its parent widget
 */
MiniPlotMpl::MiniPlotMpl(QWidget *parent)
    : QWidget(parent), m_canvas(new FigureCanvasQt(111)),
      m_homeBtn(createHomeButton()), m_lines(),
      m_colorCycler(cycler("color", STORED_LINE_COLOR_CYCLE)), m_xunit(),
      m_activeCurveLabel(), m_storedCurveLabels(), m_zoomer(m_canvas) {
  auto plotLayout = new QGridLayout(this);
  plotLayout->setContentsMargins(0, 0, 0, 0);
  plotLayout->setSpacing(0);
  // We intentionally place the canvas and home button in the same location
  // in the grid layout so that they overlap and take up less space.
  plotLayout->addWidget(m_canvas, 0, 0);
  plotLayout->addWidget(m_homeBtn, 0, 0, Qt::AlignLeft | Qt::AlignBottom);
  setLayout(plotLayout);

  // Capture mouse events destined for the plot canvas
  m_canvas->installEventFilterToMplCanvas(this);
  // Mouse events cause zooming by default. See mouseReleaseEvent
  // for exceptions
  m_zoomer.enableZoom(true);
  connect(m_homeBtn, SIGNAL(clicked()), this, SLOT(onHomeClicked()));
}

/**
 * Set data and metadata for a new curve
 * @param x The X-axis data
 * @param y The Y-axis data
 * @param xunit The X unit a label
 * @param curveLabel A label for the curve data
 */
void MiniPlotMpl::setData(std::vector<double> x, std::vector<double> y,
                          QString xunit, QString curveLabel) {
  if (warnDataInvalid(x, y))
    return;

  clearCurve();
  auto axes = m_canvas->gca();
  // plot automatically calls "scalex=True, scaley=True"
  m_lines.emplace_back(
      axes.plot(std::move(x), std::move(y), ACTIVE_CURVE_FORMAT));
  setXLabel(std::move(xunit));
  m_activeCurveLabel = curveLabel;
  // If the current axis limits can fit the data then matplotlib
  // won't change the axis scale. If the intensity of different plots
  // is very different we need ensure the scale is tight enough to
  // see newer plots so we force a recalculation from the data
  axes.relim();
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
 * @return True if the Y scale is logarithmic
 */
bool MiniPlotMpl::isYLogScale() const {
  return m_canvas->gca().getYScale() == LOG_SCALE_NAME;
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
 * Set the Y scale to logarithmic
 */
void MiniPlotMpl::setYLogScale() { m_canvas->gca().setYScale(LOG_SCALE_NAME); }

/**
 * Set the Y scale to linear
 */
void MiniPlotMpl::setYLinearScale() {
  m_canvas->gca().setYScale(LIN_SCALE_NAME);
}

/**
 * Clear all artists from the canvas
 */
void MiniPlotMpl::clearAll() {
  m_lines.clear();
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
    // handled by mouse press events below
    stopEvent = true;
    break;
  case QEvent::MouseButtonPress:
  case QEvent::MouseButtonRelease: {
    auto mouseEvt = static_cast<QMouseEvent *>(evt);
    if (mouseEvt->buttons() & Qt::RightButton) {
      stopEvent = true;
      emit showContextMenu();
    }
  } break;
  default:
    break;
  }
  return stopEvent;
}

/**
 * Wire to the home button click
 */
void MiniPlotMpl::onHomeClicked() { m_zoomer.zoomOut(); }

} // namespace MantidWidgets
} // namespace MantidQt
