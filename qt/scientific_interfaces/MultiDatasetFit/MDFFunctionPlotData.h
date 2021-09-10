// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QString>

#include <memory>
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
  MDFFunctionPlotData(const std::shared_ptr<Mantid::API::IFunction> &fun, double startX, double endX,
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
  std::shared_ptr<Mantid::API::IFunction> m_function;
  /// Curve object to plot the function.
  QwtPlotCurve *m_functionCurve;

  /// Default size of the function domain
  static size_t g_defaultDomainSize;
};

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt
