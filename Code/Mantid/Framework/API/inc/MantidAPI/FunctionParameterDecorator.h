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

  IFunction_sptr clone() const;

  virtual void setWorkspace(boost::shared_ptr<const Workspace> ws);
  virtual void
  setMatrixWorkspace(boost::shared_ptr<const MatrixWorkspace> workspace,
                     size_t wi, double startX, double endX);

  /// Set i-th parameter of decorated function.
  virtual void setParameter(size_t i, const double &value,
                            bool explicitlySet = true);
  /// Set i-th parameter description of decorated function.
  virtual void setParameterDescription(size_t i,
                                       const std::string &description);
  /// Get i-th parameter of decorated function.
  virtual double getParameter(size_t i) const;
  /// Set parameter of decorated function by name.
  virtual void setParameter(const std::string &name, const double &value,
                            bool explicitlySet = true);
  /// Set description of parameter of decorated function by name.
  virtual void setParameterDescription(const std::string &name,
                                       const std::string &description);

  /// Value of i-th active parameter of the decorated function.
  virtual double activeParameter(size_t i) const;
  /// Set new value of i-th active parameter of the decorated function.
  virtual void setActiveParameter(size_t i, double value);

  /// Get parameter of decorated function by name.
  virtual double getParameter(const std::string &name) const;
  /// Total number of parameters of decorated function.
  virtual size_t nParams() const;
  /// Returns the index of parameter of decorated function name.
  virtual size_t parameterIndex(const std::string &name) const;
  /// Returns the name of parameter i of decorated function.
  virtual std::string parameterName(size_t i) const;
  /// Returns the description of parameter i of decorated function.
  virtual std::string parameterDescription(size_t i) const;
  /// Checks if a parameter of decorated function has been set explicitly
  virtual bool isExplicitlySet(size_t i) const;
  /// Get the fitting error for a parameter of decorated function.
  virtual double getError(size_t i) const;
  /// Set the fitting error for a parameter of decorated function.
  virtual void setError(size_t i, double err);

  /// Check if a declared parameter i of decorated function is active.
  virtual bool isFixed(size_t i) const;
  /// Removes a declared parameter i of decorated function from the list of
  /// active.
  virtual void fix(size_t i);
  /// Restores a declared parameter i of decorated function to the active
  /// status.
  virtual void unfix(size_t i);

  /// Return parameter index of decorated function from a parameter reference.
  /// Usefull for constraints and ties in composite functions.
  virtual size_t getParameterIndex(const ParameterReference &ref) const;

  /// Returns the number of attributes associated with the decorated function.
  virtual size_t nAttributes() const;
  /// Returns a list of attribute names of decorated function.
  virtual std::vector<std::string> getAttributeNames() const;
  /// Return a value of attribute attName of decorated function-
  virtual IFunction::Attribute getAttribute(const std::string &attName) const;
  /// Set a value to attribute attName of decorated function.
  virtual void setAttribute(const std::string &attName,
                            const IFunction::Attribute &attValue);
  /// Check if attribute attName exists in decorated function
  virtual bool hasAttribute(const std::string &attName) const;

  /// Tie a parameter of decorated function to other parameters (or a constant).
  virtual ParameterTie *tie(const std::string &parName, const std::string &expr,
                            bool isDefault = false);
  /// Apply the ties in decorated function.
  virtual void applyTies();
  /// Remove all ties of decorated function.
  virtual void clearTies();
  virtual void removeTie(const std::string &parName);
  /// Removes i-th parameter's of decorated function tie.
  virtual bool removeTie(size_t i);
  /// Get the tie of i-th parameter of decorated function.
  virtual ParameterTie *getTie(size_t i) const;

  /// Add a constraint to decorated function.
  virtual void addConstraint(IConstraint *ic);
  /// Get constraint of i-th parameter of decorated function.
  virtual IConstraint *getConstraint(size_t i) const;
  /// Remove a constraint of decorated function.
  virtual void removeConstraint(const std::string &parName);
  /// Set parameters of decorated function to satisfy constraints.
  void setUpForFit();

protected:
  /// Does nothing.
  void init() {}

  void throwIfNoFunctionSet() const;

  void declareParameter(const std::string &name, double initValue,
                        const std::string &description);

  virtual void addTie(ParameterTie *tie);

  virtual void beforeDecoratedFunctionSet(const IFunction_sptr &fn);
  void setDecoratedFunctionPrivate(const IFunction_sptr &fn);

  IFunction_sptr m_wrappedFunction;
};

typedef boost::shared_ptr<FunctionParameterDecorator>
FunctionParameterDecorator_sptr;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_FUNCTIONPARAMETERDECORATOR_H_ */
