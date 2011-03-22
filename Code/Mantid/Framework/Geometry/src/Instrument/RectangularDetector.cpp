#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Rendering/BitmapGeometryHandler.h"
#include "MantidKernel/Exception.h"

#include <algorithm>
#include <stdexcept> 
#include <ostream>
namespace Mantid
{
namespace Geometry
{




/** Empty constructor
 */
RectangularDetector::RectangularDetector() : CompAssembly(), IObjComponent(NULL),
m_minDetId(0),m_maxDetId(0)
{

  setGeometryHandler(new BitmapGeometryHandler(this));
}


/** Constructor for a parametrized RectangularDetector
 * @param base: the base (un-parametrized) RectangularDetector
 * @param map: pointer to the ParameterMap
 * */
RectangularDetector::RectangularDetector(const RectangularDetector* base, const ParameterMap * map)
 : CompAssembly(base,map), IObjComponent(NULL), m_rectBase(base),m_minDetId(0),m_maxDetId(0)
{

  setGeometryHandler(new BitmapGeometryHandler(this));
}


/** Valued constructor
 *  @param n :: name of the assembly
 *  @param reference :: the parent Component
 * 
 * 	If the reference is an object of class Component,
 *  normal parenting apply. If the reference object is
 *  an assembly itself, then in addition to parenting
 *  this is registered as a children of reference.
 */
RectangularDetector::RectangularDetector(const std::string& n, IComponent* reference) :
    CompAssembly(n, reference), IObjComponent(NULL),m_minDetId(0),m_maxDetId(0)
{

  this->setName(n);
  setGeometryHandler(new BitmapGeometryHandler(this));
}


/** Destructor
 */
RectangularDetector::~RectangularDetector()
{
  
}

/** Clone method
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
 * @param X :: index from 0..m_xpixels-1
 * @param Y :: index from 0..m_ypixels-1
 * @return a pointer to the component in the assembly at the (X,Y) pixel position
 * @throw runtime_error if the x/y pixel width is not set, or X/Y are out of range
 */
boost::shared_ptr<Detector> RectangularDetector::getAtXY(int X, int Y) const
{
  if ((xpixels() <= 0) || (ypixels() <= 0))
    throw std::runtime_error("RectangularDetector::getAtXY: invalid X or Y width set in the object.");
  if ((X < 0) || (X >= xpixels()))
    throw std::runtime_error("RectangularDetector::getAtXY: X specified is out of range.");
  if ((Y < 0) || (Y >= ypixels()))
    throw std::runtime_error("RectangularDetector::getAtXY: Y specified is out of range.");

  //Find the index and return that.
  int i = X*ypixels() + Y;

  //Use the [] operator to return it (or create the parametrized version if needed)
  return boost::dynamic_pointer_cast<Detector>( this->operator[](i) );
}

//-------------------------------------------------------------------------------------------------
/// Returns the number of pixels in the X direction.
/// @return number of X pixels
int RectangularDetector::xpixels() const
{
  if (m_isParametrized)
    return m_rectBase->m_xpixels;
  else
    return this->m_xpixels;
}

//-------------------------------------------------------------------------------------------------
/// Returns the number of pixels in the X direction.
/// @return number of y pixels
int RectangularDetector::ypixels() const
{
  if (m_isParametrized)
    return m_rectBase->m_ypixels;
  else
    return this->m_ypixels;
}

//-------------------------------------------------------------------------------------------------
/// Returns the step size in the X direction
double RectangularDetector::xstep() const
{
  if (m_isParametrized)
    return m_rectBase->m_xstep;
  else
    return this->m_xstep;
}


//-------------------------------------------------------------------------------------------------
/// Returns the step size in the Y direction
double RectangularDetector::ystep() const
{
  if (m_isParametrized)
    return m_rectBase->m_ystep;
  else
    return this->m_ystep;
}

//-------------------------------------------------------------------------------------------------
/// Returns the start position in the X direction
double RectangularDetector::xstart() const
{
  if (m_isParametrized)
    return m_rectBase->m_xstart;
  else
    return this->m_xstart;
}


//-------------------------------------------------------------------------------------------------
/// Returns the start position in the Y direction
double RectangularDetector::ystart() const
{
  if (m_isParametrized)
    return m_rectBase->m_ystart;
  else
    return this->m_ystart;
}

//-------------------------------------------------------------------------------------------------
/// Returns the size in the X direction
double RectangularDetector::xsize() const
{
  if (m_isParametrized)
    return m_rectBase->m_xsize;
  else
    return this->m_xsize;
}


//-------------------------------------------------------------------------------------------------
/// Returns the size in the Y direction
double RectangularDetector::ysize() const
{
  if (m_isParametrized)
    return m_rectBase->m_ysize;
  else
    return this->m_ysize;
}

//-------------------------------------------------------------------------------------------------
/** Returns the position of the center of the pixel at x,y, relative to the center
 * of the RectangularDetector, in the plain X,Y coordinates of the
 * pixels (i.e. unrotated).
 * @param x :: x pixel integer
 * @param y :: y pixel integer
 * @return a V3D vector of the relative position
 */
V3D RectangularDetector::getRelativePosAtXY(int x, int y) const
{
  if (m_isParametrized)
    return m_rectBase->getRelativePosAtXY(x,y);
  else
    return V3D( m_xstart + m_xstep * x, m_ystart + m_ystep * y, 0);
}


//-------------------------------------------------------------------------------------------------
/** Initialize a RectangularDetector by creating all of the pixels
 * contained within it. You should have set the name, position
 * and rotation and facing of this object BEFORE calling this.
 *
 * @param shape :: a geometry Object containing the shape of each (individual) pixel in the assembly.
 *              All pixels must have the same shape.
 * @param xpixels :: number of pixels in X
 * @param xstart :: x-position of the 0-th pixel (in length units, normally meters)
 * @param xstep :: step size between pixels in the horizontal direction (in length units, normally meters)
 * @param ypixels :: number of pixels in Y
 * @param ystart :: y-position of the 0-th pixel (in length units, normally meters)
 * @param ystep :: step size between pixels in the vertical direction (in length units, normally meters)
 * @param idstart :: detector ID of the first pixel
 * @param idfillbyfirst_y :: set to true if ID numbers increase with Y indices first. That is: (0,0)=0; (0,1)=1, (0,2)=2 and so on.
 * @param idstepbyrow :: amount to increase the ID number on each row. e.g, if you fill by Y first,
 *            and set  idstepbyrow = 100, and have 50 Y pixels, you would get:
 *            (0,0)=0; (0,1)=1; ... (0,49)=49; (1,0)=100; (1,1)=101; etc.
 * @param idstep :: amount to increase each individual ID number with a row. e.g, if you fill by Y first,
 *            and idstep=100 and idstart=1 then (0,0)=1; (0,1)=101; and so on
 *
 */
void RectangularDetector::initialize(boost::shared_ptr<Object> shape,
    int xpixels, double xstart, double xstep,
    int ypixels, double ystart, double ystep,
    int idstart, bool idfillbyfirst_y, int idstepbyrow,
    int idstep
    )
{

  if (m_isParametrized)
    throw std::runtime_error("RectangularDetector::initialize() called for a parametrized RectangularDetector");


  m_xpixels = xpixels;
  m_ypixels = ypixels;
  m_xsize = xpixels * xstep;
  m_ysize = ypixels * ystep;
  m_xstart = xstart;
  m_ystart = ystart;
  m_xstep = xstep;
  m_ystep = ystep;
  mShape = shape;
 
  //TODO: some safety checks
  std::string name = this->getName();
  int minDetId=idstart,maxDetId=idstart;
  //Loop through all the pixels
  int ix, iy;
  for (ix=0; ix<m_xpixels; ix++)
    for (iy=0; iy<m_ypixels; iy++)
    {
      //Make the name
      std::ostringstream oss;
      oss << name << "(" << ix << "," << iy << ")";

      //Calculate its id and set it.
      int id;
      if (idfillbyfirst_y)
        id = idstart + ix * idstepbyrow + iy * idstep;
      else
        id = idstart + iy * idstepbyrow + ix * idstep;
     
      //minimum rectangular detector id
      if(id<minDetId)
      {
        minDetId=id;
      }
      //maximum rectangular detector id
      if(id>maxDetId)
      {
        maxDetId=id;
      }
      //Create the detector from the given id & shape and with THIS as the parent.
      Detector* detector = new Detector(oss.str(), id, shape, this);

      //Calculate the x,y position
      double x = xstart + ix * xstep;
      double y = ystart + iy * ystep;
      V3D pos(x,y,0);
      //Translate (relative to parent)
      detector->translate(pos);

      //Add it to this assembly
      this->add(detector);

    }
    m_minDetId=minDetId;
    m_maxDetId=maxDetId;

   
}

//-------------------------------------------------------------------------------------------------
/** Returns the minimum detector id
  * @return minimum detector id
 */
int RectangularDetector::minDetectorID()
{
  if (m_isParametrized)
    return m_rectBase->m_minDetId;
  return m_minDetId;
}

//-------------------------------------------------------------------------------------------------
/** Returns the maximum detector id
  * @return maximum detector id
 */
int RectangularDetector::maxDetectorID()
{
 if (m_isParametrized)
    return m_rectBase->m_maxDetId;
  return m_maxDetId;
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
  // Avoid compiler warning
  (void)point;
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::isValid() is not implemented.");
  return false;
}

//-------------------------------------------------------------------------------------------------
/// Does the point given lie on the surface of this object component?
bool RectangularDetector::isOnSide(const V3D& point) const
{
  // Avoid compiler warning
  (void)point;
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::isOnSide() is not implemented.");
  return false;
}


//-------------------------------------------------------------------------------------------------
///Checks whether the track given will pass through this Component.
int RectangularDetector::interceptSurface(Track& track) const
{
  // Avoid compiler warning
  (void)track;
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::interceptSurface() is not implemented.");
  return 0;
}


//-------------------------------------------------------------------------------------------------
/// Finds the approximate solid angle covered by the component when viewed from the point given
double RectangularDetector::solidAngle(const V3D& observer) const
{
  // Avoid compiler warning
  (void)observer;
  //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::solidAngle() is not implemented.");
  return 0;
}


//-------------------------------------------------------------------------------------------------
///Try to find a point that lies within (or on) the object
int RectangularDetector::getPointInObject(V3D& point) const
{
  // Avoid compiler warning
  (void)point;
 //TODO: Implement
  throw Kernel::Exception::NotImplementedError("RectangularDetector::getPointInObject() is not implemented.");
  return 0;
}

//-------------------------------------------------------------------------------------------------
/**
 * Return the bounding box (as 6 double values)
 * @param xmax :: maximum x of the bounding box
 * @param ymax :: maximum y of the bounding box
 * @param zmax :: maximum z of the bounding box
 * @param xmin :: minimum x of the bounding box
 * @param ymin :: minimum y of the bounding box
 * @param zmin :: minimum z of the bounding box
 */
void RectangularDetector::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const
{
  // Use cached box
  BoundingBox box;
  this->getBoundingBox(box);
  xmax = box.xMax();
  xmin = box.xMin();
  ymax = box.yMax();
  ymin = box.yMin();
  zmax = box.zMax();
  zmin = box.zMin();
}


void RectangularDetector::getBoundingBox(BoundingBox & assemblyBox) const
{
  if( !m_cachedBoundingBox )
  {
    m_cachedBoundingBox = new BoundingBox();
    //Get all the corner
    BoundingBox compBox;
    getAtXY(0, 0)->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    getAtXY(this->xpixels()-1, 0)->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    getAtXY(this->xpixels()-1, this->ypixels()-1)->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    getAtXY(0, this->ypixels()-1)->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
  }
  // Use cached box
  assemblyBox = *m_cachedBoundingBox;
}


/**
 * Return the number of pixels to make a texture in, given the
 * desired pixel size. A texture has to have 2^n pixels per side.
 * @param desired :: the requested pixel size
 * @return number of pixels for texture
 */
int getOneTextureSize(int desired)
{
  int size = 2;
  while (desired > size)
  {
    size = size * 2;
  }
  return size;
}

/**
 * Return the number of pixels to make a texture in, given the
 * desired pixel size. A texture has to have 2^n pixels per side.
 * @param xsize :: pixel texture size in x direction
 * @param ysize :: pixel texture size in y direction
 */
void RectangularDetector::getTextureSize(int & xsize, int & ysize) const
{
  xsize = getOneTextureSize(this->xpixels());
  ysize = getOneTextureSize(this->ypixels());
}



/** Set the texture ID to use when rendering the RectangularDetector
 */
void RectangularDetector::setTextureID(unsigned int textureID)
{
  mTextureID = textureID;
}

/** Return the texture ID to be used in plotting . */
unsigned int RectangularDetector::getTextureID() const
{
 return mTextureID;
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
const boost::shared_ptr<const Object> RectangularDetector::shape() const
{
  //std::cout << "RectangularDetector::Shape() called.\n";
  //throw Kernel::Exception::NotImplementedError("RectangularDetector::Shape() is not implemented.");


  // --- Create a cuboid shape for your pixels ----
  double szX=m_xpixels;
  double szY=m_ypixels;
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
  //Geometry::Object baseObj();
  //Looks like the object doesn't really contain any shape data; that is all in the GeometryHandler (i think)!

  //TODO: Create a shared pointer to this base object.

  //TODO: Here create the real shape of the detector and somehow get the texture on there.


  //TODO: Write a special geometry handler for RectangularDetector
//  boost::shared_ptr<GeometryHandler> handler(new GluGeometryHandler(Obj));
//  Obj->setGeometryHandler(handler);

  return cuboidShape;
}





//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// ------------ END OF IObjComponent methods ----------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------



/** Print information about elements in the assembly to a stream
 *  Overload the operator <<
 * @param os :: output stream
 * @param ass :: component assembly
 * @return stream representation of rectangular detector
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


} // Namespace Geometry
} // Namespace Mantid

