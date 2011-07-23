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
  /** Helper function. Return true if the box is touching the implicit function
   *
   * @param function :: MDImplicitFunction
   * @param box :: IMDBox
   * @return true if the box is touching the implicit function
   */
  TMDE(
  bool MDBoxIterator)::boxIsTouching(Mantid::Geometry::MDImplicitFunction * function, IMDBox<MDE,nd> * box)
  {
    // NULL box does not touch anything.
    if (!box) return false;
    // Get the vertexes of the box as a bare array
    size_t numVertices = 0;
    coord_t * vertexes = box->getVertexesArray(numVertices);
    bool retVal = function->isBoxTouching(vertexes, numVertices);
    delete [] vertexes;
    return retVal;
  }

  //----------------------------------------------------------------------------------------------
  /** Helper function. Return true if the box is touching the implicit function
   *
   * @param function :: MDImplicitFunction
   * @param box :: IMDBox
   * @return true if the box is touching the implicit function
   */
  TMDE(
  MDImplicitFunction::eContact MDBoxIterator)::boxContact(Mantid::Geometry::MDImplicitFunction * function, IMDBox<MDE,nd> * box)
  {
    // NULL box does not touch anything.
    if (!box) return MDImplicitFunction::NOT_TOUCHING;
    // Get the vertexes of the box as a bare array
    size_t numVertices = 0;
    coord_t * vertexes = box->getVertexesArray(numVertices);
    MDImplicitFunction::eContact retVal = function->boxContact(vertexes, numVertices);
    delete [] vertexes;
    return retVal;
  }



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
      : m_topBox(topBox),
        m_maxDepth(maxDepth), m_leafOnly(leafOnly),
        m_done(false),
        m_function(function)
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

    // Check if top-level box is excluded by the implicit function
    if (m_function)
    {
      if (!boxIsTouching(m_function, m_current))
      {
        m_done = true;
        return;
      }
    }

    // Starting with leafs only? You need to skip ahead (if the top level box has children)
    if (m_leafOnly && m_current->getNumChildren() > 0 && m_currentDepth < m_maxDepth)
      next();

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


  //----------------------------------------------------------------------------------------------
  /**  Advance to the next box in the workspace. If the current box is the last one in the workspace
   * do nothing and return false.
   * @return true if there are more cells to iterate through.
   */
  TMDE(
  bool MDBoxIterator)::next()
  {
    while (!m_done)
    {
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

        // Check if the child is excluded by the implicit function
        bool skipBecauseItIsExcluded = false;
        if (m_function)
          skipBecauseItIsExcluded = !boxIsTouching(m_function, m_current);

        // For leaves-only, you need to keep looking this way until you reach a leaf (no children)
        // For boxes excluded by an implicit function, you need to keep looking too.
        if ((skipBecauseItIsExcluded) ||
            (m_leafOnly && m_current->getNumChildren() > 0 && m_currentDepth < m_maxDepth) )
          return next();
        else
          // If not leaves-only, then you return each successive child you encounter.
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

      // Check if the child is excluded by the implicit function
      bool skipBecauseItIsExcluded = false;
      if (m_function)
        skipBecauseItIsExcluded = !boxIsTouching(m_function, m_current);

      // For leaves-only, you need to keep looking this way until you reach a leaf (no children)
      // For boxes excluded by an implicit function, you need to keep looking too.
      if ((skipBecauseItIsExcluded) ||
          (m_leafOnly && m_current->getNumChildren() > 0 && m_currentDepth < m_maxDepth) )
      {
        // Keep going
        // return next();
      }
      else
        // If not leaves-only, then you return each successive child you encounter.
        return true;

    } // (while not done)

    // Done!
    return false;
  }


} // namespace Mantid
} // namespace MDEvents

