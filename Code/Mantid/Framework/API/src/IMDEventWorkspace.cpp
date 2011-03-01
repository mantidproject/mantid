#include "MantidAPI/Dimension.h"
#include "MantidAPI/IMDEventWorkspace.h"

using Mantid::MDEvents::CoordType;

namespace Mantid
{
namespace API
{

  //-----------------------------------------------------------------------------------------------
  /** Add a new dimension
   *
   * @param dimInfo :: Dimension object which will be copied into the workspace
   */
  void IMDEventWorkspace::addDimension(Dimension dimInfo)
  {
    dimensions.push_back(dimInfo);
  }


  //-----------------------------------------------------------------------------------------------
  /** Get the given dimension
   *
   * @param dim :: index of dimension to set
   * @return Dimension object
   */
  Dimension IMDEventWorkspace::getDimension(size_t dim)
  {
    return dimensions[dim];
  }



}//namespace MDEvents

}//namespace Mantid

