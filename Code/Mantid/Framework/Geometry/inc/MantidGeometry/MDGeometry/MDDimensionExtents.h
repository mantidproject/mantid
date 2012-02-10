#ifndef MDDIMENSIONEXTENTS_H_
#define MDDIMENSIONEXTENTS_H_

/*
 * MDDimensionExtents.h
 *
 *  Created on: Jan 14, 2011
 *      Author: Janik Zikovsky
 */
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include <limits>

namespace Mantid
{
namespace Geometry
{

#pragma pack(push, 4) //Ensure the structure is no larger than it needs to

// the statement to exclude using macro min(a,b) in visual C++ uder win
#ifdef min
#undef min
#endif
// the statement to exclude using macro max(a,b) in visual C++ uder win
#ifdef max
#undef max
#endif

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
      min( coord_t(1e30) ),
      max( coord_t(-1e30) )
    { }

    // ---- Public members ----------
    /// Extent: minimum value in that dimension
    coord_t min;
    /// Extent: maximum value in that dimension
    coord_t max;
  };

#pragma pack(pop) //Return to default packing size

}//namespace API

}//namespace Mantid



#endif /* MDDIMENSIONEXTENTS_H_ */
