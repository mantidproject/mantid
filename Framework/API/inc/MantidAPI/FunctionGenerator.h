#ifndef MANTID_API_FUNCTIONGENERATOR_H_
#define MANTID_API_FUNCTIONGENERATOR_H_

#include "MantidAPI/IFunction.h"

namespace Mantid {
namespace API {
/**
FunctionGenerator is a partial implementation of IFunction that defines a
function consisting of two parts: the source and the target. The source
function generates the target function which in turn is used to calculate
the output. Concrete functions subclassing FunctionGenerator must implement
the following virtual methods:

  name()
  category()
  buildTargetFunction()
  updateTargetFunction()

Parameters and attributes of both source and target functions become
parameters (attributes) of FunctionGenerator without changing names.
Virtual method isSourceName(name) decides to which function a parameter
belongs to. By default if a name has the signature of a composite function
(f<number>.name) then it is attributed to the target function.

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport FunctionGenerator : public IFunction {
public:
  /// Constructor
  FunctionGenerator(IFunction_sptr source);

  /// @name Overrides implementing composition of two functions:
  /// m_source and m_target.
  //@{
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
  size_t nParams() const override;
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

  /// Return parameter index from a parameter reference.
  size_t getParameterIndex(const ParameterReference &ref) const override;
  /// Set up the function for a fit.
  void setUpForFit() override;
  /// Get the tie for i-th parameter
  ParameterTie *getTie(size_t i) const override;
  /// Get the i-th constraint
  IConstraint *getConstraint(size_t i) const override;

  /// Build target function.
  virtual void buildTargetFunction() const = 0;

protected:
  /// Declare a new parameter
  void declareParameter(const std::string &name, double initValue = 0,
                        const std::string &description = "") override;
  /// Change status of parameter
  void setParameterStatus(size_t i, ParameterStatus status) override;
  /// Get status of parameter
  ParameterStatus getParameterStatus(size_t i) const override;
  //@}

public:
  /** @name Attributes */
  //@{
  /// Returns the number of attributes associated with the function
  size_t nAttributes() const override;
  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const override;
  /// Return a value of attribute attName
  Attribute getAttribute(const std::string &name) const override;
  /// Set a value to attribute attName
  void setAttribute(const std::string &name, const Attribute &) override;
  /// Check if attribute attName exists
  bool hasAttribute(const std::string &name) const override;
  //@}

  /// Evaluate the function
  void function(const FunctionDomain &domain,
                FunctionValues &values) const override;

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

  /// Test if a name (parameter's or attribute's) belongs to m_source
  virtual bool isSourceName(const std::string &aName) const;
  /// Update target function.
  virtual void updateTargetFunction() const = 0;
  /// Update target function if necessary.
  void checkTargetFunction() const;
  /// Function that calculates parameters of the target function.
  IFunction_sptr m_source;
  /// Function that actually calculates the output.
  mutable IFunction_sptr m_target;
  /// Cached number of parameters in m_source.
  size_t m_nOwnParams;
  /// Flag indicating that updateTargetFunction() is required.
  mutable bool m_dirty;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONGENERATOR_H_*/
