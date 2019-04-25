// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/ErrorCurve.h"

#include <QPainter>
#include <qwt_scale_map.h>
#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {

/// Create a error curve dependent on a data curve.
/// @param dataCurve :: The curve displaying the data.
/// @param errors :: A vector with error bars.
ErrorCurve::ErrorCurve(const QwtPlotCurve *dataCurve,
                       const std::vector<double> &errors) {
  if (!dataCurve) {
    throw std::runtime_error("Null pointer to a data curve.");
  }
  auto n = dataCurve->dataSize();
  m_x.resize(n);
  m_y.resize(n);
  for (int i = 0; i < n; ++i) {
    m_x[i] = dataCurve->x(i);
    m_y[i] = dataCurve->y(i);
  }

  if (!errors.empty()) {
    setErrorBars(errors);
  }

  setItemAttribute(QwtPlotItem::AutoScale, true);
  m_pen = dataCurve->pen();
}

/// Set error bars
/// @param errors :: A pointer to an array with error bars.
void ErrorCurve::setErrorBars(const std::vector<double> &errors) {
  if (errors.size() != m_x.size()) {
    throw std::runtime_error(
        "Number of error values is different form the number of data points.");
  }
  m_e = errors;
}

/// Draw this curve
void ErrorCurve::draw(QPainter *painter, const QwtScaleMap &xMap,
                      const QwtScaleMap &yMap,
                      const QRect & /*canvasRect*/) const {
  if (m_e.empty())
    return;
  painter->save();
  painter->setPen(m_pen);
  int n = dataSize();
  const int dx = 4;

  for (int i = 0; i < n; ++i) {
    const double E = m_e[i];
    if (E <= 0.0)
      continue;

    const int xi = xMap.transform(m_x[i]);
    const double Y = m_y[i];
    const int yi = yMap.transform(Y);
    const int ei1 = yMap.transform(Y - E);
    const int ei2 = yMap.transform(Y + E);

    painter->drawLine(xi, ei1, xi, yi);
    painter->drawLine(xi - dx, ei1, xi + dx, ei1);

    painter->drawLine(xi, yi, xi, ei2);
    painter->drawLine(xi - dx, ei2, xi + dx, ei2);
  }

  painter->restore();
}

/// Number of points in the curve
int ErrorCurve::dataSize() const { return static_cast<int>(m_x.size()); }

/**
 * Bounding rectangle of all points and error bars, used for autoscaling.
 * For an empty series the rectangle is invalid, meaning autoscaler ignores it.
 * @returns Bounding rectangle
 */
QRectF ErrorCurve::boundingRect() const {
  // Default returns an invalid rect, like base class
  QRectF rect = QRectF(1, 1, -2, -2);
  if (!m_e.empty()) {
    int n = dataSize();
    const double width = m_x[n - 1] - m_x[0];
    double max = std::numeric_limits<double>::min();
    double min = std::numeric_limits<double>::max();
    for (int i = 0; i < n; i++) {
      const double upper = m_y[i] + m_e[i];
      const double lower = m_y[i] - m_e[i];
      if (upper > max) {
        max = upper;
      }
      if (lower < min) {
        min = lower;
      }
    }
    const double height = max - min;
    // On a graph, y increases from bottom to top.
    // Coordinate system expects y to increase from top to bottom.
    // That's why we set rect's "top left" to the bottom left
    // (can't set a negative height or it won't autoscale).
    rect.setRect(m_x[0], min, width, height);
  }
  return rect;
}

} // namespace MantidWidgets
} // namespace MantidQt
