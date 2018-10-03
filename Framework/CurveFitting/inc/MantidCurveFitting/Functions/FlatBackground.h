// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_FLATBACKGROUND_H_
#define MANTID_CURVEFITTING_FLATBACKGROUND_H_

#include "MantidCurveFitting/Functions/BackgroundFunction.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** FlatBackground : TODO: DESCRIPTION

  @date 2012-03-20
*/
class DLLExport FlatBackground : public BackgroundFunction {
public:
  std::string name() const override;
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;

private:
  void init() override;
  /// Calculate histogram data.
  void histogram1D(double *out, double left, const double *right,
                   const size_t nBins) const override;
  /// Devivatives of the histogram.
  void histogramDerivative1D(API::Jacobian *jacobian, double left,
                             const double *right,
                             const size_t nBins) const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_FLATBACKGROUND_H_ */
