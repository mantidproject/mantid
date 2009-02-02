//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Object.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/GeometryHandler.h"
#include "MantidGeometry/CacheGeometryHandler.h"
#include <cfloat>

namespace Mantid
{
namespace Geometry
{

IObjComponent::IObjComponent()
  {
	handle=new CacheGeometryHandler(this);
  }
  // Looking to get rid of the first of these constructors in due course (and probably add others)
IObjComponent::~IObjComponent()
  {
	if(handle!=NULL)
		delete handle;
  }

} // namespace Geometry
} // namespace Mantid
