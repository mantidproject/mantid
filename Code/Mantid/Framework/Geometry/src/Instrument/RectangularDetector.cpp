#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rendering/BitmapGeometryHandler.h"
#include "MantidKernel/Exception.h"
#include <algorithm>
#include <ostream>
#include <stdexcept>
#include "MantidGeometry/Instrument/RectangularDetectorPixel.h"

namespace Mantid {
namespace Geometry {

using Kernel::V3D;
using Kernel::Quat;
using Kernel::Matrix;

/** Empty constructor
 */
RectangularDetector::RectangularDetector()
    : CompAssembly(), IObjComponent(NULL), m_minDetId(0), m_maxDetId(0) {

  setGeometryHandler(new BitmapGeometryHandler(this));
}

/** Constructor for a parametrized RectangularDetector
 * @param base: the base (un-parametrized) RectangularDetector
 * @param map: pointer to the ParameterMap
 * */
RectangularDetector::RectangularDetector(const RectangularDetector *base,
                                         const ParameterMap *map)
    : CompAssembly(base, map), IObjComponent(NULL), m_rectBase(base),
      m_minDetId(0), m_maxDetId(0) {
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
RectangularDetector::RectangularDetector(const std::string &n,
                                         IComponent *reference)
    : CompAssembly(n, reference), IObjComponent(NULL), m_minDetId(0),
      m_maxDetId(0) {

  this->setName(n);
  setGeometryHandler(new BitmapGeometryHandler(this));
}

/** Destructor
 */
RectangularDetector::~RectangularDetector() {}

/** Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent *RectangularDetector::clone() const {
  return new RectangularDetector(*this);
}

//-------------------------------------------------------------------------------------------------
/** Return a pointer to the component in the assembly at the
 * (X,Y) pixel position.
 *
 * @param X :: index from 0..m_xpixels-1
 * @param Y :: index from 0..m_ypixels-1
 * @return a pointer to the component in the assembly at the (X,Y) pixel
 *position
 * @throw runtime_error if the x/y pixel width is not set, or X/Y are out of
 *range
 */
boost::shared_ptr<Detector> RectangularDetector::getAtXY(const int X,
                                                         const int Y) const {
  if ((xpixels() <= 0) || (ypixels() <= 0))
    throw std::runtime_error("RectangularDetector::getAtXY: invalid X or Y "
                             "width set in the object.");
  if ((X < 0) || (X >= xpixels()))
    throw std::runtime_error(
        "RectangularDetector::getAtXY: X specified is out of range.");
  if ((Y < 0) || (Y >= ypixels()))
    throw std::runtime_error(
        "RectangularDetector::getAtXY: Y specified is out of range.");

  // Find the index and return that.
  // int i = X*ypixels() + Y;
  // return boost::dynamic_pointer_cast<Detector>( this->operator[](i) );

  // Get to column
  ICompAssembly_sptr xCol =
      boost::dynamic_pointer_cast<ICompAssembly>(this->getChild(X));
  if (!xCol)
    throw std::runtime_error(
        "RectangularDetector::getAtXY: X specified is out of range.");
  return boost::dynamic_pointer_cast<Detector>(xCol->getChild(Y));
}

//-------------------------------------------------------------------------------------------------
/** Return the detector ID corresponding to the component in the assembly at the
 * (X,Y) pixel position. No bounds check is made!
 *
 * @param X :: index from 0..m_xpixels-1
 * @param Y :: index from 0..m_ypixels-1
 * @return detector ID int
 * @throw runtime_error if the x/y pixel width is not set, or X/Y are out of
 *range
 */
detid_t RectangularDetector::getDetectorIDAtXY(const int X, const int Y) const {
  const RectangularDetector *me = this;
  if (m_map)
    me = this->m_rectBase;

  if (me->m_idfillbyfirst_y)
    return me->m_idstart + X * me->m_idstepbyrow + Y * me->m_idstep;
  else
    return me->m_idstart + Y * me->m_idstepbyrow + X * me->m_idstep;
}

//-------------------------------------------------------------------------------------------------
/** Given a detector ID, return the X,Y coords into the rectangular detector
 *
 * @param detectorID :: detectorID
 * @return pair of (x,y)
 */
std::pair<int, int>
RectangularDetector::getXYForDetectorID(const int detectorID) const {
  const RectangularDetector *me = this;
  if (m_map)
    me = this->m_rectBase;

  int id = detectorID - me->m_idstart;
  if ((me->m_idstepbyrow == 0) || (me->m_idstep == 0))
    return std::pair<int, int>(-1, -1);
  int row = id / me->m_idstepbyrow;
  int col = (id % me->m_idstepbyrow) / me->m_idstep;

  if (me->m_idfillbyfirst_y) // x is the fast-changing axis
    return std::pair<int, int>(row, col);
  else
    return std::pair<int, int>(col, row);
}

//-------------------------------------------------------------------------------------------------
/// Returns the number of pixels in the X direction.
/// @return number of X pixels
int RectangularDetector::xpixels() const {
  if (m_map)
    return m_rectBase->m_xpixels;
  else
    return this->m_xpixels;
}

//-------------------------------------------------------------------------------------------------
/// Returns the number of pixels in the X direction.
/// @return number of y pixels
int RectangularDetector::ypixels() const {
  if (m_map)
    return m_rectBase->m_ypixels;
  else
    return this->m_ypixels;
}

//-------------------------------------------------------------------------------------------------
/// Returns the step size in the X direction
double RectangularDetector::xstep() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_rectBase, "scalex"))
      scaling = m_map->get(m_rectBase, "scalex")->value<double>();
    return m_rectBase->m_xstep * scaling;
  } else
    return this->m_xstep;
}

//-------------------------------------------------------------------------------------------------
/// Returns the step size in the Y direction
double RectangularDetector::ystep() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_rectBase, "scaley"))
      scaling = m_map->get(m_rectBase, "scaley")->value<double>();
    return m_rectBase->m_ystep * scaling;
  } else
    return this->m_ystep;
}

//-------------------------------------------------------------------------------------------------
/// Returns the start position in the X direction
double RectangularDetector::xstart() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_rectBase, "scalex"))
      scaling = m_map->get(m_rectBase, "scalex")->value<double>();
    return m_rectBase->m_xstart * scaling;
  } else
    return this->m_xstart;
}

//-------------------------------------------------------------------------------------------------
/// Returns the start position in the Y direction
double RectangularDetector::ystart() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_rectBase, "scaley"))
      scaling = m_map->get(m_rectBase, "scaley")->value<double>();
    return m_rectBase->m_ystart * scaling;
  } else
    return this->m_ystart;
}

//-------------------------------------------------------------------------------------------------
/// Returns the size in the X direction
double RectangularDetector::xsize() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_rectBase, "scalex"))
      scaling = m_map->get(m_rectBase, "scalex")->value<double>();
    return m_rectBase->m_xsize * scaling;
  } else
    return this->m_xsize;
}

//-------------------------------------------------------------------------------------------------
/// Returns the size in the Y direction
double RectangularDetector::ysize() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_rectBase, "scaley"))
      scaling = m_map->get(m_rectBase, "scaley")->value<double>();
    return m_rectBase->m_ysize * scaling;
  } else
    return this->m_ysize;
}

//-------------------------------------------------------------------------------------------------
/// Returns the idstart
int RectangularDetector::idstart() const {
  if (m_map)
    return m_rectBase->m_idstart;
  else
    return this->m_idstart;
}

//-------------------------------------------------------------------------------------------------
/// Returns the idfillbyfirst_y
bool RectangularDetector::idfillbyfirst_y() const {
  if (m_map)
    return m_rectBase->m_idfillbyfirst_y;
  else
    return this->m_idfillbyfirst_y;
}

//-------------------------------------------------------------------------------------------------
/// Returns the idstepbyrow
int RectangularDetector::idstepbyrow() const {
  if (m_map)
    return m_rectBase->m_idstepbyrow;
  else
    return this->m_idstepbyrow;
}

//-------------------------------------------------------------------------------------------------
/// Returns the idstep
int RectangularDetector::idstep() const {
  if (m_map)
    return m_rectBase->m_idstep;
  else
    return this->m_idstep;
}

//-------------------------------------------------------------------------------------------------
/** Returns the position of the center of the pixel at x,y, relative to the
 * center
 * of the RectangularDetector, in the plain X,Y coordinates of the
 * pixels (i.e. unrotated).
 * @param x :: x pixel integer
 * @param y :: y pixel integer
 * @return a V3D vector of the relative position
 */
V3D RectangularDetector::getRelativePosAtXY(int x, int y) const {
  if (m_map) {
    double scalex = 1.0;
    if (m_map->contains(m_rectBase, "scalex"))
      scalex = m_map->get(m_rectBase, "scalex")->value<double>();
    double scaley = 1.0;
    if (m_map->contains(m_rectBase, "scaley"))
      scaley = m_map->get(m_rectBase, "scaley")->value<double>();
    return m_rectBase->getRelativePosAtXY(x, y) * V3D(scalex, scaley, 1.0);
  } else
    return V3D(m_xstart + m_xstep * x, m_ystart + m_ystep * y, 0);
}

//-------------------------------------------------------------------------------------------------
/** Initialize a RectangularDetector by creating all of the pixels
 * contained within it. You should have set the name, position
 * and rotation and facing of this object BEFORE calling this.
 *
 * @param shape :: a geometry Object containing the shape of each (individual)
 *pixel in the assembly.
 *              All pixels must have the same shape.
 * @param xpixels :: number of pixels in X
 * @param xstart :: x-position of the 0-th pixel (in length units, normally
 *meters)
 * @param xstep :: step size between pixels in the horizontal direction (in
 *length units, normally meters)
 * @param ypixels :: number of pixels in Y
 * @param ystart :: y-position of the 0-th pixel (in length units, normally
 *meters)
 * @param ystep :: step size between pixels in the vertical direction (in length
 *units, normally meters)
 * @param idstart :: detector ID of the first pixel
 * @param idfillbyfirst_y :: set to true if ID numbers increase with Y indices
 *first. That is: (0,0)=0; (0,1)=1, (0,2)=2 and so on.
 * @param idstepbyrow :: amount to increase the ID number on each row. e.g, if
 *you fill by Y first,
 *            and set  idstepbyrow = 100, and have 50 Y pixels, you would get:
 *            (0,0)=0; (0,1)=1; ... (0,49)=49; (1,0)=100; (1,1)=101; etc.
 * @param idstep :: amount to increase each individual ID number with a row.
 *e.g, if you fill by Y first,
 *            and idstep=100 and idstart=1 then (0,0)=1; (0,1)=101; and so on
 *
 */
void RectangularDetector::initialize(boost::shared_ptr<Object> shape,
                                     int xpixels, double xstart, double xstep,
                                     int ypixels, double ystart, double ystep,
                                     int idstart, bool idfillbyfirst_y,
                                     int idstepbyrow, int idstep) {

  if (m_map)
    throw std::runtime_error("RectangularDetector::initialize() called for a "
                             "parametrized RectangularDetector");

  m_xpixels = xpixels;
  m_ypixels = ypixels;
  m_xsize = xpixels * xstep;
  m_ysize = ypixels * ystep;
  m_xstart = xstart;
  m_ystart = ystart;
  m_xstep = xstep;
  m_ystep = ystep;
  mShape = shape;

  /// IDs start here
  m_idstart = idstart;
  /// IDs are filled in Y fastest
  m_idfillbyfirst_y = idfillbyfirst_y;
  /// Step size in ID in each row
  m_idstepbyrow = idstepbyrow;
  /// Step size in ID in each col
  m_idstep = idstep;

  // Some safety checks
  if (m_xpixels <= 0)
    throw std::invalid_argument(
        "RectangularDetector::initialize(): xpixels should be > 0");
  if (m_ypixels <= 0)
    throw std::invalid_argument(
        "RectangularDetector::initialize(): ypixels should be > 0");

  std::string name = this->getName();
  int minDetId = idstart, maxDetId = idstart;
  // Loop through all the pixels
  int ix, iy;
  for (ix = 0; ix < m_xpixels; ix++) {
    // Create an ICompAssembly for each x-column
    std::ostringstream oss_col;
    oss_col << name << "(x=" << ix << ")";
    CompAssembly *xColumn = new CompAssembly(oss_col.str(), this);

    for (iy = 0; iy < m_ypixels; iy++) {
      // Make the name
      std::ostringstream oss;
      oss << name << "(" << ix << "," << iy << ")";

      // Calculate its id and set it.
      int id;
      id = this->getDetectorIDAtXY(ix, iy);

      // minimum rectangular detector id
      if (id < minDetId) {
        minDetId = id;
      }
      // maximum rectangular detector id
      if (id > maxDetId) {
        maxDetId = id;
      }
      // Create the detector from the given id & shape and with xColumn as the
      // parent.
      RectangularDetectorPixel *detector = new RectangularDetectorPixel(
          oss.str(), id, shape, xColumn, this, size_t(iy), size_t(ix));

      // Calculate the x,y position
      double x = xstart + ix * xstep;
      double y = ystart + iy * ystep;
      V3D pos(x, y, 0);
      // Translate (relative to parent). This gives the un-parametrized
      // position.
      detector->translate(pos);

      // Add it to the x-colum
      xColumn->add(detector);
      //      this->add(detector);
    }
  }
  m_minDetId = minDetId;
  m_maxDetId = maxDetId;
}

//-------------------------------------------------------------------------------------------------
/** Returns the minimum detector id
  * @return minimum detector id
 */
int RectangularDetector::minDetectorID() {
  if (m_map)
    return m_rectBase->m_minDetId;
  return m_minDetId;
}

//-------------------------------------------------------------------------------------------------
/** Returns the maximum detector id
  * @return maximum detector id
 */
int RectangularDetector::maxDetectorID() {
  if (m_map)
    return m_rectBase->m_maxDetId;
  return m_maxDetId;
}

//-------------------------------------------------------------------------------------------------
/// @copydoc Mantid::Geometry::CompAssembly::getComponentByName
boost::shared_ptr<const IComponent>
RectangularDetector::getComponentByName(const std::string &cname,
                                        int nlevels) const {
  // cache the detector's name as all the other names are longer
  const std::string NAME = this->getName();

  // if the component name is too short, just return
  if (cname.length() <= NAME.length())
    return boost::shared_ptr<const IComponent>();

  // check that the searched for name starts with the detector's
  // name as they are generated
  if (cname.substr(0, NAME.length()).compare(NAME) == 0) {
    return CompAssembly::getComponentByName(cname, nlevels);
  } else {
    return boost::shared_ptr<const IComponent>();
  }
}

//------------------------------------------------------------------------------------------------
/** Test the intersection of the ray with the children of the component
 *assembly, for InstrumentRayTracer.
 * Uses the knowledge of the RectangularDetector shape to significantly speed up
 *tracking.
 *
 * @param testRay :: Track under test. The results are stored here.
 * @param searchQueue :: If a child is a sub-assembly then it is appended for
 *later searching. Unused.
 */
void RectangularDetector::testIntersectionWithChildren(
    Track &testRay, std::deque<IComponent_const_sptr> & /*searchQueue*/) const {
  /// Base point (x,y,z) = position of pixel 0,0
  V3D basePoint;

  /// Vertical (y-axis) basis vector of the detector
  V3D vertical;

  /// Horizontal (x-axis) basis vector of the detector
  V3D horizontal;

  basePoint = getAtXY(0, 0)->getPos();
  horizontal = getAtXY(xpixels() - 1, 0)->getPos() - basePoint;
  vertical = getAtXY(0, ypixels() - 1)->getPos() - basePoint;

  // The beam direction
  V3D beam = testRay.direction();

  // From: http://en.wikipedia.org/wiki/Line-plane_intersection (taken on May 4,
  // 2011),
  // We build a matrix to solve the linear equation:
  Matrix<double> mat(3, 3);
  mat.setColumn(0, beam * -1.0);
  mat.setColumn(1, horizontal);
  mat.setColumn(2, vertical);
  mat.Invert();

  // Multiply by the inverted matrix to find t,u,v
  V3D tuv = mat * (testRay.startPoint() - basePoint);
  //  std::cout << tuv << "\n";

  // Intersection point
  V3D intersec = beam;
  intersec *= tuv[0];

  // t = coordinate along the line
  // u,v = coordinates along horizontal, vertical
  // (and correct for it being between 0, xpixels-1).  The +0.5 is because the
  // base point is at the CENTER of pixel 0,0.
  double u = (double(xpixels() - 1) * tuv[1] + 0.5);
  double v = (double(ypixels() - 1) * tuv[2] + 0.5);

  //  std::cout << u << ", " << v << "\n";

  // In indices
  int xIndex = int(u);
  int yIndex = int(v);

  // Out of range?
  if (xIndex < 0)
    return;
  if (yIndex < 0)
    return;
  if (xIndex >= xpixels())
    return;
  if (yIndex >= ypixels())
    return;

  // TODO: Do I need to put something smart here for the first 3 parameters?
  auto comp = getAtXY(xIndex, yIndex);
  testRay.addLink(intersec, intersec, 0.0, *(comp->shape()),
                  comp->getComponentID());
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// ------------ IObjComponent methods ----------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
/// Does the point given lie within this object component?
bool RectangularDetector::isValid(const V3D &point) const {
  // Avoid compiler warning
  (void)point;
  throw Kernel::Exception::NotImplementedError(
      "RectangularDetector::isValid() is not implemented.");
  return false;
}

//-------------------------------------------------------------------------------------------------
/// Does the point given lie on the surface of this object component?
bool RectangularDetector::isOnSide(const V3D &point) const {
  // Avoid compiler warning
  (void)point;
  throw Kernel::Exception::NotImplementedError(
      "RectangularDetector::isOnSide() is not implemented.");
  return false;
}

//-------------------------------------------------------------------------------------------------
/// Checks whether the track given will pass through this Component.
int RectangularDetector::interceptSurface(Track &track) const {
  // Avoid compiler warning
  (void)track;
  throw Kernel::Exception::NotImplementedError(
      "RectangularDetector::interceptSurface() is not implemented.");
  return 0;
}

//-------------------------------------------------------------------------------------------------
/// Finds the approximate solid angle covered by the component when viewed from
/// the point given
double RectangularDetector::solidAngle(const V3D &observer) const {
  // Avoid compiler warning
  (void)observer;
  throw Kernel::Exception::NotImplementedError(
      "RectangularDetector::solidAngle() is not implemented.");
  return 0;
}

//-------------------------------------------------------------------------------------------------
/// Try to find a point that lies within (or on) the object
int RectangularDetector::getPointInObject(V3D &point) const {
  // Avoid compiler warning
  (void)point;
  throw Kernel::Exception::NotImplementedError(
      "RectangularDetector::getPointInObject() is not implemented.");
  return 0;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the bounding box and store it in the given object. This is cached after
 * the first call.
 * @param assemblyBox :: A BoundingBox object that will be overwritten
 */
void RectangularDetector::getBoundingBox(BoundingBox &assemblyBox) const {
  if (!m_cachedBoundingBox) {
    m_cachedBoundingBox = new BoundingBox();
    // Get all the corner
    BoundingBox compBox;
    getAtXY(0, 0)->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    getAtXY(this->xpixels() - 1, 0)->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    getAtXY(this->xpixels() - 1, this->ypixels() - 1)->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    getAtXY(0, this->ypixels() - 1)->getBoundingBox(compBox);
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
int getOneTextureSize(int desired) {
  int size = 2;
  while (desired > size) {
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
void RectangularDetector::getTextureSize(int &xsize, int &ysize) const {
  xsize = getOneTextureSize(this->xpixels());
  ysize = getOneTextureSize(this->ypixels());
}

/** Set the texture ID to use when rendering the RectangularDetector
 */
void RectangularDetector::setTextureID(unsigned int textureID) {
  mTextureID = textureID;
}

/** Return the texture ID to be used in plotting . */
unsigned int RectangularDetector::getTextureID() const { return mTextureID; }

/**
 * Draws the objcomponent, If the handler is not set then this function does
 * nothing.
 */
void RectangularDetector::draw() const {
  // std::cout << "RectangularDetector::draw() called for " << this->getName()
  // << "\n";
  if (Handle() == NULL)
    return;
  // Render the ObjComponent and then render the object
  Handle()->Render();
}

/**
 * Draws the Object
 */
void RectangularDetector::drawObject() const {
  // std::cout << "RectangularDetector::drawObject() called for " <<
  // this->getName() << "\n";
  // if(shape!=NULL)    shape->draw();
}

/**
 * Initializes the ObjComponent for rendering, this function should be called
 * before rendering.
 */
void RectangularDetector::initDraw() const {
  // std::cout << "RectangularDetector::initDraw() called for " <<
  // this->getName() << "\n";
  if (Handle() == NULL)
    return;
  // Render the ObjComponent and then render the object
  // if(shape!=NULL)    shape->initDraw();
  Handle()->Initialize();
}

//-------------------------------------------------------------------------------------------------
/// Returns the shape of the Object
const boost::shared_ptr<const Object> RectangularDetector::shape() const {
  // --- Create a cuboid shape for your pixels ----
  double szX = m_xpixels;
  double szY = m_ypixels;
  double szZ = 0.5;
  std::ostringstream xmlShapeStream;
  xmlShapeStream << " <cuboid id=\"detector-shape\"> "
                 << "<left-front-bottom-point x=\"" << szX << "\" y=\"" << -szY
                 << "\" z=\"" << -szZ << "\"  /> "
                 << "<left-front-top-point  x=\"" << szX << "\" y=\"" << -szY
                 << "\" z=\"" << szZ << "\"  /> "
                 << "<left-back-bottom-point  x=\"" << -szX << "\" y=\"" << -szY
                 << "\" z=\"" << -szZ << "\"  /> "
                 << "<right-front-bottom-point  x=\"" << szX << "\" y=\"" << szY
                 << "\" z=\"" << -szZ << "\"  /> "
                 << "</cuboid>";

  std::string xmlCuboidShape(xmlShapeStream.str());
  Geometry::ShapeFactory shapeCreator;
  boost::shared_ptr<Geometry::Object> cuboidShape =
      shapeCreator.createShape(xmlCuboidShape);

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
std::ostream &operator<<(std::ostream &os, const RectangularDetector &ass) {
  ass.printSelf(os);
  os << "************************" << std::endl;
  os << "Number of children :" << ass.nelements() << std::endl;
  ass.printChildren(os);
  return os;
}

} // Namespace Geometry
} // Namespace Mantid
