#ifndef MANTID_API_FUNCTIONDOMAIN_H_
#define MANTID_API_FUNCTIONDOMAIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace API
{
/** Abstract class that represents the domain of a function.
    A domain is a generalisation of x (argument) and y (value) arrays.
    A domain consists at least of a list of function arguments for which a function should 
    be evaluated and a buffer for the calculated values. If used in fitting also contains
    the fit data and weights.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class MANTID_API_DLL FunctionDomain
{
public:
  /// Return the number of points, values, etc in the domain
  virtual size_t size() const = 0;
  /// store i-th calculated value. 0 <= i < size()
  virtual void setCalculated(size_t i,double value) = 0;
  /// get i-th calculated value. 0 <= i < size()
  virtual double getCalculated(size_t i) const = 0;
  /// Virtual destructor
  virtual ~FunctionDomain(){}
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONDOMAIN_H_*/
