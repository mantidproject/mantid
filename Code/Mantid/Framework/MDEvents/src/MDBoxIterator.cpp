#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidKernel/System.h"

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
   */
  TMDE(MDBoxIterator)::MDBoxIterator(IMDBox<MDE,nd> * topBox, size_t maxDepth, bool leafOnly)
      : m_topBox(topBox),
        m_maxDepth(maxDepth), m_leafOnly(leafOnly),
        m_done(false)
  {
    if (!m_topBox)
      throw std::invalid_argument("MDBoxIterator::ctor(): NULL top-level box given.");

    // Allocate an array of indices
    m_indices = new size_t[m_maxDepth+1];
    // Allocate an array of parents pointers
    m_parents = new IMDBox<MDE,nd> *[m_maxDepth+1];

    // Indices start at -1 (meaning do the parent)
    for (size_t i=0; i<m_maxDepth+1; i++)
    {
      m_indices[i] = 0;
      m_parents[i] = NULL;
    }

    // We have no parent
    m_parent = NULL;
    m_parentNumChildren = 0;

    m_current = m_topBox;
    m_currentDepth = m_current->getDepth();

    if (m_currentDepth > m_maxDepth)
      throw std::invalid_argument("MDBoxIterator::ctor(): The maxDepth parameter must be >= the depth of the topBox.");

    // Starting with leafs only? You need to skip ahead
    if (m_leafOnly && m_current->getNumChildren() > 0 && m_currentDepth < m_maxDepth)
      next();


//
////    // We start with the top box as the parent
////    m_parent = m_topBox;
////    m_parents[m_parent->getDepth()] = m_parent;
////
////    // This is how deep we currently are
////    m_currentDepth = m_parent->getDepth()+1;
////    m_parentNumChildren = m_parent->getNumChildren();
//
//    if (m_parentNumChildren > 0)
//    {
//      // The first box is the first child
//      m_current = m_parent->getChild(0);
//      if (m_leafOnly && (m_currentDepth < m_maxDepth) && (m_current->getNumChildren() > 0))
//      {
//        // We are looking only for leaves and this isn't one. Use the next() method to go to the next one
//        next();
//      }
//    }
//    else
//    {
//      // Special case with no children at all.
//      m_current = m_topBox;
//      m_parent = NULL;
//      m_currentDepth = m_current->getDepth();
//    }
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TMDE(MDBoxIterator)::~MDBoxIterator()
  {
    delete [] m_indices;
    delete [] m_parents;
  }
  
//  /** Get the i-th coordinate of the current cell
//   *
//   * @param i :: dimension # to retriev
//   * @return position of the center of the cell in that dimension
//   */
//  TMDE(
//  double MDBoxIterator)::getCoordinate(std::size_t /*i*/) const
//  {
//    return 0.0;
//  }

  /**  Advance to the next box in the workspace. If the current box is the last one in the workspace
   * do nothing and return false.
   * @return true if there are more cells to iterate through.
   */
  TMDE(
  bool MDBoxIterator)::next()
  {
    if (m_done) return false;

    size_t children = m_current->getNumChildren();

    if ((children > 0) && (m_currentDepth < m_maxDepth))
    {
      // Current becomes the parent
      m_parent = m_current;
      m_parentNumChildren = m_parent->getNumChildren();

      // Go deeper
      m_indices[++m_currentDepth] = 0;

      // Save the parent for this depth
      m_parents[m_currentDepth] = m_parent;
      // Take the first child of it
      m_current = m_parent->getChild(0);

      // For leaves-only, you need to keep looking this way until you reach a leaf (no children)
      if (m_leafOnly && m_current->getNumChildren() > 0 && m_currentDepth < m_maxDepth)
        return next();
      else
        return true;
    }

    // If you reach here, then it is a leaf node that we've already returned.

    // Go to the next child
    size_t newIndex = ++m_indices[m_currentDepth];

    while (newIndex >= m_parentNumChildren)
    {
      // Reached the end of the number of children of the current parent.

      // Are we as high as we can go?
      if (m_currentDepth == 0)
      {
        m_done = true;
        return false;
      }

      // Time to go up a level
      m_currentDepth--;
      m_parent = m_parents[m_currentDepth];

      // Reached a null parent = higher than we started. We're done
      if (!m_parent)
      {
        m_done = true;
        return false;
      }

      // If we get here, then we have a valid parent
      m_parentNumChildren = m_parent->getNumChildren();

      // Move on to the next child of this level because we already returned this guy.
      newIndex = ++m_indices[m_currentDepth];
    }

    // This is now the current box
    m_current = m_parent->getChild(m_indices[m_currentDepth]);

    // For leaves-only, you need to keep looking this way until you reach a leaf (no children)
    if (m_leafOnly && m_current->getNumChildren() > 0 && m_currentDepth < m_maxDepth)
      return next();
    else
      return true;

  }


} // namespace Mantid
} // namespace MDEvents

