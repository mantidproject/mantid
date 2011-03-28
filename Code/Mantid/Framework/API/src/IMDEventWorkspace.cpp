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


  //-----------------------------------------------------------------------------------------------
  /** Get the index of the dimension that matches the name given
   *
   * @param name :: name of the dimensions
   * @return the index (size_t)
   * @throw runtime_error if it cannot be found.
   */
  size_t IMDEventWorkspace::getDimensionIndexByName(const std::string & name)
  {
    for (size_t d=0; d<dimensions.size(); d++)
      if (dimensions[d].getName() == name)
        return d;
    throw std::runtime_error("Dimension named '" + name + "' was not found in the IMDEventWorkspace.");
  }



}//namespace MDEvents

}//namespace Mantid

