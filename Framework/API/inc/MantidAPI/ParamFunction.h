#ifndef MANTID_API_PARAMFUNCTION_H_
#define MANTID_API_PARAMFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/Unit.h"
#include "MantidAPI/IFunction.h"
#include <string>
#include <vector>

namespace Mantid {
namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Jacobian;
class ParameterTie;
class IConstraint;
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
  /// Virtual destructor
  ~ParamFunction() override;

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

  /// Check if a declared parameter i is active
  bool isFixed(size_t i) const override;
  /// Removes a declared parameter i from the list of active
  void fix(size_t i) override;
  /// Restores a declared parameter i to the active status
  void unfix(size_t i) override;

  /// Return parameter index from a parameter reference. Usefull for constraints
  /// and ties in composite functions
  size_t getParameterIndex(const ParameterReference &ref) const override;
  /// Get the containing function
  IFunction_sptr getContainingFunction(const ParameterReference &ref) const;
  /// Get the containing function
  IFunction_sptr getContainingFunction(IFunction_sptr fun);

  /// Apply the ties
  void applyTies() override;
  /// Remove all ties
  void clearTies() override;
  void removeTie(const std::string &parName) override {
    IFunction::removeTie(parName);
  }
  /// Removes i-th parameter's tie
  bool removeTie(size_t i) override;
  /// Get the tie of i-th parameter
  ParameterTie *getTie(size_t i) const override;
  /// Add a new tie
  void addTie(ParameterTie *tie) override;

  /// Add a constraint to function
  void addConstraint(IConstraint *ic) override;
  /// Get constraint of i-th parameter
  IConstraint *getConstraint(size_t i) const override;
  /// Remove a constraint
  void removeConstraint(const std::string &parName) override;
  /// Set parameters to satisfy constraints
  void setUpForFit() override;

protected:
  /// Declare a new parameter
  void declareParameter(const std::string &name, double initValue = 0,
                        const std::string &description = "") override;

  /// Get the address of the parameter. For use in UserFunction with mu::Parser
  virtual double *getParameterAddress(size_t i);

  /// Nonvirtual member which removes all declared parameters
  void clearAllParameters();

private:
  /// The index map. m_indexMap[i] gives the total index for active parameter i
  std::vector<bool> m_isFixed;
  /// Keeps parameter names
  std::vector<std::string> m_parameterNames;
  /// Keeps parameter values
  std::vector<double> m_parameters;
  /// Keeps parameter errors
  std::vector<double> m_errors;
  /// Holds parameter ties as <parameter index,tie pointer>
  std::vector<ParameterTie *> m_ties;
  /// Holds the constraints added to function
  std::vector<IConstraint *> m_constraints;
  /// Flags of explicitly set parameters
  std::vector<bool> m_explicitlySet;
  /// parameter descriptions
  std::vector<std::string> m_parameterDescriptions;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PARAMFUNCTION_H_*/
