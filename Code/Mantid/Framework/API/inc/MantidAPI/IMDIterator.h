#ifndef MANTID_API_IFMDITERATOR_H_
#define MANTID_API_IFMDITERATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/VMD.h"
#include <vector>

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
  class MANTID_API_DLL IMDIterator
  {
  public:

  //=============================== REMOVE THESE SOON ==================================================
    /// Get the size of the data (number of entries that will be iterated through)
    virtual size_t getDataSize() const
    { throw std::runtime_error("IMDIterator: Not Implemented."); }

    /// Get the i-th coordinate of the current cell
    virtual double getCoordinate(std::size_t i) const
    { UNUSED_ARG(i); throw std::runtime_error("IMDIterator: Not Implemented."); }

    /// Return the current data pointer (index) into the MDWorkspace
    virtual size_t getPointer() const
    { throw std::runtime_error("IMDIterator: Not Implemented."); }
    //=============================== END REMOVE THESE SOON ==================================================


    /// Advance to the next cell. If the current cell is the last one in the workspace
    /// do nothing and return false.
    virtual bool next()
    { throw std::runtime_error("IMDIterator: Not Implemented."); }

    /// Returns the normalized signal for this box
    virtual signal_t getNormalizedSignal() const
    { throw std::runtime_error("IMDIterator: Not Implemented."); }

    /// Returns the normalized error for this box
    virtual signal_t getNormalizedError() const
    { throw std::runtime_error("IMDIterator: Not Implemented."); }

    /// Return a list of vertexes defining the volume pointed to
    virtual coord_t * getVertexesArray(size_t & numVertices) const
    { UNUSED_ARG(numVertices); throw std::runtime_error("IMDIterator: Not Implemented."); }


    /// Returns the number of events/points contained in this box
    virtual size_t getNumEvents() const
    { throw std::runtime_error("IMDIterator: Not Implemented."); }

    /// For a given event/point in this box, return the run index
    virtual uint16_t getInnerRunIndex(size_t index) const
    { UNUSED_ARG(index); throw std::runtime_error("IMDIterator: Not Implemented."); }

    /// For a given event/point in this box, return the detector ID
    virtual int32_t getInnerDetectorID(size_t index) const
    { UNUSED_ARG(index); throw std::runtime_error("IMDIterator: Not Implemented."); }

    /// Returns the position of a given event for a given dimension
    virtual size_t getInnerPosition(size_t index, size_t dimension) const
    { UNUSED_ARG(index); UNUSED_ARG(dimension); throw std::runtime_error("IMDIterator: Not Implemented."); }

    /// Returns the signal of a given event
    virtual signal_t getInnerSignal(size_t index)
    { UNUSED_ARG(index); throw std::runtime_error("IMDIterator: Not Implemented."); }

    /// Returns the error of a given event
    virtual signal_t getInnerError(size_t index)
    { UNUSED_ARG(index); throw std::runtime_error("IMDIterator: Not Implemented."); }


    //=======================================================================================

//    /// Return a list of vertexes defining the volume pointed to
//    virtual std::vector<Mantid::Kernel::VMD> getVertexes() const
//    { UNUSED_ARG(index); throw std::runtime_error("IMDIterator: Not Implemented."); }

  };


} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IFMDITERATOR_H_*/
