#include "MantidKernel/Exception.h"
#include "MantidKernel/System.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/CoordTransform.h"
#include "MantidMDEvents/CoordTransformDistance.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param inD :: # of input dimensions
   * @param center :: array of size[inD], with the coordinates at the center
   * @param dimensionsUsed :: bool array of size[inD] where True is set for those dimensions that are considered when
   *        calculating distance.
   * @return
   */
  CoordTransformDistance::CoordTransformDistance(const size_t inD, const coord_t * center, const bool * dimensionsUsed)
  : CoordTransform(inD, 1)
  {
    // Create and copy the arrays.
    m_center = new coord_t[inD];
    m_dimensionsUsed = new bool[inD];
    for (size_t d=0; d<inD; d++)
    {
      m_center[d] = center[d];
      m_dimensionsUsed[d] = dimensionsUsed[d];
    }
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CoordTransformDistance::~CoordTransformDistance()
  {
    delete m_center;
    delete m_dimensionsUsed;
  }
  

  //----------------------------------------------------------------------------------------------
  /** Apply the coordinate transformation.
   *
   * Calculate the SQUARE of the distance between the input coordinates to m_center
   * but only on dimensionsUsed[d] == true.
   *
   * @param inputVector :: fixed-size array of input coordinates, of size inD
   * @param outVector :: fixed-size array of output coordinates, of size 1
   */
  void CoordTransformDistance::apply(const coord_t * inputVector, coord_t * outVector)
  {
    coord_t distanceSquared = 0;
    for (size_t d=0; d<inD; d++)
    {
      if (m_dimensionsUsed[d])
      {
        coord_t dist = inputVector[d] - m_center[d];
        distanceSquared += (dist * dist);
      }
    }
    /// Return the only output dimension
    outVector[0] = distanceSquared;
  }

} // namespace Mantid
} // namespace MDEvents

