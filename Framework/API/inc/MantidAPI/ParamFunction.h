#ifndef MANTID_API_PARAMFUNCTION_H_
#define MANTID_API_PARAMFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ParameterTie.h"
#include <string>
#include <vector>

namespace Mantid {
namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Jacobian;
/** Implements the part of IFunction interface dealing with parameters. This
   function has parameters of its own
    as opposed to a CompositeFunction which list of parameters consists only of
   parameters of the member functions.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 13/01/2011

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL ParamFunction : public virtual IFunction {
public:
  /// Default constructor
  ParamFunction() {}

  /// Set i-th parameter
  void setParameter(size_t, const double &value,
                    bool explicitlySet = true) override;
  /// Set i-th parameter description
  void setParameterDescription(size_t, const std::string &description) override;
  /// Get i-th parameter
  double getParameter(size_t i) const override;
  /// Set parameter by name.
  void setParameter(const std::string &name, const double &value,
                    bool explicitlySet = true) override;
  /// Set description of parameter by name.
  void setParameterDescription(const std::string &name,
                               const std::string &description) override;
  /// Get parameter by name.
  double getParameter(const std::string &name) const override;
  /// Check if function has a parameter with this name.
  bool hasParameter(const std::string &name) const override;
  /// Total number of parameters
  size_t nParams() const override { return m_parameters.size(); }
  /// Returns the index of parameter name
  size_t parameterIndex(const std::string &name) const override;
  /// Returns the name of parameter i
  std::string parameterName(size_t i) const override;
  /// Returns the description of parameter i
  std::string parameterDescription(size_t i) const override;
  /// Checks if a parameter has been set explicitly
  bool isExplicitlySet(size_t i) const override;
  /// Get the fitting error for a parameter
  double getError(size_t i) const override;
  /// Set the fitting error for a parameter
  void setError(size_t i, double err) override;

  /// Return parameter index from a parameter reference. Usefull for constraints
  /// and ties in composite functions
  size_t getParameterIndex(const ParameterReference &ref) const override;
  /// Get the containing function
  IFunction_sptr getContainingFunction(const ParameterReference &ref) const;
  /// Get the containing function
  IFunction_sptr getContainingFunction(IFunction_sptr fun);

protected:
  /// Declare a new parameter
  void declareParameter(const std::string &name, double initValue = 0,
                        const std::string &description = "") override;
  /// Get the address of the parameter. For use in UserFunction with mu::Parser
  virtual double *getParameterAddress(size_t i);
  /// Nonvirtual member which removes all declared parameters
  void clearAllParameters();
  /// Change status of parameter
  void setParameterStatus(size_t i, ParameterStatus status) override;
  /// Get status of parameter
  ParameterStatus getParameterStatus(size_t i) const override;

private:
  /// Check that a parameter index is in a valid range.
  /// @param i :: Index to check.
  inline void checkParameterIndex(size_t i) const {
    if (i >= nParams()) {
      throw std::out_of_range("ParamFunction parameter index " +
                              std::to_string(i) + " out of range " +
                              std::to_string(nParams()));
    }
  }
  /// Keeps status for each parameter.
  std::vector<ParameterStatus> m_parameterStatus;
  /// Keeps parameter names
  std::vector<std::string> m_parameterNames;
  /// Keeps parameter values
  std::vector<double> m_parameters;
  /// Keeps parameter errors
  std::vector<double> m_errors;
  /// Flags of explicitly set parameters
  std::vector<bool> m_explicitlySet;
  /// parameter descriptions
  std::vector<std::string> m_parameterDescriptions;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PARAMFUNCTION_H_*/
