#ifndef MANTID_CURVEFITTING_CHEBYSHEV_H_
#define MANTID_CURVEFITTING_CHEBYSHEV_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BackgroundFunction.h"
#include <valarray>

namespace Mantid {
namespace CurveFitting {
/**
Implements Chebyshev polynomial expansion.

Attributes: int n - the highest polynomial order.
Parameters: n+1 expansion coefficients \f$a_i\f$ as in expression:
\f$\sum_{i=0}^{n} a_i T_i(x)\f$

Uses the Clenshaw algorithm to evaluate the expansion.

@author Roman Tolchenov, Tessella inc
@date 14/05/2010

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
class DLLExport Chebyshev : public BackgroundFunction {
public:
  /// Constructor
  Chebyshev();
  /// Destructor
  ~Chebyshev(){};

  /// overwrite IFunction base class methods
  std::string name() const { return "Chebyshev"; }
  virtual const std::string category() const { return "Background"; }
  void function1D(double *out, const double *xValues, const size_t nData) const;
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData);

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &);

private:
  /// Polynomial order
  int m_n;
  /// Lower x boundary. The default is -1
  double m_StartX;
  /// Upper x boundary. The default is  1
  double m_EndX;
  /// Keep intermediate calculatons
  mutable std::valarray<double> m_b;
};

typedef boost::shared_ptr<Chebyshev> Chebyshev_sptr;

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CHEBYSHEV_H_*/
