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
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/FuncMinimizers/DerivMinimizer.h"

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {
/** Implementing Polak-Ribiere flavour of the conjugate gradient algorithm
    by wrapping the IFuncMinimizer interface around the GSL implementation of
   this algorithm.

    @author Anders Markvardsen, ISIS, RAL
    @date 13/1/2010
*/
class MANTID_CURVEFITTING_DLL PRConjugateGradientMinimizer : public DerivMinimizer {
public:
  /// Constructor.
  PRConjugateGradientMinimizer() : DerivMinimizer() {}
  /// Name of the minimizer.
  std::string name() const override { return "Conjugate gradient (Polak-Ribiere imp.)"; }

protected:
  /// Return a concrete type to initialize m_gslSolver with
  const gsl_multimin_fdfminimizer_type *getGSLMinimizerType() override;
};

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
