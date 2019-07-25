// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
#include "MantidGeometry/Instrument/Detector.h"
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

namespace {
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
} // namespace

namespace Mantid {
namespace Geometry {

using Kernel::Matrix;
using Kernel::V3D;

/** Constructor for a parametrized RectangularDetector
 * @param base: the base (un-parametrized) RectangularDetector
 * @param map: pointer to the ParameterMap
 * */
RectangularDetector::RectangularDetector(const RectangularDetector *base,
                                         const ParameterMap *map)
    : GridDetector(base, map) {
  init();
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
    : GridDetector(n, reference) {
  init();
  m_textureID = 0;
  this->setName(n);
  setGeometryHandler(new GeometryHandler(this));
}

bool RectangularDetector::compareName(const std::string &proposedMatch) {
  static const boost::regex exp("RectangularDetector|rectangularDetector|"
                                "rectangulardetector|rectangular_detector");

  return boost::regex_match(proposedMatch, exp);
}

/** Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
RectangularDetector *RectangularDetector::clone() const {
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
  return GridDetector::getAtXYZ(X, Y, 0);
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
  return GridDetector::getDetectorIDAtXYZ(X, Y, 0);
}

//-------------------------------------------------------------------------------------------------
/** Given a detector ID, return the X,Y coords into the rectangular detector
 *
 * @param detectorID :: detectorID
 * @return pair of (x,y)
 */
std::pair<int, int>
RectangularDetector::getXYForDetectorID(const int detectorID) const {
  auto xyz = GridDetector::getXYZForDetectorID(detectorID);
  return std::pair<int, int>(std::get<0>(xyz), std::get<1>(xyz));
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
  return GridDetector::getRelativePosAtXYZ(x, y, 0);
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
void RectangularDetector::initialize(boost::shared_ptr<IObject> shape,
                                     int xpixels, double xstart, double xstep,
                                     int ypixels, double ystart, double ystep,
                                     int idstart, bool idfillbyfirst_y,
                                     int idstepbyrow, int idstep) {

  GridDetector::initialize(
      shape, xpixels, xstart, xstep, ypixels, ystart, ystep, 0, 0, 0, idstart,
      idfillbyfirst_y ? "yxz" : "xyz", idstepbyrow, idstep);
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
  auto xIndex = int(u);
  auto yIndex = int(v);

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
  m_textureID = textureID;
}

/** Return the texture ID to be used in plotting . */
unsigned int RectangularDetector::getTextureID() const { return m_textureID; }

const Kernel::Material RectangularDetector::material() const {
  return Kernel::Material();
}

size_t RectangularDetector::registerContents(
    ComponentVisitor &componentVisitor) const {
  return componentVisitor.registerRectangularBank(*this);
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
  os << "************************\n";
  os << "Number of children :" << ass.nelements() << '\n';
  ass.printChildren(os);
  return os;
}

} // Namespace Geometry
} // Namespace Mantid
