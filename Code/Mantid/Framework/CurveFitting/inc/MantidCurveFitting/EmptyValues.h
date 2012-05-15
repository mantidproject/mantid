#ifndef MANTID_CURVEFITTING_EMPTYVALUES_H_
#define MANTID_CURVEFITTING_EMPTYVALUES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/IFunctionValues.h"

#include <stdexcept>

namespace Mantid
{
namespace CurveFitting
{
/** Base class for a storage of values of a function.

    @author Roman Tolchenov, Tessella plc
    @date 23/03/2012

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
class MANTID_CURVEFITTING_DLL EmptyValues: public API::IFunctionValues
{
public:
  /// Constructor
  EmptyValues(const size_t n):m_size(n){}
  /// Default constructor.
  virtual ~EmptyValues(){}
  /// Return the number of values
  virtual size_t size() const {return m_size;}
  /// Get a pointer to calculated data at index i
  virtual double* getPointerToCalculated(size_t)
  {throw std::runtime_error("EmptyValues don't contain actual values");}
  /// Set all calculated values to zero
  virtual void zeroCalculated(){}
  /// set all calculated values to same number
  virtual void setCalculated(double)
  {throw std::runtime_error("EmptyValues don't contain actual values");}

protected:
  /// Copy calculated values to a buffer
  /// @param to :: Pointer to the buffer, it must be large enough
  virtual void copyTo(double*) const
  {throw std::runtime_error("EmptyValues don't contain actual values");}
  /// Add calculated values to values in a buffer and save result to the buffer
  /// @param to :: Pointer to the buffer, it must be large enough
  virtual void add(double*) const
  {throw std::runtime_error("EmptyValues don't contain actual values");}
  /// Multiply calculated values by values in a buffer and save result to the buffer
  /// @param to :: Pointer to the buffer, it must be large enough
  virtual void multiply(double*) const
  {throw std::runtime_error("EmptyValues don't contain actual values");}
  /// Number of values
  size_t m_size;

};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_EMPTYVALUES_H_*/
