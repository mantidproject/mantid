#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/CoordTransform.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace MDEvents
{

  //----------------------------------------------------------------------------------------------
  /** Constructor. Validates the inputs
   *
   * @param inD :: input number of dimensions, >= 1
   * @param outD :: output number of dimensions, <= inD
   * @throw std::runtime_error if outD > inD
   */
  CoordTransform::CoordTransform(const size_t inD, const size_t outD)
  : inD(inD), outD(outD)
  {
    if (outD > inD)
      throw std::runtime_error("CoordTransform: Cannot have more output dimensions than input dimensions!");
    if (outD == 0)
      throw std::runtime_error("CoordTransform: invalid number of output dimensions!");
    if (inD == 0)
      throw std::runtime_error("CoordTransform: invalid number of input dimensions!");
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CoordTransform::~CoordTransform()
  {
  }




} // namespace Mantid
} // namespace MDEvents

