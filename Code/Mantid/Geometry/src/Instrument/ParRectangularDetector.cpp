#include "MantidGeometry/Instrument/ParRectangularDetector.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ParCompAssembly.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/BitmapGeometryHandler.h"
#include "MantidGeometry/IRectangularDetector.h"
#include <algorithm>
#include <stdexcept> 
#include <ostream>

namespace Mantid
{
namespace Geometry
{

ParRectangularDetector::ParRectangularDetector(const RectangularDetector* base, const ParameterMap& map)
      :ParCompAssembly(base,map), IObjComponent(NULL),  mBase(base)
{
  setGeometryHandler(new BitmapGeometryHandler(this));
}

/** Copy constructor */
ParRectangularDetector::ParRectangularDetector(const ParRectangularDetector & other)
      :ParCompAssembly(other.mBase, other.m_map), IObjComponent(NULL), mBase(other.mBase)
{
  setGeometryHandler(new BitmapGeometryHandler(this));
}

/*! Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent* ParRectangularDetector::clone() const
{
  return new ParRectangularDetector(*this);
}

/// Get a detector at given XY indices.
boost::shared_ptr<Detector> ParRectangularDetector::getAtXY(int X, int Y) const
{
  return mBase->getAtXY(X,Y);
//  if ((xpixels() <= 0) || (ypixels() <= 0))
//  {
//    //std::cout << "xPixels " << xPixels << " yPixels " << yPixels << "\n";
//    throw std::runtime_error("RectangularDetector::getAtXY: invalid X or Y width set in the object.");
//  }
//  if ((X < 0) || (X >= xpixels()))
//    throw std::runtime_error("RectangularDetector::getAtXY: X specified is out of range.");
//  if ((Y < 0) || (Y >= ypixels()))
//    throw std::runtime_error("RectangularDetector::getAtXY: Y specified is out of range.");
//  //Find the index and return that.
//  int i = X*ypixels() + Y;
//  return boost::dynamic_pointer_cast<Detector>( this->operator[](i) );
}

/// Return the number of pixels in the X direction
int ParRectangularDetector::xpixels() const
{
  return mBase->xpixels();
}

/// Return the number of pixels in the Y direction
int ParRectangularDetector::ypixels() const
{
  return mBase->ypixels();
}

///Size in X of the detector
double ParRectangularDetector::xsize() const
{
  return mBase->xsize();
}

///Size in Y of the detector
double ParRectangularDetector::ysize() const
{
  return mBase->ysize();
}

V3D ParRectangularDetector::getRelativePosAtXY(int x, int y) const
{
  //This function will be the same for parametrized detectors, as long as they are NOT scaled.
  return mBase->getRelativePosAtXY(x,y);
}











//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// ------------ IObjComponent methods ----------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
/// Does the point given lie within this object component?
bool ParRectangularDetector::isValid(const V3D& point) const
{
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::isValid() is not implemented.");
  return false;
}

//-------------------------------------------------------------------------------------------------
/// Does the point given lie on the surface of this object component?
bool ParRectangularDetector::isOnSide(const V3D& point) const
{
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::isOnSide() is not implemented.");
  return false;
}


//-------------------------------------------------------------------------------------------------
///Checks whether the track given will pass through this Component.
int ParRectangularDetector::interceptSurface(Track& track) const
{
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::interceptSurface() is not implemented.");
  return 0;
}


//-------------------------------------------------------------------------------------------------
/// Finds the approximate solid angle covered by the component when viewed from the point given
double ParRectangularDetector::solidAngle(const V3D& observer) const
{
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::solidAngle() is not implemented.");
  return 0;
}


//-------------------------------------------------------------------------------------------------
///Try to find a point that lies within (or on) the object
int ParRectangularDetector::getPointInObject(V3D& point) const
{
 //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::getPointInObject() is not implemented.");
  return 0;
}

//-------------------------------------------------------------------------------------------------
/** Return the bounding box (as 6 double values)
 *
 */
void ParRectangularDetector::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const
{
  // Use cached box
  BoundingBox box;
  //TODO!
  //this->getBoundingBox(box);
  xmax = box.xMax();
  xmin = box.xMin();
  ymax = box.yMax();
  ymin = box.yMin();
  zmax = box.zMax();
  zmin = box.zMin();
}


/**
 * Draws the objcomponent, If the handler is not set then this function does nothing.
 */
void ParRectangularDetector::draw() const
{
  //std::cout << "RectangularDetector::draw() called for " << this->getName() << "\n";
  if(Handle()==NULL)return;
  //Render the ObjComponent and then render the object
  Handle()->Render();
}

/**
 * Draws the Object
 */
void ParRectangularDetector::drawObject() const
{
  //std::cout << "RectangularDetector::drawObject() called for " << this->getName() << "\n";
  //if(shape!=NULL)    shape->draw();
}

/**
 * Initializes the ObjComponent for rendering, this function should be called before rendering.
 */
void ParRectangularDetector::initDraw() const
{
  //std::cout << "RectangularDetector::initDraw() called for " << this->getName() << "\n";
  if(Handle()==NULL)return;
  //Render the ObjComponent and then render the object
  //if(shape!=NULL)    shape->initDraw();
  Handle()->Initialize();
}



//-------------------------------------------------------------------------------------------------
/// Returns the shape of the Object
const boost::shared_ptr<const Object> ParRectangularDetector::shape() const
{
  throw Kernel::Exception::NotImplementedError("ParRectangularDetector::Shape() is not implemented.");
}





//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// ------------ END OF IObjComponent methods ----------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------



} // Namespace Geometry
} // Namespace Mantid

