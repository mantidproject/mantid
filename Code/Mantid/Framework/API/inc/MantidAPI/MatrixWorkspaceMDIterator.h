#ifndef MANTID_API_MATRIXWORKSPACEMDITERATOR_H_
#define MANTID_API_MATRIXWORKSPACEMDITERATOR_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"


namespace Mantid
{
namespace API
{

  /** IMDIterator-compatible implementation of an iterator
   * through a MatrixWorkspace
    
    @date 2012-02-08

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport MatrixWorkspaceMDIterator : public IMDIterator
  {
  public:
    MatrixWorkspaceMDIterator(const MatrixWorkspace * workspace, Mantid::Geometry::MDImplicitFunction * function,
        size_t beginWI = 0, size_t endWI = size_t(-1));
    virtual ~MatrixWorkspaceMDIterator();
    virtual size_t getDataSize() const;

    virtual bool valid() const;

    virtual bool next();

    virtual bool next(size_t skip);

    virtual void jumpTo(size_t index);

    virtual signal_t getNormalizedSignal() const;

    virtual signal_t getNormalizedError() const;

    virtual signal_t getSignal() const;

    virtual signal_t getError() const;

    virtual coord_t * getVertexesArray(size_t & numVertices) const;

    virtual coord_t * getVertexesArray(size_t & numVertices, const size_t outDimensions, const bool * maskDim) const;

    virtual Mantid::Kernel::VMD getCenter() const;

    virtual size_t getNumEvents() const;

    virtual uint16_t getInnerRunIndex(size_t index) const;

    virtual int32_t getInnerDetectorID(size_t index) const;

    virtual coord_t getInnerPosition(size_t index, size_t dimension) const;

    virtual signal_t getInnerSignal(size_t index) const;

    virtual signal_t getInnerError(size_t index) const;

    virtual bool getIsMasked() const;

    virtual std::vector<size_t> findNeighbourIndexes() const;

    virtual std::vector<size_t> findNeighbourIndexesFaceTouching() const;

    virtual size_t getLinearIndex() const;

    virtual bool isWithinBounds(size_t index) const;

  private:
    void calcWorkspacePos(size_t newWI);

    /// Workspace being iterated
    const MatrixWorkspace * m_ws;

    /// The linear position/index into the MDHistoWorkspace.
    uint64_t m_pos;

    /// The maximum linear index in the workspace
    uint64_t m_max;

    /// Implicit function to limit volume searched
    Mantid::Geometry::MDImplicitFunction * m_function;

    /// Workspace index of the spectrum we are looking at
    size_t m_workspaceIndex;

    /// x-index, index into the Y[] data array of the spectrum.
    size_t m_xIndex;

    /// Coordinates of the center at the current iterator pos
    mutable Mantid::Kernel::VMD m_center;

    /// Cached copies of X,Y,E at current workspace index
    MantidVec m_X;
    MantidVec m_Y;
    mutable MantidVec m_E;

    /// Error vector has been cached?
    mutable bool m_errorIsCached;

    /// Is the matrix workspace binned (true) e.g. Y vector is 1 shorter than X
    bool m_isBinnedData;

    /// The Y (vertical, e.g. spectra) dimension
    Mantid::Geometry::IMDDimension_const_sptr m_dimY;

    /// Blocksize of workspace
    size_t m_blockSize;

    /// Workspace index at which the iterator begins
    size_t m_beginWI;

    /// Workspace index at which the iterator ends
    size_t m_endWI;

    /// For numeric axes, this is the size of the bin in the vertical direction.
    /// It is 1.0 for spectrum axes
    double m_verticalBinSize;
  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_MATRIXWORKSPACEMDITERATOR_H_ */
