#ifndef MDFFUNCTIONPLOTDATA_H_
#define MDFFUNCTIONPLOTDATA_H_

#include <QString>

#include <boost/shared_ptr.hpp>
#include <qwt_double_rect.h>

// Forward declarations
class QwtPlot;
class QwtPlotCurve;

namespace Mantid {
namespace API {
class IFunction;
}
} // namespace Mantid

namespace MantidQt {
namespace CustomInterfaces {
namespace MDF {

/**
 * Contains the curve and helps to set the data to plot a function.
 */
class MDFFunctionPlotData {
public:
  MDFFunctionPlotData(boost::shared_ptr<Mantid::API::IFunction> fun,
                      double startX, double endX,
                      size_t nX = g_defaultDomainSize);
  ~MDFFunctionPlotData();
  QwtDoubleRect boundingRect() const;
  void show(QwtPlot *plot);
  void hide();
  /// Set curve data
  void setDomain(double startX, double endX, size_t nX = g_defaultDomainSize);
  /// Update function parameters and attributes
  void updateFunction(const Mantid::API::IFunction &fun);

private:
  // No copying
  MDFFunctionPlotData(const MDFFunctionPlotData &);
  MDFFunctionPlotData &operator=(const MDFFunctionPlotData &);

  /// The function
  boost::shared_ptr<Mantid::API::IFunction> m_function;
  /// Curve object to plot the function.
  QwtPlotCurve *m_functionCurve;

  /// Default size of the function domain
  static size_t g_defaultDomainSize;
};

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*MDFFUNCTIONPLOTDATA_H_*/
