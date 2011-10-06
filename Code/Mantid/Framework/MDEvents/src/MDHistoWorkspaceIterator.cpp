#include "MantidMDEvents/MDHistoWorkspaceIterator.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param workspace :: MDHistoWorkspace_sptr being iterated
   * @return
   */
  MDHistoWorkspaceIterator::MDHistoWorkspaceIterator(MDHistoWorkspace_sptr workspace)
  : m_ws(workspace), m_pos(0)
  {
    if (!m_ws)
      throw std::invalid_argument("MDHistoWorkspaceIterator::ctor(): NULL workspace given.");
    m_max = m_ws->getNPoints();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDHistoWorkspaceIterator::~MDHistoWorkspaceIterator()
  {
  }
  


  //----------------------------------------------------------------------------------------------
  /// Advance to the next cell. If the current cell is the last one in the workspace
  /// do nothing and return false.
  /// @return true if you can continue iterating
  bool MDHistoWorkspaceIterator::next()
  {
    return (++m_pos < m_max);
  }

  //----------------------------------------------------------------------------------------------
  /// Returns the normalized signal for this box
  signal_t MDHistoWorkspaceIterator::getNormalizedSignal() const
  {
    return m_ws->getSignalNormalizedAt(m_pos);
  }

  /// Returns the normalized error for this box
  signal_t MDHistoWorkspaceIterator::getNormalizedError() const
  {
    return m_ws->getErrorNormalizedAt(m_pos);
  }

  //----------------------------------------------------------------------------------------------
  /// Return a list of vertexes defining the volume pointed to
  coord_t * MDHistoWorkspaceIterator::getVertexesArray(size_t & numVertices) const
  {
    // The MDHistoWorkspace takes care of this
    return m_ws->getVertexesArray(m_pos, numVertices);
  }


  //----------------------------------------------------------------------------------------------
  /// Returns the number of events/points contained in this box
  size_t MDHistoWorkspaceIterator::getNumEvents() const
  {
    // There are no events contained - this is a binned workspace
    return 0;
  }

  //----------------------------------------------------------------------------------------------
  /// For a given event/point in this box, return the run index
  uint16_t MDHistoWorkspaceIterator::getInnerRunIndex(size_t index) const
  {
    UNUSED_ARG(index);
    throw std::runtime_error("MDHistoWorkspaceIterator: No events are contained, so it is not possible to return inner run index.");
  }

  /// For a given event/point in this box, return the detector ID
  int32_t MDHistoWorkspaceIterator::getInnerDetectorID(size_t index) const
  {
    UNUSED_ARG(index);
    throw std::runtime_error("MDHistoWorkspaceIterator: No events are contained, so it is not possible to return inner detector ID.");
  }

  /// Returns the position of a given event for a given dimension
  size_t MDHistoWorkspaceIterator::getInnerPosition(size_t index, size_t dimension) const
  {
    UNUSED_ARG(index); UNUSED_ARG(dimension);
    throw std::runtime_error("MDHistoWorkspaceIterator: No events are contained, so it is not possible to return inner position.");
  }

  /// Returns the signal of a given event
  signal_t MDHistoWorkspaceIterator::getInnerSignal(size_t index) const
  {
    UNUSED_ARG(index);
    throw std::runtime_error("MDHistoWorkspaceIterator: No events are contained, so it is not possible to return inner signal.");
  }

  /// Returns the error of a given event
  signal_t MDHistoWorkspaceIterator::getInnerError(size_t index) const
  {
    UNUSED_ARG(index);
    throw std::runtime_error("MDHistoWorkspaceIterator: No events are contained, so it is not possible to return inner error.");
  }

} // namespace Mantid
} // namespace MDEvents

