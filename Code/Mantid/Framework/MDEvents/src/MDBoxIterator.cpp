#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBoxIterator.h"

using namespace Mantid;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDEvents
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param topBox :: top-level parent box.
   * @param maxDepth :: maximum depth to go to
   * @param leafOnly :: only report "leaf" nodes, e.g. boxes that are no longer split OR are at maxDepth.
   * @param function :: ImplicitFunction that limits iteration volume. NULL for don't limit this way.
   *        Note that the top level box is ALWAYS returned at least once, even if it is outside the
   *        implicit function
   */
  TMDE(MDBoxIterator)::MDBoxIterator(IMDBox<MDE,nd> * topBox, size_t maxDepth, bool leafOnly,
      Mantid::Geometry::MDImplicitFunction * function)
      : m_pos(0), m_current(NULL), m_currentMDBox(NULL), m_events(NULL)
  {
    if (!topBox)
      throw std::invalid_argument("MDBoxIterator::ctor(): NULL top-level box given.");

    if (topBox->getDepth() > maxDepth)
      throw std::invalid_argument("MDBoxIterator::ctor(): The maxDepth parameter must be >= the depth of the topBox.");

    // Use the "getBoxes" to get all the boxes in a vector.
    m_boxes.clear();
    if (function)
      topBox->getBoxes(m_boxes, maxDepth, leafOnly, function);
    else
      topBox->getBoxes(m_boxes, maxDepth, leafOnly);

    // We avoid copying by NOT calling the init() method
    m_max = m_boxes.size();
    // Get the first box
    if (m_max > 0)
      m_current = m_boxes[0];  }


  //----------------------------------------------------------------------------------------------
  /** Constructor for parallelized iterators
   *
   * @param boxes :: ref to the list of ALL boxes in the workspace
   * @param begin :: start iterating at this point in the list
   * @param end :: stop iterating at this point in the list
   */
  TMDE(MDBoxIterator)::MDBoxIterator(std::vector<IMDBox<MDE,nd>*> & boxes, size_t begin, size_t end)
    : m_pos(0), m_current(NULL), m_currentMDBox(NULL), m_events(NULL)

  {
    this->init(boxes, begin, end);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor helper function
   * @param boxes :: ref to the list of ALL boxes in the workspace
   * @param begin :: start iterating at this point in the list
   * @param end :: stop iterating at this point in the list
   */
  TMDE(
  void MDBoxIterator)::init(std::vector<IMDBox<MDE,nd>*> & boxes, size_t begin, size_t end)
  {
    m_boxes.clear();
    if (begin >= boxes.size())
      throw std::runtime_error("MDBoxIterator::ctor(): invalid beginning position.");
    size_t theEnd = end;
    if (theEnd < begin)
      throw std::runtime_error("MDBoxIterator::ctor(): end position is before the position.");
    if (theEnd > boxes.size())
      theEnd = boxes.size();

    // Copy the pointers to boxes in the range.
    m_boxes.assign(boxes.begin() + begin, boxes.begin() + end);

    m_max = m_boxes.size();
    // Get the first box
    if (m_max > 0)
      m_current = m_boxes[0];
  }

    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TMDE(MDBoxIterator)::~MDBoxIterator()
  {
  }


  //----------------------------------------------------------------------------------------------
  /** Jump to the index^th cell.
   *
   * @param index :: point to jump to. Must be 0 <= index < getDataSize().
   */
  TMDE(
  void MDBoxIterator)::jumpTo(size_t index)
  {
    releaseEvents();
    m_pos = index;
    if (m_pos < m_max)
    {
      m_current = m_boxes[m_pos];
    }
  }

  //----------------------------------------------------------------------------------------------
  /// @return true if the iterator is currently valid
  TMDE(
  bool MDBoxIterator)::valid() const
  {
    return (m_current != NULL);
  }

  //----------------------------------------------------------------------------------------------
  /// Advance to the next cell. If the current cell is the last one in the workspace
  /// do nothing and return false.
  /// @return true if you can continue iterating
  TMDE(
  bool MDBoxIterator)::next()
  {
    return this->next(1);
  }

  //----------------------------------------------------------------------------------------------
  /// Advance, skipping a certain number of cells.
  /// @param skip :: how many to increase. If 1, then every point will be sampled.
  TMDE(
  inline bool MDBoxIterator)::next(size_t skip)
  {
    releaseEvents();
    m_pos += skip;
    if (m_pos < m_max)
    {
      // Move up.
      m_current = m_boxes[m_pos];
      return true;
    }
    else
      // Done - can't iterate
      return false;
  }

  //----------------------------------------------------------------------------------------------
  /** If needed, retrieve the events vector from the box.
   * Does nothing if the events are already obtained.
   * @throw if the box cannot have events.
   */
  TMDE(
  void MDBoxIterator)::getEvents() const
  {
    if (!m_events)
    {
      if (!m_currentMDBox)
        m_currentMDBox = dynamic_cast<MDBox<MDE,nd> *>(m_current);
      if (m_currentMDBox)
      {
        // Retrieve the event vector.
        m_events = &m_currentMDBox->getConstEvents();
      }
      else
        throw std::runtime_error("MDBoxIterator: requested the event list from a box that is not a MDBox!");
    }
  }



  //----------------------------------------------------------------------------------------------
  /** After you're done with a given box, release the events list
   * (if it was retrieved)
   */
  TMDE(
  void MDBoxIterator)::releaseEvents() const
  {
    if (m_events)
    {
      m_currentMDBox->releaseEvents();
      m_events = NULL;
      m_currentMDBox = NULL;
    }
  }


  //----------------------------------------------------------------------------------------------
  /// Returns the number of entries to be iterated against.
  TMDE(size_t MDBoxIterator)::getDataSize() const
  {
    return m_max;
  }


  //----------------------------------------------------------------------------------------------
  /// Returns the normalized signal for this box
  TMDE(signal_t MDBoxIterator)::getNormalizedSignal() const
  {
    return m_current->getSignalNormalized();
  }

  /// Returns the normalized error for this box
  TMDE(signal_t MDBoxIterator)::getNormalizedError() const
  {
    return m_current->getError() * m_current->getInverseVolume();
  }

  /// Returns the signal for this box
  TMDE(signal_t MDBoxIterator)::getSignal() const
  {
    return m_current->getSignal();
  }

  /// Returns the error for this box
  TMDE(signal_t MDBoxIterator)::getError() const
  {
    return m_current->getError();
  }

  /// Return a list of vertexes defining the volume pointed to
  TMDE(coord_t * MDBoxIterator)::getVertexesArray(size_t & numVertices) const
  {
    return m_current->getVertexesArray(numVertices);
  }

  TMDE(coord_t * MDBoxIterator)::getVertexesArray(size_t & numVertices, const size_t outDimensions, const bool * maskDim) const
  {
    return m_current->getVertexesArray(numVertices, outDimensions, maskDim);
  }

  /// Returns the position of the center of the box pointed to.
  TMDE(Mantid::Kernel::VMD MDBoxIterator)::getCenter() const
  {
    coord_t center[nd];
    m_current->getCenter(center);
    return Mantid::Kernel::VMD(nd, center);
  }


  //----------------------------------------------------------------------------------------------
  /// Returns the number of events/points contained in this box
  TMDE(size_t MDBoxIterator)::getNumEvents() const
  {
    // Do we have a MDBox?
    m_currentMDBox = dynamic_cast<MDBox<MDE,nd> *>(m_current);
    if (m_currentMDBox)
      return m_current->getNPoints();
    else
      return 0;
  }


  /// For a given event/point in this box, return the run index
  TMDE(uint16_t MDBoxIterator)::getInnerRunIndex(size_t index) const
  {
    getEvents();
    return (*m_events)[index].getRunIndex();
  }


  /// For a given event/point in this box, return the detector ID
  TMDE(int32_t MDBoxIterator)::getInnerDetectorID(size_t index) const
  {
    getEvents();
    return (*m_events)[index].getDetectorID();
  }


  /// Returns the position of a given event for a given dimension
  TMDE(coord_t MDBoxIterator)::getInnerPosition(size_t index, size_t dimension) const
  {
    getEvents();
    return (*m_events)[index].getCenter(dimension);
  }


  /// Returns the signal of a given event
  TMDE(signal_t MDBoxIterator)::getInnerSignal(size_t index) const
  {
    getEvents();
    return (*m_events)[index].getSignal();
  }

  /// Returns the error of a given event
  TMDE(signal_t MDBoxIterator)::getInnerError(size_t index) const
  {
    getEvents();
    return (*m_events)[index].getError();
  }


} // namespace Mantid
} // namespace MDEvents

