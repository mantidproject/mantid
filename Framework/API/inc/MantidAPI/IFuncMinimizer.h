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

namespace Mantid {
namespace API {
// Forward declaration
class IFitFunction;

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
  /// @param maxIterations :: Maximum number of iterations.
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
