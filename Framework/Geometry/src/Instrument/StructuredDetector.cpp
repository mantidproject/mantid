#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rendering/StructuredGeometryHandler.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include <algorithm>
#include <boost/regex.hpp>
#include <ostream>
#include <stdexcept>

namespace Mantid {
namespace Geometry {

using Kernel::V3D;
using Kernel::Quat;
using Kernel::Matrix;

/** Empty constructor
*/
StructuredDetector::StructuredDetector()
    : CompAssembly(), IObjComponent(nullptr), m_base(nullptr) {
  init();
}

/** Valued constructor
*  @param name :: name of the assembly
*  @param reference :: the parent Component
*
* 	If the reference is an object of class Component,
*  normal parenting apply. If the reference object is
*  an assembly itself, then in addition to parenting
*  this is registered as a children of reference.
*/
StructuredDetector::StructuredDetector(const std::string &name,
                                       IComponent *reference)
    : CompAssembly(name, reference), IObjComponent(nullptr), m_base(nullptr) {
  init();
  this->setName(name);
}

/** Constructor for a parametrized StructuredrDetector
* @param base: the base (un-parametrized) StructuredDetector
* @param map: pointer to the ParameterMap
* */
StructuredDetector::StructuredDetector(const StructuredDetector *base,
                                       const ParameterMap *map)
    : CompAssembly(base, map), IObjComponent(nullptr), m_base(base) {
  init();
}

bool StructuredDetector::compareName(const std::string &proposedMatch) {
  boost::regex exp(
      "(StructuredDetector)|(structuredDetector)|(structureddetector)|"
      "(structured_detector)");

  return boost::regex_match(proposedMatch, exp);
}

void StructuredDetector::init() {
  m_xpixels = 0;
  m_ypixels = 0;
  m_minDetId = 0;
  m_maxDetId = 0;
  m_idstart = 0;
  m_idfillbyfirst_y = false;
  m_idstepbyrow = 0;
  m_idstep = 0;

  setGeometryHandler(new StructuredGeometryHandler(this));
}

/** Clone method
*  Make a copy of the component assembly
*  @return new(*this)
*/
IComponent *StructuredDetector::clone() const {
  return new StructuredDetector(*this);
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
boost::shared_ptr<Detector> StructuredDetector::getAtXY(const int X,
                                                        const int Y) const {
  if ((xpixels() <= 0) || (ypixels() <= 0))
    throw std::runtime_error("StructuredDetector::getAtXY: invalid X or Y "
                             "width set in the object.");
  if ((X < 0) || (X >= xpixels()))
    throw std::runtime_error(
        "StructuredDetector::getAtXY: X specified is out of range.");
  if ((Y < 0) || (Y >= ypixels()))
    throw std::runtime_error(
        "StructuredDetector::getAtXY: Y specified is out of range.");

  // Get to column
  ICompAssembly_sptr xCol =
      boost::dynamic_pointer_cast<ICompAssembly>(this->getChild(X));
  if (!xCol)
    throw std::runtime_error(
        "StructuredDetector::getAtXY: X specified is out of range.");
  return boost::dynamic_pointer_cast<Detector>(xCol->getChild(Y));
}

/** Return the detector ID corresponding to the component in the assembly at the
* (X,Y) pixel position. No bounds check is made!
*
* @param X :: index from 0..m_xpixels-1
* @param Y :: index from 0..m_ypixels-1
* @return detector ID int
* @throw runtime_error if the x/y pixel width is not set, or X/Y are out of
*range
*/
detid_t StructuredDetector::getDetectorIDAtXY(const int X, const int Y) const {
  const StructuredDetector *me = m_map == nullptr ? this : this->m_base;

  if (me->m_idfillbyfirst_y)
    return me->m_idstart + X * me->m_idstepbyrow + Y * me->m_idstep;
  else
    return me->m_idstart + Y * me->m_idstepbyrow + X * me->m_idstep;
}

/** Given a detector ID, return the X,Y coords into the structured detector
*
* @param detectorID :: detectorID
* @return pair of (x,y)
*/
std::pair<int, int>
StructuredDetector::getXYForDetectorID(const int detectorID) const {
  const StructuredDetector *me = this;
  if (m_map)
    me = this->m_base;

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

/// Returns the number of pixels in the X direction.
/// @return number of X pixels
int StructuredDetector::xpixels() const {
  if (m_map)
    return m_base->m_xpixels;
  else
    return this->m_xpixels;
}

/** Sets the colours for detector IDs based on color maps created by instrument
actor
* @param r red color channel
* @param g green color channel
* @param b blue color channel
*/
void StructuredDetector::setColors(const std::vector<int> &r,
                                   const std::vector<int> &g,
                                   const std::vector<int> &b) const {
  if (m_map)
    m_base->setColors(r, g, b);
  else {
    this->r = r;
    this->g = g;
    this->b = b;
  }
}

/// Returns the red color channel for detector colors
/// @return red color channel
std::vector<int> const &StructuredDetector::getR() const {
  if (m_map)
    return m_base->getR();
  else
    return r;
}

/// Returns the green color channel for detector colors
/// @return green color channel
std::vector<int> const &StructuredDetector::getG() const {
  if (m_map)
    return m_base->getG();
  else
    return g;
}

/// Returns the blue color channel for detector colors
/// @return blue color channel
std::vector<int> const &StructuredDetector::getB() const {
  if (m_map)
    return m_base->getB();
  else
    return b;
}

/// Returns the number of pixels in the Y direction.
/// @return number of Y pixels
int StructuredDetector::ypixels() const {
  if (m_map)
    return m_base->m_ypixels;
  else
    return this->m_ypixels;
}

/// Returns the idstart
int StructuredDetector::idstart() const {
  if (m_map)
    return m_base->m_idstart;
  else
    return this->m_idstart;
}

/// Returns the idfillbyfirst_y
bool StructuredDetector::idfillbyfirst_y() const {
  if (m_map)
    return m_base->m_idfillbyfirst_y;
  else
    return this->m_idfillbyfirst_y;
}

/// Returns the idstepbyrow
int StructuredDetector::idstepbyrow() const {
  if (m_map)
    return m_base->m_idstepbyrow;
  else
    return this->m_idstepbyrow;
}

/// Returns the idstep
int StructuredDetector::idstep() const {
  if (m_map)
    return m_base->m_idstep;
  else
    return this->m_idstep;
}

std::vector<double> const &StructuredDetector::getXValues() const {
  if (m_map)
    return m_base->getXValues();
  else
    return this->m_xvalues;
}

std::vector<double> const &StructuredDetector::getYValues() const {
  if (m_map)
    return m_base->getYValues();
  else
    return this->m_yvalues;
}

/** Initialize a StructuredDetector by creating all of the pixels
* contained within it. You should have set the name, position
* and rotation and facing of this object BEFORE calling this.
* NB xpixels and ypixels requires (xpixels+1)*(ypixels+1) vertices
*
* @param xpixels :: number of pixels in X
* @param ypixels :: number of pixels in Y
* @param x :: X vertices
* @param y :: Y vertices
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
void StructuredDetector::initialize(int xpixels, int ypixels,
                                    const std::vector<double> &x,
                                    const std::vector<double> &y, int idstart,
                                    bool idfillbyfirst_y, int idstepbyrow,
                                    int idstep) {
  if (m_map)
    throw std::runtime_error("StructuredDetector::initialize() called for a "
                             "parametrized StructuredDetector");

  m_xpixels = xpixels;
  m_ypixels = ypixels;

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
        "StructuredDetector::initialize(): xpixels should be > 0");
  if (m_ypixels <= 0)
    throw std::invalid_argument(
        "StructuredDetector::initialize(): ypixels should be > 0");
  if (x.size() != y.size())
    throw std::invalid_argument(
        "StructuredDetector::initialize(): x.size() should be = y.size()");
  if (x.size() != (size_t)((m_xpixels + 1) * (m_ypixels + 1)))
    throw std::invalid_argument("StructuredDetector::initialize(): x.size() "
                                "should be = (xpixels+1)*(ypixels+1)");

  // Store vertices
  m_xvalues = x;
  m_yvalues = y;

  createDetectors();
}

/** Creates all detector pixels within the StructuredDetector.
*/
void StructuredDetector::createDetectors() {
  auto minDetId = m_idstart;
  auto maxDetId = m_idstart;

  for (auto ix = 0; ix < m_xpixels; ix++) {
    // Create an ICompAssembly for each x-column
    std::ostringstream oss_col;

    oss_col << this->getName() << "(x=" << ix << ")";
    CompAssembly *xColumn = new CompAssembly(oss_col.str(), this);

    for (auto iy = 0; iy < m_ypixels; iy++) {
      std::ostringstream oss;
      oss << this->getName() << "(" << ix << "," << iy << ")";

      // Calculate its id and set it.
      auto id = this->getDetectorIDAtXY(ix, iy);

      // Store min and max det IDs
      // minimum structured detector id
      if (id < minDetId) {
        minDetId = id;
      }
      // maximum structured detector id
      if (id > maxDetId) {
        maxDetId = id;
      }

      // Create and store detector pixel
      xColumn->add(addDetector(xColumn, oss.str(), ix, iy, id));
    }
  }

  m_minDetId = minDetId;
  m_maxDetId = maxDetId;
}

boost::shared_ptr<Mantid::Geometry::Object>
streamShape(const std::string &name, double xlb, double xlf, double xrf,
            double xrb, double ylb, double ylf, double yrf, double yrb) {

  std::ostringstream shapestr;

  // Create XML shape used to describe detector pixel
  shapestr << "<type name=\"userShape\" >";
  shapestr << "<hexahedron id=\"" << name << "\" >";
  shapestr << "<left-back-bottom-point x=\"" << xlb << "\""
           << " y=\"" << ylb << "\""
           << " z=\"0\" />";
  shapestr << "<left-front-bottom-point x=\"" << xlf << "\""
           << " y=\"" << ylf << "\""
           << " z=\"0\" />";
  shapestr << "<right-front-bottom-point x=\"" << xrf << "\""
           << " y=\"" << yrf << "\""
           << " z=\"0\" />";
  shapestr << "<right-back-bottom-point x=\"" << xrb << "\""
           << " y=\"" << yrb << "\""
           << " z=\"0\" />";
  shapestr << "<left-back-top-point x=\"" << xlb << "\""
           << " y=\"" << ylb << "\""
           << " z=\"0.001\" />";
  shapestr << "<left-front-top-point x=\"" << xlf << "\""
           << " y=\"" << ylf << "\""
           << " z=\"0.001\" />";
  shapestr << "<right-front-top-point x=\"" << xrf << "\""
           << " y=\"" << yrf << "\""
           << " z=\"0.001\" />";
  shapestr << "<right-back-top-point x=\"" << xrb << "\""
           << " y=\"" << yrb << "\""
           << " z=\"0.001\" />";
  shapestr << "</hexahedron>";
  shapestr << "<bounding-box>";
  shapestr << "<x-min val=\"" << std::min(xlf, xlb) << "\" />";
  shapestr << "<x-max val=\"" << std::max(xrb, xrf) << "\" />";
  shapestr << "<y-min val=\"" << ylb << "\" />";
  shapestr << "<y-max val=\"" << yrf << "\" />";
  shapestr << "<z-min val=\"0\" />";
  shapestr << "<z-max val=\"0.001\" />";
  shapestr << "</bounding-box>";
  shapestr << "<algebra val=\"" << name << "\" />";
  shapestr << "</type>";

  Mantid::Geometry::ShapeFactory shapeCreator;

  return shapeCreator.createShape(shapestr.str(), false);
}

/** Creates new hexahedral detector pixel at row x column y using the
*   detector vertex values.
* @param parent :: The parent component assembly
* @param name :: The pixel name identifier
* @param x :: The pixel row
* @param y :: The pixel column
* @param id :: The pixel ID
* @return newly created detector.
*/
Detector *StructuredDetector::addDetector(CompAssembly *parent,
                                          const std::string &name, int x, int y,
                                          int id) {
  auto w = m_xpixels + 1;

  // Store hexahedral vertices for detector shape
  auto xlb = m_xvalues[(y * w) + x];
  auto xlf = m_xvalues[(y * w) + x + w];
  auto xrf = m_xvalues[(y * w) + x + w + 1];
  auto xrb = m_xvalues[(y * w) + x + 1];
  auto ylb = m_yvalues[(y * w) + x];
  auto ylf = m_yvalues[(y * w) + x + w];
  auto yrf = m_yvalues[(y * w) + x + w + 1];
  auto yrb = m_yvalues[(y * w) + x + 1];

  // calculate midpoint of trapeziod
  double a = std::abs((double)(xrf - xlf));
  double b = std::abs((double)(xrb - xlb));
  double h = std::abs((double)(ylb - ylf));
  auto cx = ((a + b) / 4);
  auto cy = h / 2;

  // store detector position before translating to origin
  auto xpos = xlb + cx;
  auto ypos = ylb + cy;

  // Translate detector shape to origin
  xlf -= xpos;
  xrf -= xpos;
  xrb -= xpos;
  xlb -= xpos;
  ylf -= ypos;
  yrf -= ypos;
  yrb -= ypos;
  ylb -= ypos;

  boost::shared_ptr<Mantid::Geometry::Object> shape =
      streamShape(name, xlb, xlf, xrf, xrb, ylb, ylf, yrf, yrb);

  // Create detector
  auto detector = new Detector(name, id, shape, parent);

  // Set detector position relative to parent
  V3D pos(xpos, ypos, 0);

  detector->translate(pos);

  return detector;
}

/** Returns the minimum detector id
* @return minimum detector id
*/
int StructuredDetector::minDetectorID() {
  if (m_map)
    return m_base->m_minDetId;
  return m_minDetId;
}

//-------------------------------------------------------------------------------------------------
/** Returns the maximum detector id
* @return maximum detector id
*/
int StructuredDetector::maxDetectorID() {
  if (m_map)
    return m_base->m_maxDetId;
  return m_maxDetId;
}

boost::shared_ptr<const IComponent>
StructuredDetector::getComponentByName(const std::string &cname,
                                       int nlevels) const {
  // exact matches
  if (cname == this->getName())
    return boost::shared_ptr<const IComponent>(this);

  // cache the detector's name as all the other names are longer
  // The extra ( is because all children of this have that as the next character
  // and this prevents Bank11 matching Bank 1
  const std::string MEMBER_NAME = this->getName() + "(";

  // check that the searched for name starts with the detector's
  // name as they are generated
  if (cname.substr(0, MEMBER_NAME.length()).compare(MEMBER_NAME) != 0) {
    return boost::shared_ptr<const IComponent>();
  } else {
    return CompAssembly::getComponentByName(cname, nlevels);
  }
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// ------------ IObjComponent methods ----------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
/// Does the point given lie within this object component?
bool StructuredDetector::isValid(const V3D &) const {
  throw Kernel::Exception::NotImplementedError(
      "StructuredDetector::isValid() is not implemented.");
}

/// Does the point given lie on the surface of this object component?
bool StructuredDetector::isOnSide(const V3D &) const {
  throw Kernel::Exception::NotImplementedError(
      "StructuredDetector::isOnSide() is not implemented.");
}

/// Checks whether the track given will pass through this Component.
int StructuredDetector::interceptSurface(Track &) const {
  throw Kernel::Exception::NotImplementedError(
      "StructuredDetector::interceptSurface() is not implemented.");
}

/// Finds the approximate solid angle covered by the component when viewed from
/// the point given
double StructuredDetector::solidAngle(const V3D &) const {
  throw Kernel::Exception::NotImplementedError(
      "StructuredDetector::solidAngle() is not implemented.");
}

/// Try to find a point that lies within (or on) the object
int StructuredDetector::getPointInObject(V3D &) const {
  throw Kernel::Exception::NotImplementedError(
      "StructuredDetector::getPointInObject() is not implemented.");
}

/**
* Get the bounding box and store it in the given object. This is cached after
* the first call.
* @param assemblyBox :: A BoundingBox object that will be overwritten
*/
void StructuredDetector::getBoundingBox(BoundingBox &assemblyBox) const {
  if (!m_cachedBoundingBox) {
    m_cachedBoundingBox = new BoundingBox();
    // Get all the corners
    BoundingBox compBox;

    boost::shared_ptr<Detector> det = getAtXY(0, 0);
    det->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    det = getAtXY(this->xpixels() - 1, 0);
    det->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    det = getAtXY(this->xpixels() - 1, this->ypixels() - 1);
    det->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    det = getAtXY(0, this->ypixels() - 1);
    det->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
  }
  // Use cached box
  assemblyBox = *m_cachedBoundingBox;
}

/**
* Draws the objcomponent, If the handler is not set then this function does
* nothing.
*/
void StructuredDetector::draw() const {
  if (Handle() == nullptr)
    return;
  // Render the ObjComponent and then render the object
  Handle()->Render();
}

/**
* Draws the Object
*/
void StructuredDetector::drawObject() const {}

/**
* Initializes the ObjComponent for rendering, this function should be called
* before rendering.
*/
void StructuredDetector::initDraw() const {
  if (Handle() == nullptr)
    return;

  Handle()->Initialize();
}

/// Returns the shape of the Object
const boost::shared_ptr<const Object> StructuredDetector::shape() const {
  // --- Create a hexahedral shape for your pixels ----
  auto w = this->xpixels() + 1;
  auto xlb = m_xvalues[0];
  auto xlf = m_xvalues[w * m_ypixels];
  auto xrf = m_xvalues[(w * m_ypixels) + m_xpixels];
  auto xrb = m_xvalues[w];
  auto ylb = m_yvalues[0];
  auto ylf = m_yvalues[(w * m_ypixels)];
  auto yrf = m_yvalues[(w * m_ypixels) + m_xpixels];
  auto yrb = m_yvalues[w];

  std::ostringstream xmlShapeStream;

  xmlShapeStream << "<hexahedron id=\"detector-shape\" >"
                 << "<left-back-bottom-point x=\"" << xlb << "\""
                 << " y=\"" << ylb << "\""
                 << " z=\"0\" />"
                 << "<left-front-bottom-point x=\"" << xlf << "\""
                 << " y=\"" << ylf << "\""
                 << " z=\"0\" />"
                 << "<right-front-bottom-point x=\"" << xrf << "\""
                 << " y=\"" << yrf << "\""
                 << " z=\"0\" />"
                 << "<right-back-bottom-point x=\"" << xrb << "\""
                 << " y=\"" << yrb << "\""
                 << " z=\"0\" />"
                 << "<left-back-top-point x=\"" << xlb << "\""
                 << " y=\"" << ylb << "\""
                 << " z=\"0.5\" />"
                 << "<left-front-top-point x=\"" << xlf << "\""
                 << " y=\"" << ylf << "\""
                 << " z=\"0.5\" />"
                 << "<right-front-top-point x=\"" << xrf << "\""
                 << " y=\"" << yrf << "\""
                 << " z=\"0.5\" />"
                 << "<right-back-top-point x=\"" << xrb << "\""
                 << " y=\"" << yrb << "\""
                 << " z=\"0.5\" />"
                 << "</hexahedron>";

  std::string xmlHexahedralShape(xmlShapeStream.str());
  Geometry::ShapeFactory shapeCreator;

  return shapeCreator.createShape(xmlHexahedralShape);
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
* @return stream representation of structured detector
*
*  Loops through all components in the assembly
*  and call printSelf(os).
*  Also output the number of children
*/
std::ostream &operator<<(std::ostream &os, const StructuredDetector &ass) {
  ass.printSelf(os);
  os << "************************" << std::endl;
  os << "Number of children :" << ass.nelements() << std::endl;
  ass.printChildren(os);
  return os;
}

} // namespace Geometry
} // namespace Mantid
