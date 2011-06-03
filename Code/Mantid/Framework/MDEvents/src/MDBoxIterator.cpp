#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  TMDE(MDBoxIterator)::MDBoxIterator()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TMDE(MDBoxIterator)::~MDBoxIterator()
  {
  }
  

  /** Get the i-th coordinate of the current cell
   *
   * @param i :: dimension # to retriev
   * @return position of the center of the cell in that dimension
   */
  TMDE(
  double MDBoxIterator)::getCoordinate(std::size_t /*i*/) const
  {
    return 0.0;
  }

  /**  Advance to the next cell. If the current cell is the last one in the workspace
   * do nothing and return false.
   * @return true if there are more cells to iterate through.
   */
  TMDE(
  bool MDBoxIterator):: next()
  {
    return false;
  }


} // namespace Mantid
} // namespace MDEvents

