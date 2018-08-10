#include "MantidGeometry/Instrument/GridDetector.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/GridDetectorPixel.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Matrix.h"
#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <ostream>
#include <stdexcept>

namespace Mantid {
namespace Geometry {

using Kernel::Matrix;
using Kernel::V3D;

/** Empty constructor
 */
GridDetector::GridDetector()
    : CompAssembly(), IObjComponent(nullptr), m_gridBase(nullptr),
      m_minDetId(0), m_maxDetId(0) {

  init();
  setGeometryHandler(new GeometryHandler(this));
}

/** Constructor for a parametrized GridDetector
 * @param base: the base (un-parametrized) GridDetector
 * @param map: pointer to the ParameterMap
 * */
GridDetector::GridDetector(const GridDetector *base, const ParameterMap *map)
    : CompAssembly(base, map), IObjComponent(nullptr), m_gridBase(base),
      m_minDetId(0), m_maxDetId(0) {
  init();
  setGeometryHandler(new GeometryHandler(this));
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
GridDetector::GridDetector(const std::string &n, IComponent *reference)
    : CompAssembly(n, reference), IObjComponent(nullptr), m_gridBase(nullptr),
      m_minDetId(0), m_maxDetId(0) {
  init();
  this->setName(n);
  setGeometryHandler(new GeometryHandler(this));
}

bool GridDetector::compareName(const std::string &proposedMatch) {
  static const boost::regex exp("GridDetector|gridDetector|"
                                "griddetector|grid_detector");

  return boost::regex_match(proposedMatch, exp);
}

void GridDetector::init() {
  m_xpixels = m_ypixels = m_ypixels = 0;
  m_xsize = m_ysize = m_ysize = 0;
  m_xstart = m_ystart = m_zstart = 0;
  m_xstep = m_ystep = m_zstep = 0;
  m_minDetId = m_maxDetId = 0;
  m_idstart = 0;
  m_idfillbyfirst_y = false;
  m_idfillbyfirst_z = false;
  m_idstepbyrow = 0;
  m_idlayerstep = 0;
  m_idstep = 0;
}

/** Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent *GridDetector::clone() const { return new GridDetector(*this); }

//-------------------------------------------------------------------------------------------------
/** Return a pointer to the component in the assembly at the
 * (X,Y) pixel position.
 *
 * @param x :: index from 0..m_xpixels-1
 * @param y :: index from 0..m_ypixels-1
 * @param z :: index from 0..m_zpixels-1
 * @return a pointer to the component in the assembly at the (x,y,z) pixel
 *position
 * @throw runtime_error if the x/y/z pixel width is not set, or x/y/z are out of
 *range
 */
boost::shared_ptr<Detector> GridDetector::getAtXYZ(const int x, const int y,
                                                   const int z) const {
  if ((xpixels() <= 0) || (ypixels() <= 0))
    throw std::runtime_error("GridDetector::getAtXY: invalid X or Y "
                             "width set in the object.");
  if ((x < 0) || (x >= xpixels()))
    throw std::runtime_error(
        "GridDetector::getAtXYZ: x specified is out of range.");
  if ((y < 0) || (y >= ypixels()))
    throw std::runtime_error(
        "GridDetector::getAtXYZ: y specified is out of range.");
  if ((z < 0) || (z >= zpixels()))
    throw std::runtime_error(
        "GridDetector::getAtXYZ: z specified is out of range.");

  // Find the index and return that.
  // Get to layer
  auto *zLayer = this;
  if (m_zpixels > 0) {
    auto zLayer = boost::dynamic_pointer_cast<ICompAssembly>(this->getChild(z));
    if (!zLayer)
      throw std::runtime_error(
          "GridDetector::getAtXYZ: z specified is out of range.");
  }

  auto xCol = boost::dynamic_pointer_cast<ICompAssembly>(zLayer->getChild(x));

  if (!xCol)
    throw std::runtime_error(
        "GridDetector::getAtXYZ: x specified is out of range.");

  return boost::dynamic_pointer_cast<Detector>(xCol->getChild(y));
}

//-------------------------------------------------------------------------------------------------
/** Return the detector ID corresponding to the component in the assembly at the
 * (X,Y) pixel position. No bounds check is made!
 *
 * @param x :: index from 0..m_xpixels-1
 * @param y :: index from 0..m_ypixels-1
 * @param z :: index from 0..m_zpixels-1
 * @return detector ID int
 * @throw runtime_error if the x/y/z pixel width is not set, or X/Y are out of
 *range
 */
detid_t GridDetector::getDetectorIDAtXYZ(const int x, const int y,
                                         const int z) const {
  const GridDetector *me = this;
  if (m_map)
    me = this->m_gridBase;

  if (me->m_idfillbyfirst_y)
    return me->m_idstart + x * me->m_idstepbyrow + y * me->m_idstep +
           z * me->m_idlayerstep;
  else if (me->m_idfillbyfirst_z)
    return me->m_idstart + z * me->m_idstep+ y * me->m_idlayerstep +
           x * me->m_idstepbyrow;
  else
    return me->m_idstart + y * me->m_idstepbyrow + x * me->m_idstep +
           z * me->m_idlayerstep;
}

//-------------------------------------------------------------------------------------------------
/** Given a detector ID, return the X,Y,Z coords into the grid detector
 *
 * @param detectorID :: detectorID
 * @return tuple of (x,y,z)
 */
std::tuple<int, int, int>
GridDetector::getXYZForDetectorID(const int detectorID) const {
  const GridDetector *me = this;
  if (m_map)
    me = this->m_gridBase;

  int id = detectorID - me->m_idstart;
  if ((me->m_idstepbyrow == 0) || (me->m_idstep == 0))
    return std::tuple<int, int, int>(-1, -1, -1);
  int row = id / me->m_idstepbyrow;
  int col = (id % me->m_idstepbyrow) / me->m_idstep;
  int layer = ((id % me->m_idstepbyrow) % me->m_idstep) / me->m_idlayerstep;

  if (me->m_idfillbyfirst_y) // x is the fast-changing axis
    return std::tuple<int, int, int>(row, col, layer);
  else if (me->m_idfillbyfirst_z)
    return std::tuple<int, int, int>(layer, col, row);
  else
    return std::tuple<int, int, int>(col, row, layer);
}

//-------------------------------------------------------------------------------------------------
/// Returns the number of pixels in the X direction.
/// @return number of X pixels
int GridDetector::xpixels() const {
  if (m_map)
    return m_gridBase->m_xpixels;
  else
    return this->m_xpixels;
}

//-------------------------------------------------------------------------------------------------
/// Returns the number of pixels in the Y direction.
/// @return number of y pixels
int GridDetector::ypixels() const {
  if (m_map)
    return m_gridBase->m_ypixels;
  else
    return this->m_ypixels;
}

//-------------------------------------------------------------------------------------------------
/// Returns the number of pixels in the Z direction.
/// @return number of z pixels
int GridDetector::zpixels() const {
  if (m_map)
    return m_gridBase->m_zpixels;
  else
    return this->m_zpixels;
}

//-------------------------------------------------------------------------------------------------
/// Returns the step size in the X direction
double GridDetector::xstep() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scalex"))
      scaling = m_map->get(m_gridBase, "scalex")->value<double>();
    return m_gridBase->m_xstep * scaling;
  } else
    return this->m_xstep;
}

//-------------------------------------------------------------------------------------------------
/// Returns the step size in the Y direction
double GridDetector::ystep() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scaley"))
      scaling = m_map->get(m_gridBase, "scaley")->value<double>();
    return m_gridBase->m_ystep * scaling;
  } else
    return this->m_ystep;
}

//-------------------------------------------------------------------------------------------------
/// Returns the step size in the Z direction
double GridDetector::zstep() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scalez"))
      scaling = m_map->get(m_gridBase, "scalez")->value<double>();
    return m_gridBase->m_zstep * scaling;
  } else
    return this->m_zstep;
}

//-------------------------------------------------------------------------------------------------
/// Returns the start position in the X direction
double GridDetector::xstart() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scalex"))
      scaling = m_map->get(m_gridBase, "scalex")->value<double>();
    return m_gridBase->m_xstart * scaling;
  } else
    return this->m_xstart;
}

//-------------------------------------------------------------------------------------------------
/// Returns the start position in the Y direction
double GridDetector::ystart() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scaley"))
      scaling = m_map->get(m_gridBase, "scaley")->value<double>();
    return m_gridBase->m_ystart * scaling;
  } else
    return this->m_ystart;
}

//-------------------------------------------------------------------------------------------------
/// Returns the start position in the Z direction
double GridDetector::zstart() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scalez"))
      scaling = m_map->get(m_gridBase, "scalez")->value<double>();
    return m_gridBase->m_zstart * scaling;
  } else
    return this->m_zstart;
}

//-------------------------------------------------------------------------------------------------
/// Returns the size in the X direction
double GridDetector::xsize() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scalex"))
      scaling = m_map->get(m_gridBase, "scalex")->value<double>();
    return m_gridBase->m_xsize * scaling;
  } else
    return this->m_xsize;
}

//-------------------------------------------------------------------------------------------------
/// Returns the size in the Y direction
double GridDetector::ysize() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scaley"))
      scaling = m_map->get(m_gridBase, "scaley")->value<double>();
    return m_gridBase->m_ysize * scaling;
  } else
    return this->m_ysize;
}

//-------------------------------------------------------------------------------------------------
/// Returns the size in the Z direction
double GridDetector::zsize() const {
  if (m_map) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scalez"))
      scaling = m_map->get(m_gridBase, "scalez")->value<double>();
    return m_gridBase->m_zsize * scaling;
  } else
    return this->m_zsize;
}

//-------------------------------------------------------------------------------------------------
/// Returns the idstart
int GridDetector::idstart() const {
  if (m_map)
    return m_gridBase->m_idstart;
  else
    return this->m_idstart;
}

//-------------------------------------------------------------------------------------------------
/// Returns the idfillbyfirst_y
bool GridDetector::idfillbyfirst_y() const {
  if (m_map)
    return m_gridBase->m_idfillbyfirst_y;
  else
    return this->m_idfillbyfirst_y;
}

//-------------------------------------------------------------------------------------------------
/// Returns the idfillbyfirst_z
bool GridDetector::idfillbyfirst_z() const {
  if (m_map)
    return m_gridBase->m_idfillbyfirst_z;
  else
    return this->m_idfillbyfirst_z;
}

//-------------------------------------------------------------------------------------------------
/// Returns the idstepbyrow
int GridDetector::idstepbyrow() const {
  if (m_map)
    return m_gridBase->m_idstepbyrow;
  else
    return this->m_idstepbyrow;
}

//-------------------------------------------------------------------------------------------------
/// Returns the layer idstep
int GridDetector::idlayerstep() const {
  if (m_map)
    return m_gridBase->m_idlayerstep;
  else
    return this->m_idlayerstep;
}

//-------------------------------------------------------------------------------------------------
/// Returns the idstep
int GridDetector::idstep() const {
  if (m_map)
    return m_gridBase->m_idstep;
  else
    return this->m_idstep;
}

//-------------------------------------------------------------------------------------------------
/** Returns the position of the center of the pixel at x,y, relative to the
 * center
 * of the GridDetector, in the plain X,Y coordinates of the
 * pixels (i.e. unrotated).
 * @param x :: x pixel integer
 * @param y :: y pixel integer
 * @param y :: z pixel integer
 * @return a V3D vector of the relative position
 */
V3D GridDetector::getRelativePosAtXYZ(int x, int y, int z) const {
  if (m_map) {
    double scalex = 1.0;
    if (m_map->contains(m_gridBase, "scalex"))
      scalex = m_map->get(m_gridBase, "scalex")->value<double>();
    double scaley = 1.0;
    if (m_map->contains(m_gridBase, "scaley"))
      scaley = m_map->get(m_gridBase, "scaley")->value<double>();
    double scalez = 1.0;
    if (m_map->contains(m_gridBase, "scalez"))
      scaley = m_map->get(m_gridBase, "scalez")->value<double>();
    return m_gridBase->getRelativePosAtXYZ(x, y, z) *
           V3D(scalex, scaley, scalez);
  } else
    return V3D(m_xstart + m_xstep * x, m_ystart + m_ystep * y,
               m_zstart + m_zstep * z);
}

void GridDetector::createLayer(const std::string &name, CompAssembly *parent,
                               int iz, int &minDetID, int &maxDetID) {
  int ix, iy;
  for (ix = 0; ix < m_xpixels; ++ix) {
    // Create an ICompAssembly for each x-column
    std::ostringstream oss_col;
    oss_col << name << "(x=" << ix << ")";
    auto *xColumn = new CompAssembly(oss_col.str(), parent);

    for (iy = 0; iy < m_ypixels; ++iy) {
      // Make the name
      std::ostringstream oss;
      oss << name << "(" << ix << "," << iy << ")";

      // Calculate its id and set it.
      int id;
      id = this->getDetectorIDAtXYZ(ix, iy, iz);

      // minimum grid detector id
      if (id < minDetID) {
        minDetID = id;
      }
      // maximum grid detector id
      if (id > maxDetID) {
        maxDetID = id;
      }
      // Create the detector from the given id & shape and with xColumn as the
      // parent.
      auto *detector =
          new GridDetectorPixel(oss.str(), id, m_shape, xColumn, this, size_t(iy),
                                size_t(ix), size_t(iz));

      // Calculate the x,y,z position
      double x = m_xstart + ix * m_xstep;
      double y = m_ystart + iy * m_ystep;
      double z = m_zstart + iz * m_zstep;
      V3D pos(x, y, z);
      // Translate (relative to parent). This gives the un-parametrized
      // position.
      detector->translate(pos);

      // Add it to the x-column
      xColumn->add(detector);
    }
  }
}
//-------------------------------------------------------------------------------------------------
/** Initialize a GridDetector by creating all of the pixels
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
 * @param zpixels :: number of pixels in Z
 * @param zstart :: z-position of the 0-th pixel (in length units, normally
 *meters)
 * @param zstep :: step size between pixels in the beam direction (in length
 *units, normally meters)
 * @param idstart :: detector ID of the first pixel
 * @param idfillbyfirst_y :: set to true if ID numbers increase with Y indices
 *first. That is: (0,0)=0; (0,1)=1, (0,2)=2 and so on.
 * @param idfillbyfirst_z :: set to true if ID numbers increase with Z indices
 *first. That is: (0,0,0)=0; (0,0,1)=1, (0,0,2)=2 and so on.
 * @param idstepbyrow :: amount to increase the ID number on each row. e.g, if
 *you fill by Y first,
 *            and set  idstepbyrow = 100, and have 50 Y pixels, you would get:
 *            (0,0)=0; (0,1)=1; ... (0,49)=49; (1,0)=100; (1,1)=101; etc.
 * @param idlayerstep :: amount to increase each individual ID number with a
 *layer.
 * @param idstep :: amount to increase each individual ID number with a row.
 *e.g, if you fill by Y first,
 *            and idstep=100 and idstart=1 then (0,0)=1; (0,1)=101; and so on
 *
 */
void GridDetector::initialize(boost::shared_ptr<IObject> shape, int xpixels,
                              double xstart, double xstep, int ypixels,
                              double ystart, double ystep, int zpixels,
                              double zstart, double zstep, int idstart,
                              bool idfillbyfirst_y, bool idfillbyfirst_z,
                              int idstepbyrow, int idlayerstep, int idstep) {

  if (m_map)
    throw std::runtime_error("GridDetector::initialize() called for a "
                             "parametrized GridDetector");

  m_xpixels = xpixels;
  m_ypixels = ypixels;
  m_zpixels = zpixels;
  m_xsize = xpixels * xstep;
  m_ysize = ypixels * ystep;
  m_zsize = zpixels * zstep;
  m_xstart = xstart;
  m_ystart = ystart;
  m_zstart = zstart;
  m_xstep = xstep;
  m_ystep = ystep;
  m_zstep = zstep;
  m_shape = shape;

  /// IDs start here
  m_idstart = idstart;
  /// IDs are filled in Y fastest
  m_idfillbyfirst_y = idfillbyfirst_y;
  /// IDs are filled by Y fastest
  m_idfillbyfirst_z = idfillbyfirst_z;
  /// Step size in ID in each row
  m_idstepbyrow = idstepbyrow;
  /// Step size in ID in each layer
  m_idlayerstep = idlayerstep;
  /// Step size in ID in each col
  m_idstep = idstep;

  // Some safety checks
  if (idfillbyfirst_y && idfillbyfirst_z)
    throw std::invalid_argument(
        "GridDetector::initialize(): cannot fill by both y and z initially");
  if (m_xpixels <= 0)
    throw std::invalid_argument(
        "GridDetector::initialize(): xpixels should be > 0");
  if (m_ypixels <= 0)
    throw std::invalid_argument(
        "GridDetector::initialize(): ypixels should be > 0");

  std::string name = this->getName();
  int minDetId = idstart, maxDetId = idstart;
  // Loop through all the pixels
  if (m_zpixels > 0) {
    int iz;
    for (iz = 0; iz < m_zpixels; ++iz) {
      // Create an ICompAssembly for each x-column
      std::ostringstream oss_layer;
      oss_layer << name << "(z=" << iz << ")";
      CompAssembly *zLayer = new CompAssembly(oss_layer.str(), this);

      createLayer(name, zLayer, iz, minDetId, maxDetId);
    }
  } else
    createLayer(name, this, 0, minDetId, maxDetId);

  m_minDetId = minDetId;
  m_maxDetId = maxDetId;
}

//-------------------------------------------------------------------------------------------------
/** Returns the minimum detector id
 * @return minimum detector id
 */
int GridDetector::minDetectorID() {
  if (m_map)
    return m_gridBase->m_minDetId;
  return m_minDetId;
}

//-------------------------------------------------------------------------------------------------
/** Returns the maximum detector id
 * @return maximum detector id
 */
int GridDetector::maxDetectorID() {
  if (m_map)
    return m_gridBase->m_maxDetId;
  return m_maxDetId;
}

//-------------------------------------------------------------------------------------------------
/// @copydoc Mantid::Geometry::CompAssembly::getComponentByName
boost::shared_ptr<const IComponent>
GridDetector::getComponentByName(const std::string &cname, int nlevels) const {
  // exact matches
  if (cname == this->getName())
    return boost::shared_ptr<const IComponent>(this);

  // cache the detector's name as all the other names are longer
  // The extra ( is because all children of this have that as the next character
  // and this prevents Bank11 matching Bank 1
  const std::string MEMBER_NAME = this->getName() + "(";

  // check that the searched for name starts with the detector's
  // name as they are generated
  if (cname.substr(0, MEMBER_NAME.length()) != MEMBER_NAME) {
    return boost::shared_ptr<const IComponent>();
  } else {
    return CompAssembly::getComponentByName(cname, nlevels);
  }
}

//------------------------------------------------------------------------------------------------
/** Test the intersection of the ray with the children of the component
 *assembly, for InstrumentRayTracer.
 * Uses the knowledge of the GridDetector shape to significantly speed up
 *tracking.
 *
 * @param testRay :: Track under test. The results are stored here.
 * @param searchQueue :: If a child is a sub-assembly then it is appended for
 *later searching. Unused.
 */
void GridDetector::testIntersectionWithChildren(
    Track &testRay, std::deque<IComponent_const_sptr> & /*searchQueue*/) const {
  /// Base point (x,y,z) = position of pixel 0,0
  V3D basePoint;

  /// Vertical (y-axis) basis vector of the detector
  V3D vertical;

  /// Horizontal (x-axis) basis vector of the detector
  V3D horizontal;

  basePoint = getAtXYZ(0, 0, 0)->getPos();
  horizontal = getAtXYZ(xpixels() - 1, 0, 0)->getPos() - basePoint;
  vertical = getAtXYZ(0, ypixels() - 1, 0)->getPos() - basePoint;

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
  auto comp = getAtXYZ(xIndex, yIndex, 0);
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
bool GridDetector::isValid(const V3D &) const {
  throw Kernel::Exception::NotImplementedError(
      "GridDetector::isValid() is not implemented.");
}

//-------------------------------------------------------------------------------------------------
/// Does the point given lie on the surface of this object component?
bool GridDetector::isOnSide(const V3D &) const {
  throw Kernel::Exception::NotImplementedError(
      "GridDetector::isOnSide() is not implemented.");
}

//-------------------------------------------------------------------------------------------------
/// Checks whether the track given will pass through this Component.
int GridDetector::interceptSurface(Track &) const {
  throw Kernel::Exception::NotImplementedError(
      "GridDetector::interceptSurface() is not implemented.");
}

//-------------------------------------------------------------------------------------------------
/// Finds the approximate solid angle covered by the component when viewed from
/// the point given
double GridDetector::solidAngle(const V3D &) const {
  throw Kernel::Exception::NotImplementedError(
      "GridDetector::solidAngle() is not implemented.");
}

//-------------------------------------------------------------------------------------------------
/// Try to find a point that lies within (or on) the object
int GridDetector::getPointInObject(V3D &) const {
  throw Kernel::Exception::NotImplementedError(
      "GridDetector::getPointInObject() is not implemented.");
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the bounding box and store it in the given object. This is cached after
 * the first call.
 * @param assemblyBox :: A BoundingBox object that will be overwritten
 */
void GridDetector::getBoundingBox(BoundingBox &assemblyBox) const {
  if (m_map) {
    if (hasComponentInfo()) {
      assemblyBox = m_map->componentInfo().boundingBox(index(), &assemblyBox);
      return;
    }
  }
  if (!m_cachedBoundingBox) {
    m_cachedBoundingBox = new BoundingBox();
    // Get all the corner
    BoundingBox compBox;
    getAtXYZ(0, 0, 0)->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    getAtXYZ(this->xpixels() - 1, 0, 0)->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    getAtXYZ(this->xpixels() - 1, this->ypixels() - 1, 0)
        ->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    getAtXYZ(0, this->ypixels() - 1, 0)->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    getAtXYZ(0, 0, this->zpixels() - 1)->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
  }

  // Use cached box
  assemblyBox = *m_cachedBoundingBox;
}

/**
 * Draws the objcomponent, If the handler is not set then this function does
 * nothing.
 */
void GridDetector::draw() const {
  // std::cout << "GridDetector::draw() called for " << this->getName()
  // << "\n";
  if (Handle() == nullptr)
    return;
  // Render the ObjComponent and then render the object
  Handle()->render();
}

/**
 * Draws the Object
 */
void GridDetector::drawObject() const {
  // std::cout << "GridDetector::drawObject() called for " <<
  // this->getName() << "\n";
  // if(shape!=NULL)    shape->draw();
}

/**
 * Initializes the ObjComponent for rendering, this function should be called
 * before rendering.
 */
void GridDetector::initDraw() const {
  // std::cout << "GridDetector::initDraw() called for " <<
  // this->getName() << "\n";
  if (Handle() == nullptr)
    return;
  // Render the ObjComponent and then render the object
  // if(shape!=NULL)    shape->initDraw();
  Handle()->initialize();
}

//-------------------------------------------------------------------------------------------------
/// Returns the shape of the Object
const boost::shared_ptr<const IObject> GridDetector::shape() const {
  // --- Create a cuboid shape for your pixels ----
  double szX = m_xpixels;
  double szY = m_ypixels;
  double szZ = m_zpixels == 0 ? 0.5 : m_zpixels;
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
  boost::shared_ptr<Geometry::IObject> cuboidShape =
      shapeCreator.createShape(xmlCuboidShape);

  return cuboidShape;
}

const Kernel::Material GridDetector::material() const {
  return Kernel::Material();
}

size_t
GridDetector::registerContents(ComponentVisitor &componentVisitor) const {
  return componentVisitor.registerGridBank(*this);
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
 * @return stream representation of grid detector
 *
 *  Loops through all components in the assembly
 *  and call printSelf(os).
 *  Also output the number of children
 */
std::ostream &operator<<(std::ostream &os, const GridDetector &ass) {
  ass.printSelf(os);
  os << "************************\n";
  os << "Number of children :" << ass.nelements() << '\n';
  ass.printChildren(os);
  return os;
}

} // Namespace Geometry
} // Namespace Mantid
