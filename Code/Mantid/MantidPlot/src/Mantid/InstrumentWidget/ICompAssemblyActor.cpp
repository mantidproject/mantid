#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
//#include "MantidGeometry/IDetector.h"
//#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Exception.h"
#include "MantidObject.h"
#include "ICompAssemblyActor.h"
//#include "ObjComponentActor.h"
//#include "RectangularDetectorActor.h"
#include <cfloat>
using namespace Mantid;
using namespace Geometry;

/**
* This is default constructor for CompAssembly Actor
* @param withDisplayList :: true to create a display list for the compassembly and its subcomponents
*/
ICompAssemblyActor::ICompAssemblyActor(bool withDisplayList):GLActor(withDisplayList), mNumberOfDetectors(0), minBoundBox(DBL_MAX,DBL_MAX,DBL_MAX),maxBoundBox(-DBL_MAX,-DBL_MAX,-DBL_MAX)
{
}

/**
* This is a constructor for CompAssembly Actor
* @param objs            :: list of objects that are used by IObjCompenent actors and will be filled with the new objects
* @param id              :: ComponentID of this object of CompAssembly
* @param ins             :: Instrument
* @param withDisplayList :: true to create a display list for the compassembly and its subcomponents
*/
ICompAssemblyActor::ICompAssemblyActor(boost::shared_ptr<std::map<const boost::shared_ptr<const Object>,MantidObject*> >& objs, Mantid::Geometry::ComponentID id,boost::shared_ptr<Mantid::Geometry::IInstrument> ins,bool withDisplayList):GLActor(withDisplayList),mNumberOfDetectors(0),minBoundBox(DBL_MAX,DBL_MAX,DBL_MAX),maxBoundBox(-DBL_MAX,-DBL_MAX,-DBL_MAX)
{
  // Initialises
  mId=id;
  mInstrument=ins;
  mObjects=objs;
  this->setName( ins->getName() );
  //Create the subcomponent actors
  //this->initChilds(withDisplayList);
}

//------------------------------------------------------------------------------------------------
/**
 * Return the bounding box
 * @param minBound :: min point of the bounding box
 * @param maxBound :: max point of the bounding box
 */
void ICompAssemblyActor::getBoundingBox(Mantid::Geometry::V3D& minBound,Mantid::Geometry::V3D& maxBound)
{
  minBound=minBoundBox;
  maxBound=maxBoundBox;
}

