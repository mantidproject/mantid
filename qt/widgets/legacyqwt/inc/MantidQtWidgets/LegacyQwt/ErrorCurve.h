#ifndef MANTIDWIDGETS_ERRORCURVE_H
#define MANTIDWIDGETS_ERRORCURVE_H

#include "DllOption.h"
#include <qwt_plot_curve.h>
#include <qwt_plot_item.h>

namespace MantidQt {
namespace MantidWidgets {

/// Curve to draw error bars.
class EXPORT_OPT_MANTIDQT_LEGACYQWT ErrorCurve : public QwtPlotItem {

public:
  ErrorCurve(const QwtPlotCurve *dataCurve,
             const std::vector<double> &errors = std::vector<double>());
  /// Set error bars
  void setErrorBars(const std::vector<double> &errors);
  /// Number of points in the curve
  int dataSize() const;
  /// Draw this curve
  void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QRect &canvasRect) const override;
  QRectF boundingRect() const override;

private:
  std::vector<double> m_x; ///< The x coordinates
  std::vector<double> m_y; ///< The y coordinates
  std::vector<double> m_e; ///< The error bars
  QPen m_pen;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDWIDGETS_ERRORCURVE_H */
