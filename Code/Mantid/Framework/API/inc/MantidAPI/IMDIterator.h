#ifndef MANTID_API_IFMDITERATOR_H_
#define MANTID_API_IFMDITERATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

namespace Mantid
{

namespace API
{
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class IMDWorkspace;

/** This is an interface to an iterator of an IMDWorkspace

    @author Roman Tolchenov, Tessella Support Services plc
    @date 15/03/2011

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
  class DLLExport IMDIterator
  {
  public:
    /// Get the size of the data
    virtual size_t getDataSize()const = 0;
    /// Get the i-th coordinate of the current cell
    virtual double getCoordinate(int i)const = 0;
    /// Advance to the next cell. If the current cell is the last one in the workspace
    /// do nothing and return false.
    virtual bool next() = 0;
    ///< return the current data pointer (index)
    virtual size_t getPointer()const = 0;
  };


} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IFMDITERATOR_H_*/
