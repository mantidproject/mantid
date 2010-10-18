#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rendering/BitmapGeometryHandler.h"
#include "MantidKernel/Exception.h"

#include <algorithm>
#include <stdexcept> 
#include <ostream>
namespace Mantid
{
namespace Geometry
{



/*! Print information about elements in the assembly to a stream
 *  Overload the operator <<
 * @param os  :: output stream
 * @param ass :: component assembly
 *
 *  Loops through all components in the assembly
 *  and call printSelf(os).
 *  Also output the number of children
 */
std::ostream& operator<<(std::ostream& os, const RectangularDetector& ass)
{
  ass.printSelf(os);
  os << "************************" << std::endl;
  os << "Number of children :" << ass.nelements() << std::endl;
  ass.printChildren(os);
  return os;
}


/*! Empty constructor
 */
RectangularDetector::RectangularDetector() : CompAssembly(), IObjComponent(new BitmapGeometryHandler(this))
{
}

/*! Valued constructor
 *  @param n :: name of the assembly
 *  @param reference :: the parent Component
 * 
 * 	If the reference is an object of class Component,
 *  normal parenting apply. If the reference object is
 *  an assembly itself, then in addition to parenting
 *  this is registered as a children of reference.
 */
RectangularDetector::RectangularDetector(const std::string& n, Component* reference) :
    CompAssembly(n, reference), IObjComponent(new BitmapGeometryHandler(this))
{
  this->setName(n);
}

/*! Copy constructor
 *  @param other :: RectangularDetector to copy
 */
RectangularDetector::RectangularDetector(const RectangularDetector& other) :
  CompAssembly(other)
{
  //TODO: Copy other fields here
}

/*! Destructor
 */
RectangularDetector::~RectangularDetector()
{

}

/*! Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent* RectangularDetector::clone() const
{
  return new RectangularDetector(*this);
}



//-------------------------------------------------------------------------------------------------
/** Return a pointer to the component in the assembly at the
 * (X,Y) pixel position.
 *
 * @param X index from 0..xPixels-1
 * @param Y index from 0..yPixels-1
 * @throws runtime_error if the x/y pixel width is not set, or X/Y are out of range
 */
boost::shared_ptr<Detector> RectangularDetector::getAtXY(int X, int Y) const
{
  if ((xPixels <= 0) || (yPixels <= 0))
  {
    //std::cout << "xPixels " << xPixels << " yPixels " << yPixels << "\n";
    throw std::runtime_error("RectangularDetector::getAtXY: invalid X or Y width set in the object.");
  }
  if ((X < 0) || (X >= xPixels))
    throw std::runtime_error("RectangularDetector::getAtXY: X specified is out of range.");
  if ((Y < 0) || (Y >= yPixels))
    throw std::runtime_error("RectangularDetector::getAtXY: Y specified is out of range.");
  //Find the index and return that.
  int i = X*yPixels + Y;
  return boost::dynamic_pointer_cast<Detector>( this->operator[](i) );
}


//-------------------------------------------------------------------------------------------------
/// Returns the number of pixels in the X direction.
int RectangularDetector::xpixels() const
{
  return this->xPixels;
}


//-------------------------------------------------------------------------------------------------
/// Returns the number of pixels in the X direction.
int RectangularDetector::ypixels() const
{
  return this->yPixels;
}

//-------------------------------------------------------------------------------------------------
/** Returns the position of the pixel at x,y, relative to the center
 * of the RectangularDetector.
 */
V3D RectangularDetector::getRelativePosAtXY(int x, int y) const
{
  return V3D( m_xstart + m_xstep * x, m_ystart + m_ystep * y, 0);
}


//-------------------------------------------------------------------------------------------------
/** Initialize a RectangularDetector by creating all of the pixels
 * contained within it. You should have set the name, position
 * and rotation and facing of this object BEFORE calling this.
 *
 * @param shape a geometry Object containing the shape of the pixel
 * @param xpixels number of pixels in X
 * @param xstart x-position of the 0-th pixel (in length units, normally meters)
 * @param xstep step size between pixels in the horizontal direction (in length units, normally meters)
 * @param ypixels number of pixels in Y
 * @param ystart y-position of the 0-th pixel (in length units, normally meters)
 * @param ystep step size between pixels in the vertical direction (in length units, normally meters)
 * @param idstart detector ID of the first pixel
 * @param idfillbyfirst_y set to true if ID numbers increase with Y indices first. That is: (0,0)=0; (0,1)=1, (0,2)=2 and so on.
 * @param idstepbyrow amount to increase the ID number on each row. e.g, if you fill by Y first,
 *            and set  idstepbyrow = 100, and have 50 Y pixels, you would get:
 *            (0,0)=0; (0,1)=1; ... (0,49)=49; (1,0)=100; (1,1)=101; etc.
 *
 */
void RectangularDetector::initialize(boost::shared_ptr<Object> shape,
    int xpixels, double xstart, double xstep,
    int ypixels, double ystart, double ystep,
    int idstart, bool idfillbyfirst_y, int idstepbyrow
    )
{
  xPixels = xpixels;
  yPixels = ypixels;
  m_xsize = xpixels * xstep;
  m_ysize = ypixels * ystep;
  m_xstart = xstart;
  m_ystart = ystart;
  m_xstep = xstep;
  m_ystep = ystep;
  mShape = shape;
  //TODO: some safety checks

  std::string name = this->getName();

  //Loop through all the pixels
  int ix, iy;
  for (ix=0; ix<xPixels; ix++)
    for (iy=0; iy<yPixels; iy++)
    {
      //Make the name
      std::ostringstream oss;
      oss << name << "(" << ix << "," << iy << ")";

      //Create the detector from the given shape and with THIS as the parent.
      Detector* detector = new Detector(oss.str(), shape, this);

      //Calculate its id and set it.
      int id;
      if (idfillbyfirst_y)
        id = idstart + ix * idstepbyrow + iy;
      else
        id = idstart + iy * idstepbyrow + ix;
      detector->setID(id);

      //Calculate the x,y position
      double x = xstart + ix * xstep;
      double y = ystart + iy * ystep;
      V3D pos(x,y,0);
      //Translate (relative to parent)
      detector->translate(pos);

      //Add it to this assembly
      this->add(detector);

    }

}












//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// ------------ IObjComponent methods ----------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
/// Does the point given lie within this object component?
bool RectangularDetector::isValid(const V3D& point) const
{
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::isValid() is not implemented.");
  return false;
}

//-------------------------------------------------------------------------------------------------
/// Does the point given lie on the surface of this object component?
bool RectangularDetector::isOnSide(const V3D& point) const
{
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::isOnSide() is not implemented.");
  return false;
}


//-------------------------------------------------------------------------------------------------
///Checks whether the track given will pass through this Component.
int RectangularDetector::interceptSurface(Track& track) const
{
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::interceptSurface() is not implemented.");
  return 0;
}


//-------------------------------------------------------------------------------------------------
/// Finds the approximate solid angle covered by the component when viewed from the point given
double RectangularDetector::solidAngle(const V3D& observer) const
{
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::solidAngle() is not implemented.");
  return 0;
}


//-------------------------------------------------------------------------------------------------
///Try to find a point that lies within (or on) the object
int RectangularDetector::getPointInObject(V3D& point) const
{
 //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::getPointInObject() is not implemented.");
  return 0;
}

//-------------------------------------------------------------------------------------------------
/** Return the bounding box (as a BoundingBox object.
 * Saves the calculated bounding box in the cache.
 */
BoundingBox RectangularDetector::getBoundingBox() const
{
  //  if( !m_cachedBoundingBox )
//  {
//    m_cachedBoundingBox = new BoundingBox();
//    BoundingBox compBox;
//    this->getAtXY(0,0)->getBoundingBox(compBox);
//    m_cachedBoundingBox->grow(compBox);
//    this->getAtXY(xPixels-1,0)->getBoundingBox(compBox);
//    m_cachedBoundingBox->grow(compBox);
//    this->getAtXY(xPixels-1,yPixels-1)->getBoundingBox(compBox);
//    m_cachedBoundingBox->grow(compBox);
//    this->getAtXY(0,yPixels-1)->getBoundingBox(compBox);
//    m_cachedBoundingBox->grow(compBox);
//  }

    if( !m_cachedBoundingBox )
    {
      m_cachedBoundingBox = new BoundingBox();
//      V3D totalMinBound, totalMaxBound;
      //Get the bounding boxes of all 4 corners
      for (int x=0; x < this->xpixels(); x += this->xpixels()-1)
        for (int y=0; y < this->ypixels(); y += this->ypixels()-1)
        {
          //Get the BB of the detector at that corner
          double xmin,ymin,zmin,xmax,ymax,zmax;
          xmin=ymin=zmin=-1000;
          xmax=ymax=zmax=1000;
          this->getAtXY(x,y)->getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
          BoundingBox localBox(xmax,ymax,zmax,xmin,ymin,zmin);
          m_cachedBoundingBox->grow(localBox);
        }
    }
  //Copy to the result
  return *m_cachedBoundingBox;
}


//-------------------------------------------------------------------------------------------------
/** Return the bounding box (as 6 double values)
 *
 */
void RectangularDetector::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const
{
  // Use cached box
  BoundingBox box = this->getBoundingBox();

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
void RectangularDetector::draw() const
{
  //std::cout << "RectangularDetector::draw() called for " << this->getName() << "\n";
  if(Handle()==NULL)return;
  //Render the ObjComponent and then render the object
  Handle()->Render();
}

/**
 * Draws the Object
 */
void RectangularDetector::drawObject() const
{
  //std::cout << "RectangularDetector::drawObject() called for " << this->getName() << "\n";
  //if(shape!=NULL)    shape->draw();
}

/**
 * Initializes the ObjComponent for rendering, this function should be called before rendering.
 */
void RectangularDetector::initDraw() const
{
  //std::cout << "RectangularDetector::initDraw() called for " << this->getName() << "\n";
  if(Handle()==NULL)return;
  //Render the ObjComponent and then render the object
  //if(shape!=NULL)    shape->initDraw();
  Handle()->Initialize();
}



//-------------------------------------------------------------------------------------------------
/// Returns the shape of the Object
const boost::shared_ptr<const Object> RectangularDetector::Shape() const
{
  //std::cout << "RectangularDetector::Shape() called.\n";
  //throw Kernel::Exception::NotImplementedError("RectangularDetector::Shape() is not implemented.");


      // --- Create a cuboid shape for your pixels ----
      double szX=xPixels;
      double szY=yPixels;
      double szZ=0.5;
      std::ostringstream xmlShapeStream;
      xmlShapeStream
          << " <cuboid id=\"detector-shape\"> "
          << "<left-front-bottom-point x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> "
          << "<left-front-top-point  x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<szZ<<"\"  /> "
          << "<left-back-bottom-point  x=\""<<-szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> "
          << "<right-front-bottom-point  x=\""<<szX<<"\" y=\""<<szY<<"\" z=\""<<-szZ<<"\"  /> "
          << "</cuboid>";

      std::string xmlCuboidShape(xmlShapeStream.str());
      Geometry::ShapeFactory shapeCreator;
      boost::shared_ptr<Geometry::Object> cuboidShape = shapeCreator.createShape(xmlCuboidShape);

  //TODO: Create the object of the right shape
  Geometry::Object baseObj();
  //Looks like the object doesn't really contain any shape data; that is all in the GeometryHandler (i think)!

  //TODO: Create a shared pointer to this base object.

  //TODO: Here create the real shape of the detector and somehow get the texture on there.


  //TODO: Write a special geometry handler for RectangularDetector
//  boost::shared_ptr<GeometryHandler> handler(new GluGeometryHandler(Obj));
//  Obj->setGeometryHandler(handler);

  return cuboidShape;
}





} // Namespace Geometry
} // Namespace Mantid

