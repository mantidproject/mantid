#ifndef MANTID_CURVEFITTING_PARDOMAIN_H_
#define MANTID_CURVEFITTING_PARDOMAIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/SeqDomain.h"

namespace Mantid
{
namespace CurveFitting
{
/** 
    An implementation of SeqDomain for parallel cost function and derivatives computation.

    @author Roman Tolchenov, Tessella plc

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class MANTID_CURVEFITTING_DLL ParDomain: public SeqDomain
{
public:
  ParDomain():SeqDomain(){}
  /// Create and return i-th domain and i-th values, (i-1)th domain is released.
  virtual void getDomainAndValues(size_t i, API::FunctionDomain_sptr& domain, API::FunctionValues_sptr& values) const;
  /// Calculate the value of a least squares cost function
  virtual void leastSquaresVal(const CostFuncLeastSquares& leastSquares);
  /// Calculate the value, first and second derivatives of a least squares cost function
  virtual void leastSquaresValDerivHessian(const CostFuncLeastSquares& leastSquares, bool evalFunction, bool evalDeriv, bool evalHessian);
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_PARDOMAIN_H_*/
