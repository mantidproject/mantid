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
  // MSVC 2015 segfaults without this default constructor.
  // still segfaults with = default
  FunctionParameterDecorator() : IFunction(), m_wrappedFunction() {}
  void setDecoratedFunction(const std::string &wrappedFunctionName);
  IFunction_sptr getDecoratedFunction() const;

  IFunction_sptr clone() const override;

  void setWorkspace(boost::shared_ptr<const Workspace> ws) override;
  void setMatrixWorkspace(boost::shared_ptr<const MatrixWorkspace> workspace,
                          size_t wi, double startX, double endX) override;

  /// Set i-th parameter of decorated function.
  void setParameter(size_t i, const double &value,
                    bool explicitlySet = true) override;
  /// Set i-th parameter description of decorated function.
  void setParameterDescription(size_t i,
                               const std::string &description) override;
  /// Get i-th parameter of decorated function.
  double getParameter(size_t i) const override;
  /// Set parameter of decorated function by name.
  void setParameter(const std::string &name, const double &value,
                    bool explicitlySet = true) override;
  /// Set description of parameter of decorated function by name.
  void setParameterDescription(const std::string &name,
                               const std::string &description) override;

  /// Value of i-th active parameter of the decorated function.
  double activeParameter(size_t i) const override;
  /// Set new value of i-th active parameter of the decorated function.
  void setActiveParameter(size_t i, double value) override;

  /// Get parameter of decorated function by name.
  double getParameter(const std::string &name) const override;
  /// Total number of parameters of decorated function.
  size_t nParams() const override;
  /// Returns the index of parameter of decorated function name.
  size_t parameterIndex(const std::string &name) const override;
  /// Returns the name of parameter i of decorated function.
  std::string parameterName(size_t i) const override;
  /// Returns the description of parameter i of decorated function.
  std::string parameterDescription(size_t i) const override;
  /// Checks if a parameter of decorated function has been set explicitly
  bool isExplicitlySet(size_t i) const override;
  /// Get the fitting error for a parameter of decorated function.
  double getError(size_t i) const override;
  /// Set the fitting error for a parameter of decorated function.
  void setError(size_t i, double err) override;

  /// Return parameter index of decorated function from a parameter reference.
  /// Usefull for constraints and ties in composite functions.
  size_t getParameterIndex(const ParameterReference &ref) const override;

  /// Returns the number of attributes associated with the decorated function.
  size_t nAttributes() const override;
  /// Returns a list of attribute names of decorated function.
  std::vector<std::string> getAttributeNames() const override;
  /// Return a value of attribute attName of decorated function-
  IFunction::Attribute getAttribute(const std::string &attName) const override;
  /// Set a value to attribute attName of decorated function.
  void setAttribute(const std::string &attName,
                    const IFunction::Attribute &attValue) override;
  /// Check if attribute attName exists in decorated function
  bool hasAttribute(const std::string &attName) const override;

  /// Tie a parameter of decorated function to other parameters (or a constant).
  void tie(const std::string &parName, const std::string &expr,
           bool isDefault = false) override;
  /// Apply the ties in decorated function.
  void applyTies() override;
  /// Remove all ties of decorated function.
  void clearTies() override;
  void removeTie(const std::string &parName) override;
  /// Removes i-th parameter's of decorated function tie.
  bool removeTie(size_t i) override;
  /// Get the tie of i-th parameter of decorated function.
  ParameterTie *getTie(size_t i) const override;

  /// Add a constraint to decorated function.
  void addConstraint(std::unique_ptr<IConstraint> ic) override;
  /// Get constraint of i-th parameter of decorated function.
  IConstraint *getConstraint(size_t i) const override;
  /// Remove a constraint of decorated function.
  void removeConstraint(const std::string &parName) override;
  /// Set parameters of decorated function to satisfy constraints.
  void setUpForFit() override;

protected:
  /// Does nothing.
  void init() override {}

  void throwIfNoFunctionSet() const;

  void declareParameter(const std::string &name, double initValue,
                        const std::string &description) override;

  void addTie(std::unique_ptr<ParameterTie>) override;
  void setParameterStatus(size_t i, ParameterStatus status) override;
  ParameterStatus getParameterStatus(size_t i) const override;

  virtual void beforeDecoratedFunctionSet(const IFunction_sptr &fn);
  void setDecoratedFunctionPrivate(const IFunction_sptr &fn);

  IFunction_sptr m_wrappedFunction;
};

typedef boost::shared_ptr<FunctionParameterDecorator>
    FunctionParameterDecorator_sptr;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_FUNCTIONPARAMETERDECORATOR_H_ */
