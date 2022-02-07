// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidCurveFitting/GSLMatrix.h"
#include "MantidCurveFitting/GSLVector.h"

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {
class CostFuncLeastSquares;
} // namespace CostFunctions
namespace FuncMinimisers {

/**
    Implements a Gauss-Newton minimization algorithm with damping
    for use with least squares cost function.

    @author Roman Tolchenov, Tessella plc
*/
class MANTID_CURVEFITTING_DLL DampedGaussNewtonMinimizer : public API::IFuncMinimizer {
public:
  /// Constructor
  DampedGaussNewtonMinimizer(double relTol = 0.0001);
  /// Name of the minimizer.
  std::string name() const override { return "DampedGaussNewtonMinimizer"; }

  /// Initialize minimizer, i.e. pass a function to minimize.
  void initialize(API::ICostFunction_sptr function, size_t maxIterations = 0) override;
  /// Do one iteration.
  bool iterate(size_t) override;
  /// Return current value of the cost function
  double costFunctionVal() override;

private:
  /// Pointer to the cost function. Must be the least squares.
  std::shared_ptr<CostFunctions::CostFuncLeastSquares> m_leastSquares;
  /// Relative tolerance.
  double m_relTol;
  /// The damping mu parameter in the Levenberg-Marquardt method.
  // double m_damping;
};
} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
