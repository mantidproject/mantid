// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FuncMinimizers/BFGS_Minimizer.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidKernel/Logger.h"

namespace Mantid::CurveFitting::FuncMinimisers {
namespace {
/// static logger object
Kernel::Logger g_log("BFGS_Minimizer");
} // namespace

DECLARE_FUNCMINIMIZER(BFGS_Minimizer, BFGS)

/// Return a concrete type to initialize m_gslSolver
/// gsl_multimin_fdfminimizer_vector_bfgs2
const gsl_multimin_fdfminimizer_type *BFGS_Minimizer::getGSLMinimizerType() {
  return gsl_multimin_fdfminimizer_vector_bfgs2;
}

} // namespace Mantid::CurveFitting::FuncMinimisers
