#ifndef MANTID_MDEVENTS_MDBOXITERATOR_H_
#define MANTID_MDEVENTS_MDBOXITERATOR_H_
    
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidAPI/IMDIterator.h"

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
  class DLLExport MDBoxIterator : public Mantid::API::IMDIterator
  {
  public:
    MDBoxIterator();
    ~MDBoxIterator();
    
    /// Get the size of the data (number of entries that will be iterated through)
    virtual size_t getDataSize() const
    { throw std::runtime_error("Not yet implemented.");
    }

    /// Return the current data pointer (index) into the MDWorkspace
    virtual size_t getPointer() const
    { throw std::runtime_error("Not yet implemented.");
    }

    /// Get the d-th coordinate of the current cell
    virtual coord_t getCoordinate(std::size_t d) const;

    /// Advance to the next cell. If the current cell is the last one in the workspace
    /// do nothing and return false.
    virtual bool next();

  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDBOXITERATOR_H_ */
