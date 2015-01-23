#ifndef MANTID_API_IFUNCTION1D_H_
#define MANTID_API_IFUNCTION1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidKernel/Logger.h"

namespace Mantid {

namespace CurveFitting {
class Fit;
}

namespace API {

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MatrixWorkspace;
class Jacobian;
class ParameterTie;
class IConstraint;
class ParameterReference;
class FunctionHandler;
/** This is a specialization of IFunction for functions of one real argument.
    It uses FunctionDomain1D as a domain. Implement function1D(...) method in a
   concrete
    function. Implement functionDeriv1D to use analytical derivatives.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009

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
class MANTID_API_DLL IFunction1D : public virtual IFunction {
public:
  /// Virtual destructor
  /// (avoids warnings about non-trivial move assignment in virtually inheriting
  /// classes)
  virtual ~IFunction1D() {}

  /* Overidden methods */

  virtual void function(const FunctionDomain &domain,
                        FunctionValues &values) const;
  virtual void derivative(const FunctionDomain &domain, FunctionValues &values,
                          const size_t order = 1) const;

  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian);

  /// Function you want to fit to.
  virtual void function1D(double *out, const double *xValues,
                          const size_t nData) const = 0;

  /// Function to calculate the derivatives of the data set
  virtual void derivative1D(double *out, const double *xValues,
                            const size_t nData, const size_t order) const;

  /// Derivatives of function with respect to active parameters
  virtual void functionDeriv1D(Jacobian *out, const double *xValues,
                               const size_t nData);

protected:
  /// Logger instance
  static Kernel::Logger g_log;

  /// Making a friend
  friend class CurveFitting::Fit;
};

typedef boost::shared_ptr<IFunction1D> IFunction1D_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IFUNCTION1D_H_*/
