#ifndef MANTID_API_IFUNCTIONVALUES_H_
#define MANTID_API_IFUNCTIONVALUES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"

#include <vector>

namespace Mantid
{
namespace API
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
class MANTID_API_DLL IFunctionValues
{
public:

  /// Default constructor.
  virtual ~IFunctionValues(){}
  /// Return the number of values
  virtual size_t size() const  = 0;
  /// Get a pointer to calculated data at index i
  virtual double* getPointerToCalculated(size_t i) = 0;
  /// Set all calculated values to zero
  virtual void zeroCalculated() = 0;
  /// set all calculated values to same number
  virtual void setCalculated(double value) = 0;

protected:
  /// Copy calculated values to a buffer
  /// @param to :: Pointer to the buffer, it must be large enough
  virtual void copyTo(double* to) const = 0;
  /// Add calculated values to values in a buffer and save result to the buffer
  /// @param to :: Pointer to the buffer, it must be large enough
  virtual void add(double* to) const = 0;
  /// Multiply calculated values by values in a buffer and save result to the buffer
  /// @param to :: Pointer to the buffer, it must be large enough
  virtual void multiply(double* to) const = 0;

};

/// typedef for a shared pointer
typedef boost::shared_ptr<IFunctionValues> IFunctionValues_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IFUNCTIONVALUES_H_*/
