#include "MantidMDEvents/CoordTransformAligned.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CoordTransformAligned::CoordTransformAligned(const size_t inD, const size_t outD, const size_t * /*dimensionToBinFrom*/,
      const coord_t * /*min*/, const coord_t * /*step*/)
  : CoordTransform(inD, outD)
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CoordTransformAligned::~CoordTransformAligned()
  {
    // TODO Auto-generated destructor stub
  }
  


} // namespace Mantid
} // namespace MDEvents

