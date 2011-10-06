#include "MantidMDEvents/MDHistoWorkspaceIterator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"

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
  MDHistoWorkspaceIterator::MDHistoWorkspaceIterator(MDHistoWorkspace_const_sptr workspace)
  : m_ws(workspace.get()), m_pos(0)
  {
    if (m_ws == NULL)
      throw std::invalid_argument("MDHistoWorkspaceIterator::ctor(): NULL workspace given.");
    m_max = m_ws->getNPoints();
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param workspace :: MDHistoWorkspace_sptr being iterated
   * @return
   */
  MDHistoWorkspaceIterator::MDHistoWorkspaceIterator(const MDHistoWorkspace * workspace)
  : m_ws(workspace), m_pos(0)
  {
    if (m_ws == NULL)
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
  /** @return the number of points to be iterated on */
  size_t MDHistoWorkspaceIterator::getDataSize() const
  {
    return size_t(m_max);
  }


  //----------------------------------------------------------------------------------------------
  /** Jump to the index^th cell.
   *  No range checking is performed, for performance reasons!
   *
   * @param index :: point to jump to. Must be 0 <= index < getDataSize().
   */
  void MDHistoWorkspaceIterator::jumpTo(size_t index)
  {
    m_pos = uint64_t(index);
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
  /// Advance, skipping a certain number of cells.
  /// @param skip :: how many to increase. If 1, then every point will be sampled.
  bool MDHistoWorkspaceIterator::next(size_t skip)
  {
    m_pos += skip;
    return (m_pos < m_max);
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
  /// Returns the position of the center of the box pointed to.
  Mantid::Kernel::VMD MDHistoWorkspaceIterator::getCenter() const
  {
    return VMD();
  }


  //----------------------------------------------------------------------------------------------
  /// Returns the number of events/points contained in this box
  /// @return 1 always: e.g. there is one (fake) event in the middle of the box.
  size_t MDHistoWorkspaceIterator::getNumEvents() const
  {
    return 1;
  }

  //----------------------------------------------------------------------------------------------
  /// For a given event/point in this box, return the run index
  uint16_t MDHistoWorkspaceIterator::getInnerRunIndex(size_t /*index*/) const
  {
    return 0;
    //throw std::runtime_error("MDHistoWorkspaceIterator: No events are contained, so it is not possible to return inner run index.");
  }

  /// For a given event/point in this box, return the detector ID
  int32_t MDHistoWorkspaceIterator::getInnerDetectorID(size_t /*index*/) const
  {
    return 0;
    //throw std::runtime_error("MDHistoWorkspaceIterator: No events are contained, so it is not possible to return inner detector ID.");
  }

  /// Returns the position of a given event for a given dimension
  coord_t MDHistoWorkspaceIterator::getInnerPosition(size_t /*index*/, size_t dimension) const
  {
    return m_ws->getCenter(m_pos)[dimension];
  }

  /// Returns the signal of a given event
  signal_t MDHistoWorkspaceIterator::getInnerSignal(size_t /*index*/) const
  {
    return m_ws->getSignalAt(m_pos);
  }

  /// Returns the error of a given event
  signal_t MDHistoWorkspaceIterator::getInnerError(size_t /*index*/) const
  {
    return m_ws->getErrorAt(m_pos);
  }

} // namespace Mantid
} // namespace MDEvents

