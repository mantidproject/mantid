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
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ICostFunction.h"
#include "MantidKernel/PropertyManager.h"

#include <string>

namespace Mantid {
namespace API {
// Forward declaration
class IFitFunction;

/// Canonical minimizer "OutputStatus" strings. These are the single source of truth
/// for the status text a minimizer reports (and that consumers such as the Fit
/// algorithm and FitPeaks compare against), so the wording only lives in one place.
namespace MinimizerStatus {
/// Reported when a minimizer has fully converged.
inline const std::string SUCCESS = "success";
/// Reported by Levenberg-Marquardt when the change in the cost function between
/// iterations has fallen below tolerance (an essentially-converged stop).
inline const std::string CHANGES_IN_FUNCTION_TOO_SMALL = "Changes in function value are too small";
/// Reported by Levenberg-Marquardt when the change in the parameter values between
/// iterations has fallen below tolerance (an essentially-converged stop).
inline const std::string CHANGES_IN_PARAMETER_TOO_SMALL = "Changes in parameter value are too small";

/// Whether a Fit "OutputStatus" string should be treated as a converged fit.
/// In strict mode only "success" is accepted. Otherwise the tolerance-limited stopping
/// conditions count too: a minimiser started from an already-optimal set of parameters
/// stops that way routinely, and treating it as a failure is wrong.
/// Kept here beside the strings so callers do not each re-implement the comparison.
inline bool isConverged(const std::string &status, const bool strict = false) {
  if (status == SUCCESS)
    return true;
  if (strict)
    return false;
  return status == CHANGES_IN_FUNCTION_TOO_SMALL || status == CHANGES_IN_PARAMETER_TOO_SMALL;
}
} // namespace MinimizerStatus

/** An interface for function minimizers. Minimizers minimize cost functions.

    @author Anders Markvardsen, ISIS, RAL
    @date 11/12/2009
*/
class MANTID_API_DLL IFuncMinimizer : public Kernel::PropertyManager {
public:
  /// Initialize minimizer.
  /// @param function :: Function to minimize
  /// @param maxIterations :: Maximum number of iterations.
  virtual void initialize(API::ICostFunction_sptr function, size_t maxIterations = 1000) = 0;

  /// Get name of minimizer
  virtual std::string name() const = 0;

  /// Do one iteration
  /// @param iteration :: Current iteration number. 0 <= iteration <
  /// maxIterations
  /// @return :: true if iterations should be continued or false to stop
  virtual bool iterate(size_t iteration) = 0;

  /// Perform iteration with minimizer and return true if successful.
  virtual bool minimize(size_t maxIterations = 1000);

  /// Get the error string
  virtual std::string getError() const { return m_errorString; }

  /// Get value of cost function
  virtual double costFunctionVal() = 0;

  /// Finalize minimization, eg store additional outputs
  virtual void finalize() {}

protected:
  /// Error string.
  std::string m_errorString;
};

using IFuncMinimizer_sptr = std::shared_ptr<IFuncMinimizer>;

} // namespace API
} // namespace Mantid
