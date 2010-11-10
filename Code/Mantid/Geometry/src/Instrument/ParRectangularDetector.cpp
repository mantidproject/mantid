#include "MantidGeometry/Instrument/ParRectangularDetector.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
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

ParRectangularDetector::ParRectangularDetector(const RectangularDetector* base, ParameterMap_const_sptr map)
      :CompAssembly(base,map), IObjComponent(NULL),  mBase(base), m_cachedBoundingBox(NULL)
{
  setGeometryHandler(new BitmapGeometryHandler(this));
}

/** Copy constructor */
ParRectangularDetector::ParRectangularDetector(const ParRectangularDetector & other)
      :CompAssembly(other.mBase, other.m_map), IObjComponent(NULL), mBase(other.mBase), m_cachedBoundingBox(NULL)
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
}


/// Set the bounding box
void ParRectangularDetector::getBoundingBox(BoundingBox & assemblyBox) const
{
  //TODO: Male this more efficient
  CompAssembly::getBoundingBox(assemblyBox);
//  if( !m_cachedBoundingBox )
//  {
//    m_cachedBoundingBox = new BoundingBox();
//    //Get all the corner
//    BoundingBox compBox;
//    mBase->getAtXY(0, 0)->getBoundingBox(compBox);
//    m_cachedBoundingBox->grow(compBox);
//    mBase->getAtXY(this->xpixels()-1, 0)->getBoundingBox(compBox);
//    m_cachedBoundingBox->grow(compBox);
//    mBase->getAtXY(this->xpixels()-1, this->ypixels()-1)->getBoundingBox(compBox);
//    m_cachedBoundingBox->grow(compBox);
//    mBase->getAtXY(0, this->ypixels()-1)->getBoundingBox(compBox);
//    m_cachedBoundingBox->grow(compBox);
//  }
//  // Use cached box
//  assemblyBox = *m_cachedBoundingBox;
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

/// Returns the step size in the X direction
double ParRectangularDetector::xstep() const
{
  return mBase->xstep();
}

/// Returns the step size in the Y direction
double ParRectangularDetector::ystep() const
{
  return mBase->ystep();
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


/**
 * Return the number of pixels to make a texture in, given the
 * desired pixel size. A texture has to have 2^n pixels per side.
 */
void ParRectangularDetector::getTextureSize(int & xsize, int & ysize) const
{
  mBase->getTextureSize(xsize, ysize);
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

