#include "MantidQtMantidWidgets/ErrorCurve.h"

#include <QPainter>
#include <qwt_scale_map.h>
#include <stdexcept>

namespace MantidQt
{
namespace MantidWidgets
{

/// Create a error curve dependent on a data curve.
/// @param dataCurve :: The curve displaying the data.
/// @param errors :: A vector with error bars.
ErrorCurve::ErrorCurve(const QwtPlotCurve* dataCurve, const std::vector<double>& errors)
{
  if (!dataCurve)
  {
    throw std::runtime_error("Null pointer to a data curve.");
  }
  auto n = dataCurve->dataSize();
  m_x.resize(n);
  m_y.resize(n);
  for(int i = 0; i < n; ++i)
  {
    m_x[i] = dataCurve->x(i);
    m_y[i] = dataCurve->y(i);
  }

  if (!errors.empty())
  {
    setErrorBars(errors);
  }

  setItemAttribute(QwtPlotItem::AutoScale, true);
  m_pen = dataCurve->pen();
}

/// Set error bars
/// @param errors :: A pointer to an array with error bars.
void ErrorCurve::setErrorBars(const std::vector<double>& errors)
{
  if (errors.size() != m_x.size())
  {
    throw std::runtime_error("Number of error values is different form the number of data points.");
  }
  m_e = errors;
}

/// Draw this curve
void ErrorCurve::draw(QPainter *painter, 
      const QwtScaleMap &xMap, const QwtScaleMap &yMap,
      const QRect &) const
{
  if (m_e.empty()) return;
  painter->save();
  painter->setPen(m_pen);
  int n = static_cast<int>(m_x.size());
  const int dx = 4;

  for (int i = 0; i < n; ++i)
  {
    const double E = m_e[i];
    if (E <= 0.0) continue;

    const int xi = xMap.transform(m_x[i]);
    const double Y = m_y[i];
    const int yi = yMap.transform(Y);
    const int ei1 = yMap.transform(Y - E);
    const int ei2 = yMap.transform(Y + E);

    painter->drawLine(xi, ei1, xi, yi);
    painter->drawLine(xi-dx,ei1,xi+dx,ei1);

    painter->drawLine(xi, yi, xi, ei2);
    painter->drawLine(xi-dx,ei2,xi+dx,ei2);
      
  }

  painter->restore();
}

/// Number of points in the curve
int ErrorCurve::dataSize() const
{
  return static_cast<int>(m_x.size());
}

} // MantidWidgets
} // MantidQt

