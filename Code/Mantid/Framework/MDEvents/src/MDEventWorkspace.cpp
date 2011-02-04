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
    return this->getNPoints() * sizeof(MDEvent<nd>);
  }



  //-----------------------------------------------------------------------------------------------


  // We export a bunch of version of MDEventWorkspace with various dimension sizes.
  template DLLExport class MDEventWorkspace<1>;
  template DLLExport class MDEventWorkspace<2>;
  template DLLExport class MDEventWorkspace<3>;
  template DLLExport class MDEventWorkspace<4>;




}//namespace MDEvents

}//namespace Mantid

