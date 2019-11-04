// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_LEVENBERGMARQUARDTMDMINIMIZER_H_
#define MANTID_CURVEFITTING_LEVENBERGMARQUARDTMDMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidCurveFitting/GSLMatrix.h"
#include "MantidCurveFitting/GSLVector.h"

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {
class CostFuncFitting;
} // namespace CostFunctions

namespace FuncMinimisers {
/** Implementing Levenberg-Marquardt algorithm. Uses the normal system calculate
    the corrections to the parameters. Expects a cost function that can evaluate
    the value, the derivatives and the hessian matrix.

    @author Roman Tolchenov, Tessella plc
*/
class DLLExport LevenbergMarquardtMDMinimizer : public API::IFuncMinimizer {
public:
  /// Constructor
  LevenbergMarquardtMDMinimizer();
  /// Name of the minimizer.
  std::string name() const override { return "Levenberg-MarquardtMD"; }

  /// Initialize minimizer, i.e. pass a function to minimize.
  void initialize(API::ICostFunction_sptr function,
                  size_t maxIterations = 0) override;
  /// Do one iteration.
  bool iterate(size_t iteration) override;
  /// Return current value of the cost function
  double costFunctionVal() override;

private:
  /// Pointer to the cost function.
  boost::shared_ptr<CostFunctions::CostFuncFitting> m_costFunction;
  /// The tau parameter in the Levenberg-Marquardt method.
  double m_tau;
  /// The damping mu parameter in the Levenberg-Marquardt method.
  double m_mu;
  /// The nu parameter in the Levenberg-Marquardt method.
  double m_nu;
  /// The rho parameter in the Levenberg-Marquardt method.
  double m_rho;
  /// To keep function value
  double m_F;
  std::vector<double> m_D;
};

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LEVENBERGMARQUARDTMDMINIMIZER_H_*/
