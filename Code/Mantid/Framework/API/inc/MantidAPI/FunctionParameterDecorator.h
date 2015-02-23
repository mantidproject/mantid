#ifndef MANTID_API_FUNCTIONPARAMETERDECORATOR_H_
#define MANTID_API_FUNCTIONPARAMETERDECORATOR_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IFunction.h"

namespace Mantid {
namespace API {

/** FunctionParameterDecorator

  FunctionParameterDecorator is an alternative to ParamFunction. Instead of
  storing the parameters itself, it stores an "internal function" and exposes
  the parameters and attributes of this function.

  A function that implements this interface can use the decorated function
  in its implementation of IFunction::function and IFunction::functionDeriv,
  for example to modify the values calculated by the function.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 23/02/2015

    Copyright © 2015 PSI-NXMM

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL FunctionParameterDecorator : virtual public IFunction {
public:
  FunctionParameterDecorator() : IFunction(), m_wrappedFunction() {}
  virtual ~FunctionParameterDecorator() {}

  void setDecoratedFunction(const std::string &wrappedFunctionName);
  IFunction_sptr getDecoratedFunction() const;

  /// Set i-th parameter
  virtual void setParameter(size_t i, const double &value,
                            bool explicitlySet = true);
  /// Set i-th parameter description
  virtual void setParameterDescription(size_t i,
                                       const std::string &description);
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
  virtual size_t nParams() const;
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

  /// Returns the number of attributes associated with the function
  virtual size_t nAttributes() const;
  /// Returns a list of attribute names
  virtual std::vector<std::string> getAttributeNames() const;
  /// Return a value of attribute attName
  virtual IFunction::Attribute getAttribute(const std::string &attName) const;
  /// Set a value to attribute attName
  virtual void setAttribute(const std::string &attName,
                            const IFunction::Attribute &attValue);
  /// Check if attribute attName exists
  virtual bool hasAttribute(const std::string &attName) const;

  /// Tie a parameter to other parameters (or a constant)
  virtual ParameterTie *tie(const std::string &parName, const std::string &expr,
                            bool isDefault = false);
  /// Apply the ties
  virtual void applyTies();
  /// Remove all ties
  virtual void clearTies();
  virtual void removeTie(const std::string &parName);
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
  void init() {}

  void throwIfNoFunctionSet() const;

  void declareParameter(const std::string &name, double initValue,
                        const std::string &description);

  virtual void addTie(ParameterTie *tie);

  IFunction_sptr m_wrappedFunction;
};

typedef boost::shared_ptr<FunctionParameterDecorator>
FunctionParameterDecorator_sptr;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_FUNCTIONPARAMETERDECORATOR_H_ */
