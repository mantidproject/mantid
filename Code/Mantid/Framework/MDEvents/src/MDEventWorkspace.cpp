#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEvent.h"

namespace Mantid
{
namespace MDEvents
{

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this workspace */
  TMDE(
  int MDEventWorkspace)::getNumDims() const
  {
    return nd;
  }

  /** Returns the total number of points (events) in this workspace */
  TMDE(
  size_t MDEventWorkspace)::getNPoints() const
  {
    //return data.size();
    return 0;
  }

  /** Returns the number of bytes of memory
   * used by the workspace. */
  TMDE(
  size_t MDEventWorkspace)::getMemoryUsed() const
  {
    return this->getNPoints() * sizeof(MDE);
  }




}//namespace MDEvents

}//namespace Mantid

