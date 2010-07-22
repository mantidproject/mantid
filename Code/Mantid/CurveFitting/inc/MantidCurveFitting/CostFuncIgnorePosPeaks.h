#ifndef MANTID_CURVEFITTING_COSTFUNCIGNOREPOSPEAKS_H_
#define MANTID_CURVEFITTING_COSTFUNCIGNOREPOSPEAKS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ICostFunction.h"

namespace Mantid
{
namespace CurveFitting
{
/** Cost function which allows positive peaks to be ignored and is suitable
    for e.g. fitting the background

    @author Anders Markvardsen, ISIS, RAL
    @date 12/05/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CostFuncIgnorePosPeaks : public ICostFunction 
{
public:
  /// Virtual destructor
  virtual ~CostFuncIgnorePosPeaks() {}

  /// Constructor
  CostFuncIgnorePosPeaks() : m_name("Ignore positive peaks") { }

  /// Get name of minimizer
  virtual std::string name() const { return m_name;}

  /// Calculate value of cost function from observed
  /// and calculated values
  virtual double val(const double* yData, const double* inverseError, double* yCal, const int& n);

  /// Calculate the derivatives of the cost function
  virtual void deriv(const double* yData, const double* inverseError, const double* yCal, 
                     const double* jacobian, double* outDerivs, const int& p, const int& n);

private:
  /// name of this minimizer
  const std::string m_name;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_COSTFUNCIGNOREPOSPEAKS_H_*/
