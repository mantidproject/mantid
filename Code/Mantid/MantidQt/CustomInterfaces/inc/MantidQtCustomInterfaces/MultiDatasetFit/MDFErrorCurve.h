#ifndef MDFERRORCURVE_H_
#define MDFERRORCURVE_H_

#include <qwt_plot_item.h>
#include <qwt_plot_curve.h>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MDF
{

/// Curve to draw error bars.
class ErrorCurve: public QwtPlotItem
{
public:
  ErrorCurve(const QwtPlotCurve* dataCurve, const std::vector<double>& errors = std::vector<double>());
  /// Set error bars
  void setErrorBars(const std::vector<double>& errors);
  /// Number of points in the curve
  int dataSize() const;
  /// Draw this curve
  void draw(QPainter *painter, 
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRect &canvasRect) const;
private:

  std::vector<double> m_x; ///< The x coordinates
  std::vector<double> m_y; ///< The y coordinates
  std::vector<double> m_e; ///< The error bars
  QPen m_pen;
};

} // MDF
} // CustomInterfaces
} // MantidQt


#endif /*MDFERRORCURVE_H_*/
