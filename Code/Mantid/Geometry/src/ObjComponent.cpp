//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/ObjComponent.h"
#include "MantidGeometry/Object.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/GeometryHandler.h"
#include <cfloat>

namespace Mantid
{
namespace Geometry
{

/** Constructor
 *  @param name   The name of the component
 *  @param parent The Parent geometry object of this component
 */
ObjComponent::ObjComponent(const std::string& name, Component* parent) :
  IObjComponent(),Component(name,parent), shape()
{
}

/** Constructor
 *  @param name   The name of the component
 *  @param shape  A pointer to the object describing the shape of this component
 *  @param parent The Parent geometry object of this component
 */
ObjComponent::ObjComponent(const std::string& name, boost::shared_ptr<Object> shape, Component* parent) :
  IObjComponent(),Component(name,parent), shape(shape)
{
}

/// Copy constructor
ObjComponent::ObjComponent(const ObjComponent& rhs) :
  IObjComponent(),Component(rhs), shape(rhs.shape)//,handle(0)
{
}

/// Destructor
ObjComponent::~ObjComponent()
{
}

/// Does the point given lie within this object component?
bool ObjComponent::isValid(const V3D& point) const
{
  // If the form of this component is not defined, just treat as a point
  if (!shape) return (this->getPos() == point);
  // Otherwise pass through the shifted point to the Object::isValid method
  return shape->isValid( factorOutComponentPosition(point) );
}

/// Does the point given lie on the surface of this object component?
bool ObjComponent::isOnSide(const V3D& point) const
{
  // If the form of this component is not defined, just treat as a point
  if (!shape) return (this->getPos() == point);
  // Otherwise pass through the shifted point to the Object::isOnSide method
  return shape->isOnSide( factorOutComponentPosition(point) );
}

/** Checks whether the track given will pass through this Component.
 *  @param track The Track object to test (N.B. Will be modified if hits are found)
 *  @returns The number of track segments added (i.e. 1 if the track enters and exits the object once each)
 *  @throw NullPointerException if the underlying geometrical Object has not been set
 */
int ObjComponent::interceptSurface(Track& track) const
{
  // If the form of this component is not defined, throw NullPointerException
  if (!shape) throw Kernel::Exception::NullPointerException("ObjComponent::interceptSurface","shape");

  V3D trkStart = factorOutComponentPosition(track.getInit());
  V3D trkDirection = takeOutRotation(track.getUVec());

  Track probeTrack(trkStart, trkDirection);
  int intercepts = shape->interceptSurface(probeTrack);

  Track::LType::const_iterator it;
  for (it = probeTrack.begin(); it < probeTrack.end(); ++it)
  {
    V3D in = it->PtA;
    this->getRotation().rotate(in);
	//use the scale factor
	in *= m_ScaleFactor;
    in += this->getPos();
    V3D out = it->PtB;
    this->getRotation().rotate(out);
	//use the scale factor
	out *= m_ScaleFactor;
    out += this->getPos();
	track.addTUnit(shape->getName(),in,out,out.distance(track.getInit()));
//    track.addTUnit(shape->getName(),in,out,it->Dist);
  }

  return intercepts;
}

/** Finds the approximate solid angle covered by the component when viewed from the point given
 *  @param observer The position from which the component is being viewed
 *  @returns The solid angle in steradians
 *  @throw NullPointerException if the underlying geometrical Object has not been set
 */
double ObjComponent::solidAngle(const V3D& observer) const
{
  // If the form of this component is not defined, throw NullPointerException
  if (!shape) throw Kernel::Exception::NullPointerException("ObjComponent::solidAngle","shape");
  // Otherwise pass through the shifted point to the Object::solidAngle method, if non-unity
  // scaling also pass the scaling vector
  if((m_ScaleFactor-V3D(1.0,1.0,1.0)).norm()<1e-12)
      return shape->solidAngle( factorOutComponentPosition(observer) );
  else
      return shape->solidAngle( factorOutComponentPosition(observer)*m_ScaleFactor, m_ScaleFactor);
}

/**
 * Given an input estimate of the axis aligned (AA) bounding box (BB), return an improved set of values.
 * The AA BB is determined in the frame of the object and the initial estimate will be transformed there.
 * The returned BB will be the frame of the ObjComponent and may not be optimal.
 * Takes input axis aligned bounding box max and min points and calculates the bounding box for the
 * object and returns them back in max and min points. Cached values used after first call.
 *
 * @param xmax :: Maximum value for the bounding box in x direction
 * @param ymax :: Maximum value for the bounding box in y direction
 * @param zmax :: Maximum value for the bounding box in z direction
 * @param xmin :: Minimum value for the bounding box in x direction
 * @param ymin :: Minimum value for the bounding box in y direction
 * @param zmin :: Minimum value for the bounding box in z direction
 */
void ObjComponent::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const
{
  // If the form of this component is not defined, throw NullPointerException
  if (!shape) throw Kernel::Exception::NullPointerException("ObjComponent::solidAngle","shape");
  // transform input box and find new bounds
  Geometry::V3D V0(xmin,ymin,zmin),V1(xmin,ymin,zmax),V2(xmin,ymax,zmin),V3(xmin,ymax,zmax),
                V4(xmax,ymin,zmin),V5(xmax,ymin,zmax),V6(xmax,ymax,zmin),V7(xmax,ymax,zmax);
  std::vector<V3D> points;
  points.push_back(V0); points.push_back(V1); points.push_back(V2); points.push_back(V3);
  points.push_back(V4); points.push_back(V5); points.push_back(V6); points.push_back(V7);
  std::vector<V3D>::const_iterator vc;
  Geometry::V3D maxT(-DBL_MAX,-DBL_MAX,-DBL_MAX);
  Geometry::V3D minT(DBL_MAX,DBL_MAX,DBL_MAX);
  for(vc=points.begin();vc!=points.end();vc++)
  {
    Geometry::V3D pt=takeOutRotation( (*vc) - this->getPos() );
    for(int i=0;i<3;i++)
    {
      if(maxT[i]<pt[i]) maxT[i]=pt[i];
      if(minT[i]>pt[i]) minT[i]=pt[i];
    }
  }
  // pass bounds to getBoundingBox
  shape->getBoundingBox(maxT[0],maxT[1],maxT[2],minT[0],minT[1],minT[2]);
  //Apply scale factor
  maxT*=m_ScaleFactor;
  minT*=m_ScaleFactor;
  // transform result back and find new bounds by transforming all corner points and finding min/max box
  Geometry::V3D v0(minT[0],minT[1],minT[2]),v1(minT[0],minT[1],maxT[2]),v2(minT[0],maxT[1],minT[2]),v3(minT[0],maxT[1],maxT[2]),
                v4(maxT[0],minT[1],minT[2]),v5(maxT[0],minT[1],maxT[2]),v6(maxT[0],maxT[1],minT[2]),v7(maxT[0],maxT[1],maxT[2]);
  points.clear();
  points.push_back(v0); points.push_back(v1); points.push_back(v2); points.push_back(v3);
  points.push_back(v4); points.push_back(v5); points.push_back(v6); points.push_back(v7);
  maxT[0]=-DBL_MAX;maxT[1]=-DBL_MAX;maxT[2]=-DBL_MAX;
  minT[0]=DBL_MAX; minT[1]=DBL_MAX; minT[2]=DBL_MAX;
  Quat Rotate = this->getRotation();
  for(vc=points.begin();vc!=points.end();vc++)
  {
    Geometry::V3D pt= (*vc);
    Rotate.rotate(pt);
    pt+=this->getPos();
    for(int i=0;i<3;i++)
    {
      if(maxT[i]<pt[i]) maxT[i]=pt[i];
      if(minT[i]>pt[i]) minT[i]=pt[i];
    }
  }
  xmax=maxT[0]; ymax=maxT[1]; zmax=maxT[2];
  xmin=minT[0]; ymin=minT[1]; zmin=minT[2];
}

/**
 * Try to find a point that lies within (or on) the object
 * @param point On exit, set to the point value (if found)
 * @return 1 if point found, 0 otherwise
 */
int ObjComponent::getPointInObject(V3D& point) const
{
  // If the form of this component is not defined, throw NullPointerException
  if (!shape) throw Kernel::Exception::NullPointerException("ObjComponent::getPointInObject","shape");
  // Call the Object::getPointInObject method, which may give a point in Object coordinates
  int result= shape->getPointInObject( point );
  // transform point back to component space
  if(result)
  {
    //Scale up
    point*=m_ScaleFactor;
    Quat Rotate = this->getRotation();
    Rotate.rotate(point);
    point+=this->getPos();
  }
  else // scale back the point
  {
	point*=m_ScaleFactor;
  }
  return result;
}

/// Find the point that's in the same place relative to the constituent geometrical Object
/// if the position and rotation introduced by the Component is ignored
const V3D ObjComponent::factorOutComponentPosition(const V3D& point) const
{
  // First subtract the component's position, then undo the rotation
  return takeOutRotation( point - this->getPos() );
}

/// Rotates a point by the reverse of the component's rotation
const V3D ObjComponent::takeOutRotation(V3D point) const
{
  // Get the total rotation of this component and calculate the inverse (reverse rotation)
  Quat unRotate = this->getRotation();
  unRotate.inverse();
  // Now rotate our point by the angle calculated above
  unRotate.rotate(point);

  //Consider scaling factor
  point/=m_ScaleFactor;
  return point;
}

/**
 * Draws the objcomponent, If the handler is not set then this function does nothing.
 */
void ObjComponent::draw() const
{
	if(Handle()==NULL)return;
	//Render the ObjComponent and then render the object
	Handle()->Render();
}

/**
 * Draws the Object
 */
void ObjComponent::drawObject() const
{
	if(shape!=NULL)
		shape->draw();
}

/**
 * Initializes the ObjComponent for rendering, this function should be called before rendering.
 */
void ObjComponent::initDraw() const
{
	if(Handle()==NULL)return;
	//Render the ObjComponent and then render the object
	if(shape!=NULL)
		shape->initDraw();
	Handle()->Initialize();
}

} // namespace Geometry
} // namespace Mantid
