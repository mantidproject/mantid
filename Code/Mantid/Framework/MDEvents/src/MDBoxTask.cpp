#include "MantidMDEvents/MDBoxTask.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param inBox :: IMDBox pointer to the starting point of the task.
   */
  TMDE(
  MDBoxTask)::MDBoxTask(IMDBox<MDE,nd> * inBox)
  : Task(),
    inBox(inBox)
  {
    if (!inBox)
      throw std::runtime_error("MDBoxTask:: NULL IMDBox<> passed to the constructor.");
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TMDE(
  MDBoxTask)::~MDBoxTask()
  {
  }
  


} // namespace Mantid
} // namespace MDEvents

