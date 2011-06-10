#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidKernel/Exception.h"
#include "ICompAssemblyActor.h"
#include <cfloat>
using namespace Mantid;
using namespace Geometry;

/**
* This is a constructor for CompAssembly Actor
* @param objs :: list of objects that are used by IObjCompenent actors and will be filled with the new objects
* @param id :: ComponentID of this object of CompAssembly
* @param ins :: Instrument
* @param withDisplayList :: true to create a display list for the compassembly and its subcomponents
*/
ICompAssemblyActor::ICompAssemblyActor(const InstrumentActor& instrActor,const Mantid::Geometry::ComponentID& compID)
  :ComponentActor(instrActor,compID),
  mNumberOfDetectors(0),
  minBoundBox(DBL_MAX,DBL_MAX,DBL_MAX),
  maxBoundBox(-DBL_MAX,-DBL_MAX,-DBL_MAX)
{
}

//------------------------------------------------------------------------------------------------
/**
 * Return the bounding box
 * @param minBound :: min point of the bounding box
 * @param maxBound :: max point of the bounding box
 */
void ICompAssemblyActor::getBoundingBox(Mantid::Geometry::V3D& minBound,Mantid::Geometry::V3D& maxBound)const
{
  minBound=minBoundBox;
  maxBound=maxBoundBox;
}

