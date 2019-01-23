// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_PRODUCTLINEAREXP_H_
#define MANTID_CURVEFITTING_PRODUCTLINEAREXP_H_

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** ProductLinearExp : Function that evauates the product of an exponential and
  linear function.
*/
class DLLExport ProductLinearExp : public API::ParamFunction,
                                   public API::IFunction1D {
public:
  ProductLinearExp();

  std::string name() const override { return "ProductLinearExp"; }

  const std::string category() const override { return "Calibrate"; }

protected:
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_PRODUCTLINEAREXP_H_ */
