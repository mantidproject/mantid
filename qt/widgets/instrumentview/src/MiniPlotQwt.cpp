// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/MiniPlotQwt.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/InstrumentView/PeakMarker2D.h"

#include <MantidQtWidgets/Plotting/Qwt/qwt_compat.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_zoomer.h>
#include <qwt_scale_div.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>

#include <QContextMenuEvent>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>

#include <cmath>

namespace {
Mantid::Kernel::Logger g_log("MiniPlotQwt");
}

namespace MantidQt {
namespace MantidWidgets {

MiniPlotQwt::MiniPlotQwt(QWidget *parent)
    : QwtPlot(parent), m_curve(nullptr), m_xUnits("") {
  const QFont &font = parent->font();
  setAxisFont(QwtPlot::xBottom, font);
  setAxisFont(QwtPlot::yLeft, font);
  setYAxisLabelRotation(-90);
  QwtText dummyText;
  dummyText.setFont(font);
  setAxisTitle(xBottom, dummyText);
  canvas()->setCursor(Qt::ArrowCursor);
  setContextMenuPolicy(Qt::DefaultContextMenu);
  m_zoomer =
      new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
                        QwtPicker::DragSelection | QwtPicker::CornerToCorner,
                        QwtPicker::AlwaysOff, canvas());
  m_zoomer->setRubberBandPen(QPen(Qt::black));
  QList<QColor> colors;
  m_colors << Qt::red << Qt::green << Qt::blue << Qt::cyan << Qt::magenta
           << Qt::yellow << Qt::gray;
  m_colors << Qt::darkRed << Qt::darkGreen << Qt::darkBlue << Qt::darkCyan
           << Qt::darkMagenta << Qt::darkYellow << Qt::darkGray;
  m_colorIndex = 0;
  m_x0 = 0;
  m_y0 = 0;

  // Initial scales so the plot looks sensible
  setXScale(0, 1);
  setYScale(-1.2, 1.2);
}

/**
 * Destructor.
 */
MiniPlotQwt::~MiniPlotQwt() { clearAll(); }

/**
 * Set the X label of the plot
 * @param xunit
 */
void MiniPlotQwt::setXLabel(QString xunit) {
  m_xUnits = xunit;
  this->setAxisTitle(xBottom, m_xUnits);
}

/**
 * Set the scale of the horizontal axis
 * @param from :: Minimum value
 * @param to :: Maximum value
 */
void MiniPlotQwt::setXScale(double from, double to) {
  QFontMetrics fm(axisFont(QwtPlot::xBottom));
  int n = from != 0.0 ? abs(static_cast<int>(floor(log10(fabs(from))))) : 0;
  int n1 = to != 0.0 ? abs(static_cast<int>(floor(log10(fabs(to))))) : 0;
  if (n1 > n)
    n = n1;
  n += 4;
  // approxiamte width of a tick label in pixels
  int labelWidth = n * fm.width("0");
  // calculate number of major ticks
  int nMajorTicks = this->width() / labelWidth;
  if (nMajorTicks > 6)
    nMajorTicks = 6;
  // try creating a scale
  const QwtScaleDiv div = axisScaleEngine(QwtPlot::xBottom)
                              ->divideScale(from, to, nMajorTicks, nMajorTicks);
  // Major ticks are placed at round numbers so the first or last tick could be
  // missing making
  // scale look ugly. Trying to fix it if possible
  bool rescaled = false;
  // get actual tick positions
  const QwtValueList &ticks = div.ticks(QwtScaleDiv::MajorTick);
  if (!ticks.empty() && ticks.size() < nMajorTicks) {
    // how much first tick is shifted from the lower bound
    double firstShift = ticks.front() - div.lBound();
    // how much last tick is shifted from the upper bound
    double lastShift = div.hBound() - ticks.back();
    // range of the scale
    double range = fabs(div.hBound() - div.lBound());
    // we say that 1st tick is missing if first tick is father away from its end
    // of the scale
    // than the last tick is from its end
    bool isFirstMissing = fabs(firstShift) > fabs(lastShift);
    // if first tick is missing
    if (isFirstMissing) {
      // distance between nearest major ticks
      double tickSize = 0;
      if (ticks.size() == 1) {
        // guess the tick size in case of only one visible
        double tickLog = log10(firstShift);
        tickLog = tickLog > 0 ? ceil(tickLog) : floor(tickLog);
        tickSize = pow(10., tickLog);
      } else if (ticks.size() > 1) {
        // take the difference between the two first ticks
        tickSize = ticks[1] - ticks[0];
      }
      // claculate how much lower bound must be moved to make the missing tick
      // visible
      double shift = (ticks.front() - tickSize) - from;
      // if the shift is not very big rescale the axis
      if (fabs(shift / range) < 0.1) {
        from += shift;
        const QwtScaleDiv updatedDiv =
            axisScaleEngine(QwtPlot::xBottom)
                ->divideScale(from, to, nMajorTicks, nMajorTicks);
        setAxisScaleDiv(xBottom, updatedDiv);
        rescaled = true;
      }
    } else // last tick is missing
    {
      // distance between nearest major ticks
      double tickSize = 0;
      if (ticks.size() == 1) {
        // guess the tick size in case of only one visible
        double tickLog = log10(lastShift);
        tickLog = tickLog > 0 ? ceil(tickLog) : floor(tickLog);
        tickSize = pow(10., tickLog);
      } else if (ticks.size() > 1) {
        // take the difference between the two first ticks
        tickSize = ticks[1] - ticks[0];
      }
      // claculate how much upper bound must be moved to make the missing tick
      // visible
      double shift = (ticks.back() + tickSize) - to;
      // if the shift is not very big rescale the axis
      if (fabs(shift / range) < 0.1) {
        to += shift;
        const QwtScaleDiv updatedDiv =
            axisScaleEngine(QwtPlot::xBottom)
                ->divideScale(from, to, nMajorTicks, nMajorTicks);
        setAxisScaleDiv(xBottom, updatedDiv);
        rescaled = true;
      }
    }
  }

  if (!rescaled) {
    setAxisScaleDiv(xBottom, div);
  }
  m_zoomer->setZoomBase();
}

/**
 * Set the scale of the vertical axis
 * @param from :: Minimum value
 * @param to :: Maximum value
 */
void MiniPlotQwt::setYScale(double from, double to) {
  if (isYLogScale()) {
    if (from == 0 && to == 0) {
      from = 1;
      to = 10;
    } else {
      double yPositiveMin = to;
      QMap<QString, QwtPlotCurve *>::const_iterator cv = m_stored.begin();
      QwtPlotCurve *curve = nullptr;
      do {
        if (cv != m_stored.end()) {
          curve = cv.value();
          ++cv;
        } else if (curve == m_curve) {
          curve = nullptr;
          break;
        } else {
          curve = m_curve;
        }
        if (!curve)
          break;
        int n = curve->dataSize();
        for (int i = 0; i < n; ++i) {
          double y = curve->y(i);
          if (y > 0 && y < yPositiveMin) {
            yPositiveMin = y;
          }
        }
      } while (curve);
      from = yPositiveMin;
    }
  }
  setAxisScale(QwtPlot::yLeft, from, to);
  m_zoomer->setZoomBase();
}

/**
 * Set the data for the curve to display
 * @param x :: A vector of X values
 * @param y :: A vector of Y values
 * @param xUnits :: Units for the data
 * @param curveLabel :: A label for hthe die
 */
void MiniPlotQwt::setData(std::vector<double> x, std::vector<double> y,
                          QString xunit, QString curveLabel) {
  if (x.empty()) {
    g_log.warning("setData(): X array is empty!");
    return;
  }
  if (y.empty()) {
    g_log.warning("setData(): Y array is empty!");
    return;
  }
  if (x.size() != y.size()) {
    g_log.warning(std::string(
        "setData(): X/Y size mismatch! X=" + std::to_string(x.size()) +
        ", Y=" + std::to_string(y.size())));
    return;
  }

  m_xUnits = xunit;
  m_label = curveLabel;
  if (!m_curve) {
    m_curve = new QwtPlotCurve();
    m_curve->attach(this);
  }
  int dataSize = static_cast<int>(x.size());
  m_curve->setData(x.data(), y.data(), dataSize);
  setXScale(x[0], x[dataSize - 1]);
  double from = y[0];
  double to = from;
  for (int i = 0; i < dataSize; ++i) {
    const double &yy = y[i];
    if (yy < from)
      from = yy;
    if (yy > to)
      to = yy;
  }
  setYScale(from, to);
}

/**
 * Remove the curve. Rescale the axes if there are stored curves.
 */
void MiniPlotQwt::clearCurve() {
  // remove the curve
  if (m_curve) {
    m_curve->attach(nullptr);
    m_curve = nullptr;
  }
  clearPeakLabels();
  // if there are stored curves rescale axes to make them fully visible
  if (hasStored()) {
    QMap<QString, QwtPlotCurve *>::const_iterator curve = m_stored.begin();
    QwtDoubleRect br = (**curve).boundingRect();
    double xmin = br.left();
    double xmax = br.right();
    double ymin = br.top();
    double ymax = br.bottom();
    ++curve;
    for (; curve != m_stored.end(); ++curve) {
      QwtDoubleRect br = (**curve).boundingRect();
      if (br.left() < xmin)
        xmin = br.left();
      if (br.right() > xmax)
        xmax = br.right();
      if (br.top() < ymin)
        ymin = br.top();
      if (br.bottom() > ymax)
        ymax = br.bottom();
    }
    setXScale(xmin, xmax);
    setYScale(ymin, ymax);
  }
}

void MiniPlotQwt::resizeEvent(QResizeEvent *e) {
  QwtPlot::resizeEvent(e);
  recalcAxisDivs();
}

/**
 * Recalculate axis divisions to make sure that tick labels don't overlap
 */
void MiniPlotQwt::recalcAxisDivs() {
  recalcXAxisDivs();
  recalcYAxisDivs();
}

/**
 * Recalculate x-axis divisions to make sure that tick labels don't overlap
 */
void MiniPlotQwt::recalcXAxisDivs() {
  const QwtScaleDiv *div0 = axisScaleDiv(QwtPlot::xBottom);
  double from = div0->lBound();
  double to = div0->hBound();
  setXScale(from, to);
}

/**
 * Recalculate y-axis divisions to make sure that tick labels don't overlap
 */
void MiniPlotQwt::recalcYAxisDivs() {
  const QwtScaleDiv *div0 = axisScaleDiv(QwtPlot::yLeft);
  double from = div0->lBound();
  double to = div0->hBound();
  setYScale(from, to);
}

void MiniPlotQwt::contextMenuEvent(QContextMenuEvent *e) {
  // context menu will be handled with mouse events
  e->accept();
}

void MiniPlotQwt::mousePressEvent(QMouseEvent *e) {
  if (e->buttons() & Qt::RightButton) {
    if (m_zoomer->zoomRectIndex() == 0) {
      e->accept();
      // plot owner will display and process context menu
      emit showContextMenu();
    }
    return;
  }
  if (e->buttons() & Qt::LeftButton) {
    e->accept();
    m_x0 = e->x();
    m_y0 = e->y();
  }
}

void MiniPlotQwt::mouseReleaseEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    if (m_x0 == e->x() && m_y0 == e->y()) { // there were no dragging
      emit clickedAt(invTransform(xBottom, e->x() - canvas()->x()),
                     invTransform(yLeft, e->y() - canvas()->y()));
    }
  }
}

void MiniPlotQwt::setYAxisLabelRotation(double degrees) {
  axisScaleDraw(yLeft)->setLabelRotation(degrees);
}

/**
 * Set the log scale on the y axis
 */
void MiniPlotQwt::setYLogScale() {
  const QwtScaleDiv *div = axisScaleDiv(QwtPlot::yLeft);
  double from = div->lBound();
  double to = div->hBound();
  QwtLog10ScaleEngine *logEngine = new QwtLog10ScaleEngine();
  setAxisScaleEngine(yLeft, logEngine);
  setYScale(from, to);
  recalcYAxisDivs();
  replot();
}

/**
 * Set the linear scale on the y axis
 */
void MiniPlotQwt::setYLinearScale() {
  QwtLinearScaleEngine *engine = new QwtLinearScaleEngine();
  setAxisScaleEngine(yLeft, engine);
  replot();
}

/**
 * Add new peak label
 * @param marker :: A pointer to a PeakLabel, becomes owned by MiniPlotQwt
 */
void MiniPlotQwt::addPeakLabel(const PeakMarker2D *marker) {
  PeakLabel *label = new PeakLabel(marker, this);
  label->attach(this);
  m_peakLabels.append(label);
}

/**
 * Removes all peak labels.
 */
void MiniPlotQwt::clearPeakLabels() {
  foreach (PeakLabel *label, m_peakLabels) {
    label->detach();
    delete label;
  }
  m_peakLabels.clear();
}

/**
 * Returns true if the current curve isn't NULL
 */
bool MiniPlotQwt::hasCurve() const { return m_curve != nullptr; }

/**
 * Store current curve.
 */
void MiniPlotQwt::store() {
  if (m_curve) {
    removeCurve(m_label);
    m_stored.insert(m_label, m_curve);
    m_curve->setPen(QPen(m_colors[m_colorIndex]));
    ++m_colorIndex;
    m_colorIndex %= m_colors.size();
    m_curve = nullptr;
    m_label = "";
  }
}

/**
 * Returns true if there are some stored curves.
 */
bool MiniPlotQwt::hasStored() const { return !m_stored.isEmpty(); }

QStringList MiniPlotQwt::getLabels() const {
  QStringList out;
  QMap<QString, QwtPlotCurve *>::const_iterator it = m_stored.begin();
  for (; it != m_stored.end(); ++it) {
    out << it.key();
  }
  return out;
}

/**
 * Return the colour of a stored curve.
 * @param label :: The label of that curve.
 */
QColor MiniPlotQwt::getCurveColor(const QString &label) const {
  if (m_stored.contains(label)) {
    return m_stored[label]->pen().color();
  }
  return Qt::black;
}

/**
 * Remove a stored curve.
 * @param label :: The label of a curve to remove.
 */
void MiniPlotQwt::removeCurve(const QString &label) {
  QMap<QString, QwtPlotCurve *>::iterator it = m_stored.find(label);
  if (it != m_stored.end()) {
    it.value()->detach();
    delete it.value();
    m_stored.erase(it);
  }
}

/**
 * Does the y axis have the log scale?
 */
bool MiniPlotQwt::isYLogScale() const {
  const QwtScaleEngine *engine = axisScaleEngine(yLeft);
  return dynamic_cast<const QwtLog10ScaleEngine *>(engine) != nullptr;
}

/**
 * Remove all displayable objects from the plot.
 */
void MiniPlotQwt::clearAll() {
  QMap<QString, QwtPlotCurve *>::const_iterator it = m_stored.begin();
  for (; it != m_stored.end(); ++it) {
    it.value()->detach();
    delete it.value();
  }
  m_stored.clear();
  clearPeakLabels();
  clearCurve();
  m_colorIndex = 0;
}

/* ---------------------------- PeakLabel --------------------------- */

/**
 * Draw PeakLabel on a plot
 */
void PeakLabel::draw(QPainter *painter, const QwtScaleMap &xMap,
                     const QwtScaleMap &yMap, const QRect &canvasRect) const {
  (void)yMap;
  double peakX;
  if (m_plot->getXUnits().isEmpty())
    return;
  if (m_plot->getXUnits() == "dSpacing") {
    peakX = m_marker->getPeak().getDSpacing();
  } else if (m_plot->getXUnits() == "Wavelength") {
    peakX = m_marker->getPeak().getWavelength();
  } else {
    peakX = m_marker->getPeak().getTOF();
  }
  int x = xMap.transform(peakX);
  int y =
      static_cast<int>(canvasRect.top() + m_marker->getLabelRect().height());
  painter->drawText(x, y, m_marker->getLabel());
  // std::cerr << x << ' ' << y << ' ' << m_marker->getLabel().toStdString() <<
  // '\n';
}
} // namespace MantidWidgets
} // namespace MantidQt
