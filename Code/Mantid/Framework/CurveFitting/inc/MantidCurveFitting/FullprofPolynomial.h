#ifndef MANTID_CURVEFITTING_FULLPROFPOLYNOMIAL_H_
#define MANTID_CURVEFITTING_FULLPROFPOLYNOMIAL_H_

#include "MantidKernel/System.h"
#include "MantidCurveFitting/BackgroundFunction.h"

namespace Mantid {
namespace CurveFitting {

/** FullprofPolynomial : Polynomial background defined in Fullprof

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport FullprofPolynomial : public BackgroundFunction {
public:
  FullprofPolynomial();
  virtual ~FullprofPolynomial();

  /// Overwrite IFunction base class
  std::string name() const { return "FullprofPolynomial"; }

  virtual const std::string category() const { return "Background"; }

  virtual void function1D(double *out, const double *xValues,
                          const size_t nData) const;

  virtual void functionDeriv1D(API::Jacobian *out, const double *xValues,
                               const size_t nData);

  // virtual void functionLocal(std::vector<double> &out, std::vector<double>
  // xValues) const;

  /// Returns the number of attributes associated with the function (polynomial
  /// order n)
  size_t nAttributes() const { return 1; }

  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const;

  /// Return a value of attribute attName
  Attribute getAttribute(const std::string &attName) const;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &);

  /// Check if attribute attName exists
  bool hasAttribute(const std::string &attName) const;

private:
  /// Polynomial order
  int m_n;

  /// Background origin position
  double m_bkpos;
};

typedef boost::shared_ptr<FullprofPolynomial> FullprofPolynomial_sptr;

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_FULLPROFPOLYNOMIAL_H_ */
