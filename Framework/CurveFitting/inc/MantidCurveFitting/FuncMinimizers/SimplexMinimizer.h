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
#include "MantidCurveFitting/DllConfig.h"

#include <gsl/gsl_multimin.h>

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {
/** Implementing Simplex by wrapping the IFuncMinimizer interface
    around the GSL implementation of this algorithm.

    @author Anders Markvardsen, ISIS, RAL
    @date 8/1/2010
*/
class MANTID_CURVEFITTING_DLL SimplexMinimizer : public API::IFuncMinimizer {
public:
  /// Constructor setting a value for the relative error acceptance
  /// (default=0.01)
  SimplexMinimizer(const double epsabs = 1e-2);
  /// Destructor
  ~SimplexMinimizer() override;

  /// Overloading base class methods
  std::string name() const override { return "Simplex"; }
  /// Do one iteration
  bool iterate(size_t) override;
  /// Return current value of the cost function
  double costFunctionVal() override;
  /// Initialize minimizer, i.e. pass a function to minimize.
  void initialize(API::ICostFunction_sptr function, size_t maxIterations = 0) override;

protected:
  void resetSize(const double &size);

private:
  /// clear memory
  void clearMemory();

  /// Used by the GSL to evaluate the function
  static double fun(const gsl_vector *x, void *params);

  /// Absolute value of the error that is considered a fit
  double m_epsabs;

  /// Function to minimize.
  API::ICostFunction_sptr m_costFunction;

  /// size of simplex
  double m_size;

  /// used by GSL
  gsl_vector *m_simplexStepSize;

  /// Starting parameter values
  gsl_vector *m_startGuess;

  /// pointer to the GSL solver doing the work
  gsl_multimin_fminimizer *m_gslSolver;

  /// GSL simplex minimizer container
  gsl_multimin_function gslContainer;
};

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
