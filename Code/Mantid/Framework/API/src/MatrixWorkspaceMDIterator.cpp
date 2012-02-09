#include "MantidAPI/MatrixWorkspaceMDIterator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include "MantidKernel/cow_ptr.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MatrixWorkspaceMDIterator::MatrixWorkspaceMDIterator(const MatrixWorkspace * workspace, Mantid::Geometry::MDImplicitFunction * function)
  : m_ws(workspace), m_pos(0), m_max(0), m_function(function)
  {
    if (!m_ws)
      throw std::runtime_error("MatrixWorkspaceMDIterator::ctor() NULL MatrixWorkspace");
    m_max = m_ws->size();
    m_center = VMD(2);
    m_isBinnedData = m_ws->isHistogramData();
    m_dimY = m_ws->getDimension(1);
    calcWorkspacePos();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MatrixWorkspaceMDIterator::~MatrixWorkspaceMDIterator()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** @return the number of points to be iterated on */
  size_t MatrixWorkspaceMDIterator::getDataSize() const
  {
    return size_t(m_max);
  }

  //----------------------------------------------------------------------------------------------
  /** Jump to the index^th cell.
   *  No range checking is performed, for performance reasons!
   *
   * @param index :: point to jump to. Must be 0 <= index < getDataSize().
   */
  void MatrixWorkspaceMDIterator::jumpTo(size_t index)
  {
    m_pos = uint64_t(index);
    calcWorkspacePos();
  }

  /// Calculate the workspace index/x index for this iterator position
  void MatrixWorkspaceMDIterator::calcWorkspacePos()
  {
    size_t blocksize = m_ws->blocksize();
    m_workspaceIndex = m_pos / blocksize;
    m_xIndex = m_pos % blocksize;
    if (m_workspaceIndex < m_ws->getNumberHistograms())
    {
      const MantidVec & X = m_ws->readX(m_workspaceIndex);
      m_Y = m_ws->readY(m_workspaceIndex)[m_xIndex];
      m_E = m_ws->readE(m_workspaceIndex)[m_xIndex];
      // Place the point
      if (m_isBinnedData)
        m_center[0] = (X[m_xIndex] + X[m_xIndex+1]) / 2.0;
      else
        m_center[0] = X[m_xIndex];
      m_center[1] = m_dimY->getX(m_workspaceIndex);
    }
  }

  //----------------------------------------------------------------------------------------------
  /** @return true if the iterator is valid. Check this at the start of an iteration,
   * in case the very first point is not valid.
   */
  bool MatrixWorkspaceMDIterator::valid() const
  {
    return (m_pos < m_max);
  }

  //----------------------------------------------------------------------------------------------
  /// Advance to the next cell. If the current cell is the last one in the workspace
  /// do nothing and return false.
  /// @return true if you can continue iterating
  bool MatrixWorkspaceMDIterator::next()
  {
    if (m_function)
    {
      do
      {
        m_pos++;
        calcWorkspacePos();
        // Keep incrementing until you are in the implicit function
      } while (!m_function->isPointContained(m_center)
               && m_pos < m_max);
      // Is the iteration finished?
      return (m_pos < m_max);
    }
    else
    {
      // Go through every point;
      m_pos++;
      calcWorkspacePos();
      return (m_pos < m_max);
    }
  }

  //----------------------------------------------------------------------------------------------
  /// Advance, skipping a certain number of cells.
  /// @param skip :: how many to increase. If 1, then every point will be sampled.
  bool MatrixWorkspaceMDIterator::next(size_t skip)
  {
    m_pos += skip;
    calcWorkspacePos();
    return (m_pos < m_max);
  }

  //----------------------------------------------------------------------------------------------
  /// Returns the normalized signal for this box
  signal_t MatrixWorkspaceMDIterator::getNormalizedSignal() const
  {
    // TODO: Normalize
    return this->getSignal();
  }

  /// Returns the normalized error for this box
  signal_t MatrixWorkspaceMDIterator::getNormalizedError() const
  {
    // TODO: Normalize
    return this->getError();
  }

  /// Returns the signal for this box, same as innerSignal
  signal_t MatrixWorkspaceMDIterator::getSignal() const
  {
    return m_Y;
  }

  /// Returns the error for this box, same as innerError
  signal_t MatrixWorkspaceMDIterator::getError() const
  {
    return m_E;
  }

  //----------------------------------------------------------------------------------------------
  /// Return a list of vertexes defining the volume pointed to
  coord_t * MatrixWorkspaceMDIterator::getVertexesArray(size_t & numVertices) const
  {
    throw std::runtime_error("MatrixWorkspaceMDIterator::getVertexesArray() not implemented yet");
  }

  coord_t * MatrixWorkspaceMDIterator::getVertexesArray(size_t & numVertices, const size_t outDimensions, const bool * maskDim) const
  {
    throw std::runtime_error("MatrixWorkspaceMDIterator::getVertexesArray() not implemented yet");
  }

  //----------------------------------------------------------------------------------------------
  /// Returns the position of the center of the box pointed to.
  Mantid::Kernel::VMD MatrixWorkspaceMDIterator::getCenter() const
  {
    return m_center;
  }


  //----------------------------------------------------------------------------------------------
  /// Returns the number of events/points contained in this box
  /// @return 1 always: e.g. there is one (fake) event in the middle of the box.
  size_t MatrixWorkspaceMDIterator::getNumEvents() const
  {
    return 1;
  }

  //----------------------------------------------------------------------------------------------
  /// For a given event/point in this box, return the run index
  uint16_t MatrixWorkspaceMDIterator::getInnerRunIndex(size_t /*index*/) const
  {
    return 0;
  }

  /// For a given event/point in this box, return the detector ID
  int32_t MatrixWorkspaceMDIterator::getInnerDetectorID(size_t /*index*/) const
  {
    return 0;
  }

  /// Returns the position of a given event for a given dimension
  coord_t MatrixWorkspaceMDIterator::getInnerPosition(size_t /*index*/, size_t dimension) const
  {
    return m_center[dimension];
  }

  /// Returns the signal of a given event
  signal_t MatrixWorkspaceMDIterator::getInnerSignal(size_t /*index*/) const
  {
    return this->getSignal();
  }

  /// Returns the error of a given event
  signal_t MatrixWorkspaceMDIterator::getInnerError(size_t /*index*/) const
  {
    return this->getError();
  }


} // namespace Mantid
} // namespace API
