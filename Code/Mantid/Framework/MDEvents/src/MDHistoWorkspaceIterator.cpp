#include "MantidMDEvents/MDHistoWorkspaceIterator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include "MantidKernel/Utils.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Geometry::IMDDimension_const_sptr;

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param workspace :: MDHistoWorkspace_sptr being iterated
   * @param function :: The implicit function to use
   * @return
   */
  MDHistoWorkspaceIterator::MDHistoWorkspaceIterator(MDHistoWorkspace_const_sptr workspace, Mantid::Geometry::MDImplicitFunction * function)
  {
    this->init(workspace.get(), function);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param workspace :: MDHistoWorkspace_sptr being iterated
   * @param function :: The implicit function to use
   * @return
   */
  MDHistoWorkspaceIterator::MDHistoWorkspaceIterator(const MDHistoWorkspace * workspace, Mantid::Geometry::MDImplicitFunction * function)
  {
    this->init(workspace, function);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor helper
   *
   * @param workspace :: MDWorkspace
   * @param function :: implicit function or NULL for none. Gains ownership of the pointer.
   */
  void MDHistoWorkspaceIterator::init(const MDHistoWorkspace * workspace,
      Mantid::Geometry::MDImplicitFunction * function)
  {
    m_ws = workspace;
    m_pos = 0;
    m_function = function;
    if (m_ws == NULL)
      throw std::invalid_argument("MDHistoWorkspaceIterator::ctor(): NULL workspace given.");
    m_max = m_ws->getNPoints();
    m_nd = m_ws->getNumDims();
    m_center = new coord_t[m_nd];
    m_origin = new coord_t[m_nd];
    m_binWidth = new coord_t[m_nd];
    m_index = new size_t[m_nd];
    m_indexMax = new size_t[m_nd];
    m_indexMaker = new size_t[m_nd];
    Utils::NestedForLoop::SetUp(m_nd, m_index, 0);
    // Initalize all these values
    for (size_t d=0; d<m_nd; d++)
    {
      IMDDimension_const_sptr dim = m_ws->getDimension(d);
      m_center[d] = 0;
      m_origin[d] = dim->getMinimum();
      m_binWidth[d] = dim->getBinWidth();
      m_indexMax[d] = dim->getNBins();
    }
    Utils::NestedForLoop::SetUpIndexMaker(m_nd, m_indexMaker, m_indexMax);

    // Make sure that the first iteration is at a point inside the implicit function
    if (m_function)
    {
      // Calculate the center of the 0-th bin
      for (size_t d=0; d<m_nd; d++)
        m_center[d] = m_origin[d] + (0 + 0.5) * m_binWidth[d];
      // Skip on if the first point is NOT contained
      if (!m_function->isPointContained(m_center))
        next();
    }
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDHistoWorkspaceIterator::~MDHistoWorkspaceIterator()
  {
    delete [] m_center;
    delete [] m_origin;
    delete [] m_binWidth;
    delete [] m_index;
    delete [] m_indexMax;
    if (m_function) delete m_function;
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
  /** @return true if the iterator is valid. Check this at the start of an iteration,
   * in case the very first point is not valid.
   */
  bool MDHistoWorkspaceIterator::valid() const
  {
    return (m_pos < m_max);
  }

  //----------------------------------------------------------------------------------------------
  /// Advance to the next cell. If the current cell is the last one in the workspace
  /// do nothing and return false.
  /// @return true if you can continue iterating
  bool MDHistoWorkspaceIterator::next()
  {
    if (m_function)
    {
      do
      {
        m_pos++;
        Utils::NestedForLoop::Increment(m_nd, m_index, m_indexMax);
        // Calculate the center
        for (size_t d=0; d<m_nd; d++)
        {
          m_center[d] = m_origin[d] + (double(m_index[d]) + 0.5) * m_binWidth[d];
//          std::cout << m_center[d] << ",";
        }
//        std::cout<<std::endl;
        // Keep incrementing until you are in the implicit function
      } while (!m_function->isPointContained(m_center)
               && m_pos < m_max);
      // Is the iteration finished?
      return (m_pos < m_max);
    }
    else
      // Go through every point;
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

  /// Returns the signal for this box, same as innerSignal
  signal_t MDHistoWorkspaceIterator::getSignal() const
  {
    return m_ws->getSignalAt(m_pos);;
  }

  /// Returns the error for this box, same as innerError
  signal_t MDHistoWorkspaceIterator::getError() const
  {
    return m_ws->getErrorAt(m_pos);
  }
  //----------------------------------------------------------------------------------------------
  /// Return a list of vertexes defining the volume pointed to
  coord_t * MDHistoWorkspaceIterator::getVertexesArray(size_t & numVertices) const
  {
    // The MDHistoWorkspace takes care of this
    return m_ws->getVertexesArray(m_pos, numVertices);
  }

  coord_t * MDHistoWorkspaceIterator::getVertexesArray(size_t & numVertices, const size_t outDimensions, const bool * maskDim) const
  {
    //Do the same thing as is done in the IMDBox
    UNUSED_ARG(numVertices);
    UNUSED_ARG(outDimensions);
    UNUSED_ARG(maskDim);
    throw std::runtime_error("Not Implemented At present time");
  }

  //----------------------------------------------------------------------------------------------
  /// Returns the position of the center of the box pointed to.
  Mantid::Kernel::VMD MDHistoWorkspaceIterator::getCenter() const
  {
    // Get the indices
    Utils::NestedForLoop::GetIndicesFromLinearIndex(m_nd, m_pos, m_indexMaker, m_indexMax, m_index);
    // Find the center
    for (size_t d=0; d<m_nd; d++)
      m_center[d] = m_origin[d] + (double(m_index[d]) + 0.5) * m_binWidth[d];
    return VMD(m_nd, m_center);
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

