// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_STEEPESTDESCENT_MINIMIZERMINIMIZER_H_
#define MANTID_CURVEFITTING_STEEPESTDESCENT_MINIMIZERMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/FuncMinimizers/DerivMinimizer.h"

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {
/** Implementing the steepest descent algorithm
    by wrapping the IFuncMinimizer interface around the GSL implementation of
   this algorithm.

    @author Roman Tolchenov, Tessella plc
*/
class DLLExport SteepestDescentMinimizer : public DerivMinimizer {
public:
  /// Constructor.
  SteepestDescentMinimizer() : DerivMinimizer() {}
  /// Name of the minimizer.
  std::string name() const override { return "SteepestDescentMinimizer"; }

protected:
  /// Return a concrete type to initialize m_gslSolver with
  const gsl_multimin_fdfminimizer_type *getGSLMinimizerType() override;

  /// Static reference to the logger class
  static Kernel::Logger &g_log;
};

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_STEEPESTDESCENT_MINIMIZERMINIMIZER_H_*/
