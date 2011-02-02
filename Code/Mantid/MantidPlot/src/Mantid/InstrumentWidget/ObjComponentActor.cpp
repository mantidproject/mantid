#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/IDetector.h"
#include "ObjComponentActor.h"
#include "MantidObject.h"

using namespace Mantid;
using namespace Geometry;

ObjComponentActor::ObjComponentActor(bool withDisplayList)
  :GLActor(withDisplayList)
{
}


ObjComponentActor::ObjComponentActor(MantidObject *obj, boost::shared_ptr<Mantid::Geometry::IObjComponent> objComp, bool withDisplayList)
  : GLActor(withDisplayList)
{
	mObjComp=objComp;
	mObject=obj;
	this->setName( objComp->getName() );
}

ObjComponentActor::~ObjComponentActor()
{
}

//-------------------------------------------------------------------------------------------------
/**
 * Concrete implementation of rendering ObjComponent.
 */
void ObjComponentActor::define()
{
	glPushMatrix();
	// Translation first
	V3D pos = mObjComp->getPos();
	if (!(pos.nullVector()))
	{
		glTranslated(pos[0],pos[1],pos[2]);
	}
	//Rotation
	Quat rot = mObjComp->getRotation();
	if (!(rot.isNull()))
	{
		double deg,ax0,ax1,ax2;
		rot.getAngleAxis(deg,ax0,ax1,ax2);
		glRotated(deg,ax0,ax1,ax2);
	}
	//Scale
	V3D scaleFactor = mObjComp->getScaleFactor();
	if (!(scaleFactor==V3D(1,1,1)))
	{
		glScaled(scaleFactor[0],scaleFactor[1],scaleFactor[2]);
	}

	//std::cout << "ObjComponentActor::define() called with pos=" << pos << " and rot=" << rot << " and scale=" << scaleFactor << ".\n";

	//If a mantidObject was specified, call ITS draw routine
	if (mObject)
	  mObject->draw();
	else
	  //Otherwise, use the ObjComponent draw routine. This is what RectangularDetector will use.
	  mObjComp->draw();

	glPopMatrix();
}


//-------------------------------------------------------------------------------------------------
/** Append the detector ID of this object to the list, if it is a detector.
 *
 * @param idList :: sequential list of detector IDs.
 */
void ObjComponentActor::appendObjCompID(std::vector<int>& idList)
{
  //check the component type if its detector or not
  const boost::shared_ptr<Mantid::Geometry::IDetector>  detector = boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>(this->getObjComponent());
  if( detector != boost::shared_ptr<Mantid::Geometry::IDetector>() )
  {
    if( !detector->isMonitor() )
    {
      idList.push_back(detector->getID());
    }
    else
    {
      idList.push_back(-1);
    }
  }
}


//------------------------------------------------------------------------------------------------
/**
 * The colors are set using the iterator of the color list. The order of the detectors
 * in this color list was defined by the calls to appendObjCompID().
 *
 * @param list :: Color list iterator
 * @return the number of detectors
 */
int ObjComponentActor::setInternalDetectorColors(std::vector<boost::shared_ptr<GLColor> >::iterator & list)
{
  const boost::shared_ptr<Mantid::Geometry::IDetector>  detector =
      boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>(this->getObjComponent());
  if( detector != boost::shared_ptr<Mantid::Geometry::IDetector>() )
  {
    this->setColor((*list));
    //Increment for the next one
    list++;
    return 1;
  }
  else
    return 0;
}



//-------------------------------------------------------------------------------------------------
/**
 * Return the bounding box
 * @param minBound :: min point of the bounding box
 * @param maxBound :: max point of the bounding box
 */
void ObjComponentActor::getBoundingBox(Mantid::Geometry::V3D& minBound,Mantid::Geometry::V3D& maxBound)
{
	double xmin,ymin,zmin,xmax,ymax,zmax;
	xmin=ymin=zmin=-1000;
	xmax=ymax=zmax=1000;
	mObjComp->getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
	minBound[0]=xmin;minBound[1]=ymin;minBound[2]=zmin;
	maxBound[0]=xmax;maxBound[1]=ymax;maxBound[2]=zmax;
}

void ObjComponentActor::detectorCallback(DetectorCallback* callback)const
{
  boost::shared_ptr<const Mantid::Geometry::IDetector> det =
      boost::dynamic_pointer_cast<const Mantid::Geometry::IDetector> (mObjComp);
  if (det)
  {
    callback->callback(det,DetectorCallbackData(*getColor()));
  }
}
