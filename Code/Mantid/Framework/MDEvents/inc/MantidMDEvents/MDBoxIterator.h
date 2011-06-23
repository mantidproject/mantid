#ifndef MANTID_MDEVENTS_MDBOXITERATOR_H_
#define MANTID_MDEVENTS_MDBOXITERATOR_H_
    
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidMDEvents/IMDBox.h"

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
    MDBoxIterator(IMDBox<MDE,nd> * topBox, size_t maxDepth, bool leafOnly);
    ~MDBoxIterator();

    /// Return a pointer to the current box pointed to by the iterator.
    virtual IMDBox<MDE,nd> * getBox() const;

    /// Get the d-th coordinate of the current cell
    virtual coord_t getCoordinate(std::size_t d) const;

    /// Advance to the next cell. If the current cell is the last one in the workspace
    /// do nothing and return false.
    virtual bool next();

  private:
    /// Top-level box
    IMDBox<MDE,nd> * m_topBox;

    /// Array of the index into each recursion level
    size_t * m_indices;

    /// How deep to search
    size_t m_maxDepth;
  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDBOXITERATOR_H_ */
