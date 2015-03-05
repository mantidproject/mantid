#ifndef MANTID_CURVEFITTING_CONVOLUTION_H_
#define MANTID_CURVEFITTING_CONVOLUTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include <boost/shared_array.hpp>
#include <cmath>
#include <vector>

namespace Mantid {
namespace CurveFitting {
/**
Performes convolution of two functions.


@author Roman Tolchenov, Tessella plc
@date 28/01/2010

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
class DLLExport Convolution : public API::CompositeFunction {
public:

  /// Constructor
  Convolution();
  /// Destructor
  ~Convolution();

  /// overwrite IFunction base class methods
  std::string name() const { return "Convolution"; }
  virtual const std::string category() const { return "General"; }
  /// Function you want to fit to.
  /// @param domain :: The buffer for writing the calculated values. Must be big
  /// enough to accept dataSize() values
  virtual void function(const API::FunctionDomain &domain,
                        API::FunctionValues &values) const;
  /// Derivatives of function with respect to active parameters
  virtual void functionDeriv(const API::FunctionDomain &domain,
                             API::Jacobian &jacobian);

  /// Set a value to attribute attName
  virtual void setAttribute(const std::string &attName, const Attribute &);

  /// Add a function.
  size_t addFunction(API::IFunction_sptr f);
  /// Set up the function for a fit.
  void setUpForFit();

  /// Deletes and zeroes pointer m_resolution forsing function(...) to
  /// recalculate the resolution function
  void refreshResolution() const;

protected:
  /// overwrite IFunction base class method, which declare function parameters
  virtual void init();

private:
  /// To keep the Fourier transform of the resolution function (divided by the
  /// step in xValues)
  mutable std::vector<double> m_resolution;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CONVOLUTION_H_*/
