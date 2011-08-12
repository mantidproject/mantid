#ifndef MANTID_MDEVENTS_MDBOXITERATOR_H_
#define MANTID_MDEVENTS_MDBOXITERATOR_H_
    
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDLeanEvent.h"

namespace Mantid
{
namespace MDEvents
{

  /** MDBoxIterator: iterate through IMDBox
   * hierarchy down to a given maximum depth.
   * 
   * @author Janik Zikovsky
   * @date 2011-06-03
   */
  TMDE_CLASS
  class DLLExport MDBoxIterator
  {
  public:
    MDBoxIterator(IMDBox<MDE,nd> * topBox, size_t maxDepth, bool leafOnly,
        Mantid::Geometry::MDImplicitFunction * function = NULL);
    ~MDBoxIterator();

    /// Return a pointer to the current box pointed to by the iterator.
    IMDBox<MDE,nd> * getBox() const
    {
      return m_current;
    }

//    /// Get the d-th coordinate of the current cell
//    virtual coord_t getCoordinate(std::size_t d) const;

    /// Advance to the next cell. If the current cell is the last one in the workspace
    /// do nothing and return false.
    virtual bool next();

    static bool boxIsTouching(Mantid::Geometry::MDImplicitFunction * function, IMDBox<MDE,nd> * box);

    static Mantid::Geometry::MDImplicitFunction::eContact boxContact(Mantid::Geometry::MDImplicitFunction * function, IMDBox<MDE,nd> * box);

  private:
    /// Top-level box
    IMDBox<MDE,nd> * m_topBox;

    /// Current parent box
    IMDBox<MDE,nd> * m_parent;

    /// Cached number of children of the current parent.
    size_t m_parentNumChildren;

    /// Current box pointed to
    IMDBox<MDE,nd> * m_current;

    /// How deep we currently are looking at
    size_t m_currentDepth;

    /// Array of the index into each recursion level
    size_t * m_indices;

    /// Array of pointers to the parents
    IMDBox<MDE,nd> ** m_parents;

    /// How deep to search
    size_t m_maxDepth;

    /// Return only leaf nodes.
    bool m_leafOnly;

    /// Set to true when reached the end
    bool m_done;

    /// Implicit function for limiting where you iterate. NULL means no limits.
    Mantid::Geometry::MDImplicitFunction * m_function;
  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDBOXITERATOR_H_ */
