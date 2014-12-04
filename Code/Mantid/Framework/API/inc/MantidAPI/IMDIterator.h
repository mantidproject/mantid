#ifndef MANTID_API_IFMDITERATOR_H_
#define MANTID_API_IFMDITERATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IMDWorkspace.h"
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
  class MANTID_API_DLL IMDIterator
  {
  public:
    IMDIterator();
    virtual ~IMDIterator(){}

    void setNormalization(Mantid::API::MDNormalization normalization);
    Mantid::API::MDNormalization getNormalization() const;

    /// Get the size of the data (number of entries that will be iterated through)
    virtual size_t getDataSize() const = 0;

    /// Advance to the next cell. If the current cell is the last one in the workspace
    /// do nothing and return false.
    virtual bool next() = 0;

    /// Is the current position of the iterator valid?
    virtual bool valid() const = 0;

    /// Jump to the index^th cell.
    virtual void jumpTo(size_t index) = 0;

    /// Advance, skipping a certain number of cells.
    virtual bool next(size_t skip) = 0;

    /// Returns the normalized signal for this box
    virtual signal_t getNormalizedSignal() const = 0;

    /// Returns the normalized error for this box
    virtual signal_t getNormalizedError() const = 0;

    /// Returns the total signal for this box
    virtual signal_t getSignal() const = 0;

    /// Returns the total error for this box
    virtual signal_t getError() const = 0;

    /// Return a list of vertexes defining the volume pointed to
    virtual coord_t * getVertexesArray(size_t & numVertices) const = 0;

    /// Return a list of vertexes defining the volume pointed to, enable masking of dimensions.
    virtual coord_t * getVertexesArray(size_t & numVertices, const size_t outDimensions, const bool * maskDim) const = 0;

    /// Returns the position of the center of the box pointed to.
    virtual Mantid::Kernel::VMD getCenter() const = 0;

    /// Returns the number of events/points contained in this box
    virtual size_t getNumEvents() const = 0;

    /// For a given event/point in this box, return the run index
    virtual uint16_t getInnerRunIndex(size_t index) const = 0;

    /// For a given event/point in this box, return the detector ID
    virtual int32_t getInnerDetectorID(size_t index) const = 0;

    /// Returns the position of a given event for a given dimension
    virtual coord_t getInnerPosition(size_t index, size_t dimension) const = 0;

    /// Returns the signal of a given event
    virtual signal_t getInnerSignal(size_t index) const = 0;

    /// Returns the error of a given event
    virtual signal_t getInnerError(size_t index) const = 0;

    /// Returns true if masking is used.
    virtual bool getIsMasked() const = 0;

    /// Find neighbouring indexes vertex touching.
    virtual std::vector<size_t> findNeighbourIndexes() const = 0;

    /// Find neighbouring indexes face touching.
    virtual std::vector<size_t> findNeighbourIndexesFaceTouching() const = 0;

    /// Get the linear index.
    virtual size_t getLinearIndex() const = 0;

    /// Is index reachable by the iterator.
    virtual bool isWithinBounds(size_t index) const = 0;

  protected:
    /// Normalization method for getNormalizedSignal()
    Mantid::API::MDNormalization m_normalization;

  };


} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IFMDITERATOR_H_*/
