#ifndef MDDIMENSIONEXTENTS_H_
#define MDDIMENSIONEXTENTS_H_

/*
 * MDDimensionExtents.h
 *
 *  Created on: Jan 14, 2011
 *      Author: Janik Zikovsky
 */
#include "MantidKernel/System.h"
#include <limits>

namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  /** Simple class that holds the extents (min/max)
   * of a given dimension in a MD workspace or MDBox
   */
  class DLLExport MDDimensionExtents
  {
  public:

    /** Empty constructor - reset everything.
     *  */
    MDDimensionExtents() :
      min( std::numeric_limits<coord_t>::max() ),
      max( -std::numeric_limits<coord_t>::max() )
    { }

    // ---- Public members ----------
    /// Extent: minimum value in that dimension
    coord_t min;
    /// Extent: maximum value in that dimension
    coord_t max;
  };


}//namespace MDEvents

}//namespace Mantid



#endif /* MDDIMENSIONEXTENTS_H_ */
