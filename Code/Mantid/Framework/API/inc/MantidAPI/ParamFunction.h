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
  virtual ~ParamFunction();

  /// Set i-th parameter
  virtual void setParameter(size_t, const double &value,
                            bool explicitlySet = true);
  /// Set i-th parameter description
  virtual void setParameterDescription(size_t, const std::string &description);
  /// Get i-th parameter
  virtual double getParameter(size_t i) const;
  /// Set parameter by name.
  virtual void setParameter(const std::string &name, const double &value,
                            bool explicitlySet = true);
  /// Set description of parameter by name.
  virtual void setParameterDescription(const std::string &name,
                                       const std::string &description);
  /// Get parameter by name.
  virtual double getParameter(const std::string &name) const;
  /// Total number of parameters
  virtual size_t nParams() const { return m_parameters.size(); }
  /// Returns the index of parameter name
  virtual size_t parameterIndex(const std::string &name) const;
  /// Returns the name of parameter i
  virtual std::string parameterName(size_t i) const;
  /// Returns the description of parameter i
  virtual std::string parameterDescription(size_t i) const;
  /// Checks if a parameter has been set explicitly
  virtual bool isExplicitlySet(size_t i) const;
  /// Get the fitting error for a parameter
  virtual double getError(size_t i) const;
  /// Set the fitting error for a parameter
  virtual void setError(size_t i, double err);

  /// Check if a declared parameter i is active
  virtual bool isFixed(size_t i) const;
  /// Removes a declared parameter i from the list of active
  virtual void fix(size_t i);
  /// Restores a declared parameter i to the active status
  virtual void unfix(size_t i);

  /// Return parameter index from a parameter reference. Usefull for constraints
  /// and ties in composite functions
  virtual size_t getParameterIndex(const ParameterReference &ref) const;
  /// Get the containing function
  IFunction_sptr getContainingFunction(const ParameterReference &ref) const;
  /// Get the containing function
  IFunction_sptr getContainingFunction(IFunction_sptr fun);

  /// Apply the ties
  virtual void applyTies();
  /// Remove all ties
  virtual void clearTies();
  virtual void removeTie(const std::string &parName) {
    IFunction::removeTie(parName);
  }
  /// Removes i-th parameter's tie
  virtual bool removeTie(size_t i);
  /// Get the tie of i-th parameter
  virtual ParameterTie *getTie(size_t i) const;

  /// Add a constraint to function
  virtual void addConstraint(IConstraint *ic);
  /// Get constraint of i-th parameter
  virtual IConstraint *getConstraint(size_t i) const;
  /// Remove a constraint
  virtual void removeConstraint(const std::string &parName);
  /// Set parameters to satisfy constraints
  void setUpForFit();

protected:
  /// Declare a new parameter
  virtual void declareParameter(const std::string &name, double initValue = 0,
                                const std::string &description = "");

  /// Add a new tie
  virtual void addTie(ParameterTie *tie);
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
