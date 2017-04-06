#ifndef MANTID_CURVEFITTING_USERFUNCTION_H_
#define MANTID_CURVEFITTING_USERFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include <boost/shared_array.hpp>

namespace mu {
class Parser;
}

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
A user defined function.

@author Roman Tolchenov, Tessella plc
@date 15/01/2010

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
class DLLExport UserFunction : public API::ParamFunction,
                               public API::IFunction1D {
public:
  /// Constructor
  UserFunction();
  /// Destructor
  ~UserFunction() override;

  /// Returns the function's name
  std::string name() const override { return "UserFunction"; }
  // Returns Category
  const std::string category() const override { return "General"; }

  /// Function you want to fit to.
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  /// Derivatives of function with respect to active parameters
  void functionDeriv(const API::FunctionDomain &domain,
                     API::Jacobian &jacobian) override;

  /// Returns the number of attributes associated with the function
  size_t nAttributes() const override { return 1; }
  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const override {
    return std::vector<std::string>(1, "Formula");
  }
  /// Return a value of attribute attName
  Attribute getAttribute(const std::string &attName) const override {
    return attName == "Formula" ? Attribute(m_formula) : getAttribute(attName);
  }
  /// Set a value to attribute attName
  void setAttribute(const std::string &attName,
                    const Attribute &value) override;
  /// Check if attribute attName exists
  bool hasAttribute(const std::string &attName) const override {
    return attName == "Formula";
  }

private:
  /// The formula
  std::string m_formula;
  /// extended muParser instance
  mu::Parser *m_parser;
  /// Used as 'x' variable in m_parser.
  mutable double m_x;
  /// True indicates that input formula contains 'x' variable
  bool m_x_set;
  /// Temporary data storage used in functionDeriv
  mutable boost::shared_array<double> m_tmp;
  /// Temporary data storage used in functionDeriv
  mutable boost::shared_array<double> m_tmp1;

  /// mu::Parser callback function for setting variables.
  static double *AddVariable(const char *varName, void *pufun);
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_USERFUNCTION_H_*/
