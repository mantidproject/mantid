// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Matrix.h"
#include <algorithm>
#include <boost/regex.hpp>
#include <memory>
#include <ostream>
#include <stdexcept>

namespace Mantid::Geometry {

using Kernel::V3D;

/** Empty constructor
 */
StructuredDetector::StructuredDetector() : CompAssembly(), IObjComponent(nullptr), m_base(nullptr) { init(); }

/** Valued constructor
 *  @param name :: name of the assembly
 *  @param reference :: the parent Component
 *
 * 	If the reference is an object of class Component,
 *  normal parenting apply. If the reference object is
 *  an assembly itself, then in addition to parenting
 *  this is registered as a children of reference.
 */
StructuredDetector::StructuredDetector(const std::string &name, IComponent *reference)
    : CompAssembly(name, reference), IObjComponent(nullptr), m_base(nullptr) {
  init();
  this->setName(name);
}

/** Constructor for a parametrized StructuredrDetector
 * @param base: the base (un-parametrized) StructuredDetector
 * @param map: pointer to the ParameterMap
 * */
StructuredDetector::StructuredDetector(const StructuredDetector *base, const ParameterMap *map)
    : CompAssembly(base, map), IObjComponent(nullptr), m_base(base) {
  init();
}

bool StructuredDetector::compareName(const std::string &proposedMatch) {
  static const boost::regex exp("StructuredDetector|structuredDetector|"
                                "structureddetector|structured_detector");

  return boost::regex_match(proposedMatch, exp);
}

void StructuredDetector::init() {
  m_xPixels = 0;
  m_yPixels = 0;
  m_minDetId = 0;
  m_maxDetId = 0;
  m_idStart = 0;
  m_idFillByFirstY = false;
  m_idStepByRow = 0;
  m_idStep = 0;

  setGeometryHandler(new GeometryHandler(this));
}

/** Clone method
 *  Make a copy of the component assembly
 *  @return new(*this)
 */
IComponent *StructuredDetector::clone() const { return new StructuredDetector(*this); }

//-------------------------------------------------------------------------------------------------
/** Return a pointer to the component in the assembly at the
 * (X,Y) pixel position.
 *
 * @param x :: index from 0..m_xPixels-1
 * @param y :: index from 0..m_yPixels-1
 * @return a pointer to the component in the assembly at the (X,Y) pixel
 *position
 * @throw runtime_error if the x/y pixel width is not set, or X/Y are out of
 *range
 */
std::shared_ptr<Detector> StructuredDetector::getAtXY(const size_t x, const size_t y) const {
  if (x >= xPixels())
    throw std::runtime_error("StructuredDetector::getAtXY: X specified is out of range.");
  if (y >= yPixels())
    throw std::runtime_error("StructuredDetector::getAtXY: Y specified is out of range.");

  // Get to column
  ICompAssembly_sptr xCol = std::dynamic_pointer_cast<ICompAssembly>(this->getChild(static_cast<int>(x)));
  if (!xCol)
    throw std::runtime_error("StructuredDetector::getAtXY: X specified is out of range.");
  return std::dynamic_pointer_cast<Detector>(xCol->getChild(static_cast<int>(y)));
}

/** Return the detector ID corresponding to the component in the assembly at the
 * (X,Y) pixel position. No bounds check is made!
 *
 * @param x :: index from 0..m_xPixels-1
 * @param y :: index from 0..m_yPixels-1
 * @return detector ID int
 * @throw runtime_error if the x/y pixel width is not set, or X/Y are out of
 *range
 */
detid_t StructuredDetector::getDetectorIDAtXY(const size_t x, const size_t y) const {
  const StructuredDetector *me = m_map == nullptr ? this : this->m_base;

  if (me->m_idFillByFirstY)
    return static_cast<detid_t>(me->m_idStart + x * me->m_idStepByRow + y * me->m_idStep);
  else
    return static_cast<detid_t>(me->m_idStart + y * me->m_idStepByRow + x * me->m_idStep);
}

/** Given a detector ID, return the X,Y coords into the structured detector
 *
 * @param detectorID :: detectorID
 * @return pair of (x,y)
 */
std::pair<size_t, size_t> StructuredDetector::getXYForDetectorID(const detid_t detectorID) const {
  const StructuredDetector *me = this;
  if (m_map)
    me = this->m_base;

  int id = detectorID - me->m_idStart;

  if ((me->m_idStepByRow == 0) || (me->m_idStep == 0))
    return std::pair<int, int>(-1, -1);

  int row = id / me->m_idStepByRow;
  int col = (id % me->m_idStepByRow) / me->m_idStep;

  if (me->m_idFillByFirstY) // x is the fast-changing axis
    return std::pair<int, int>(row, col);
  else
    return std::pair<int, int>(col, row);
}

/// Returns the number of pixels in the X direction.
/// @return number of X pixels
size_t StructuredDetector::xPixels() const {
  if (m_map)
    return m_base->m_xPixels;
  else
    return this->m_xPixels;
}

/** Sets the colours for detector IDs based on color maps created by instrument
actor
* @param r red color channel
* @param g green color channel
* @param b blue color channel
*/
void StructuredDetector::setColors(const std::vector<int> &r, const std::vector<int> &g,
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
size_t StructuredDetector::yPixels() const {
  if (m_map)
    return m_base->m_yPixels;
  else
    return this->m_yPixels;
}

/// Returns the idStart
detid_t StructuredDetector::idStart() const {
  if (m_map)
    return m_base->m_idStart;
  else
    return this->m_idStart;
}

/// Returns the idFillByFirstY
bool StructuredDetector::idFillByFirstY() const {
  if (m_map)
    return m_base->m_idFillByFirstY;
  else
    return this->m_idFillByFirstY;
}

/// Returns the idStepByRow
int StructuredDetector::idStepByRow() const {
  if (m_map)
    return m_base->m_idStepByRow;
  else
    return this->m_idStepByRow;
}

/// Returns the idStep
int StructuredDetector::idStep() const {
  if (m_map)
    return m_base->m_idStep;
  else
    return this->m_idStep;
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
 * NB xPixels and yPixels requires (xPixels+1)*(yPixels+1) vertices
 *
 * @param xPixels :: number of pixels in X
 * @param yPixels :: number of pixels in Y
 * @param x :: X vertices
 * @param y :: Y vertices
 * @param isZBeam :: Whether or not the alongBeam axis is z
 * @param idStart :: detector ID of the first pixel
 * @param idFillByFirstY :: set to true if ID numbers increase with Y indices
 *first. That is: (0,0)=0; (0,1)=1, (0,2)=2 and so on.
 * @param idStepByRow :: amount to increase the ID number on each row. e.g, if
 *you fill by Y first,
 *            and set  idStepByRow = 100, and have 50 Y pixels, you would get:
 *            (0,0)=0; (0,1)=1; ... (0,49)=49; (1,0)=100; (1,1)=101; etc.
 * @param idStep :: amount to increase each individual ID number with a row.
 *e.g, if you fill by Y first,
 *            and idStep=100 and idStart=1 then (0,0)=1; (0,1)=101; and so on
 *
 */
void StructuredDetector::initialize(size_t xPixels, size_t yPixels, std::vector<double> &&x, std::vector<double> &&y,
                                    bool isZBeam, detid_t idStart, bool idFillByFirstY, int idStepByRow, int idStep) {
  if (m_map)
    throw std::runtime_error("StructuredDetector::initialize() called for a "
                             "parametrized StructuredDetector");

  m_xPixels = xPixels;
  m_yPixels = yPixels;

  /// IDs start here
  m_idStart = idStart;
  /// IDs are filled in Y fastest
  m_idFillByFirstY = idFillByFirstY;
  /// Step size in ID in each row
  m_idStepByRow = idStepByRow;
  /// Step size in ID in each col
  m_idStep = idStep;

  // Some safety checks
  if (m_xPixels == 0)
    throw std::invalid_argument("StructuredDetector::initialize(): xPixels should be > 0");
  if (m_yPixels == 0)
    throw std::invalid_argument("StructuredDetector::initialize(): yPixels should be > 0");
  if (x.size() != y.size())
    throw std::invalid_argument("StructuredDetector::initialize(): x.size() should be = y.size()");
  if (x.size() != (size_t)((m_xPixels + 1) * (m_yPixels + 1)))
    throw std::invalid_argument("StructuredDetector::initialize(): x.size() "
                                "should be = (xPixels+1)*(yPixels+1)");
  if (!isZBeam) // StructuredDetector only allows z-axis aligned beams.
    throw std::invalid_argument("Expecting reference_frame to provide z as "
                                "beam axis. StructuredDetecor only allows "
                                "z-axis aligned beams.");

  // Store vertices
  m_xvalues = std::move(x);
  m_yvalues = std::move(y);

  createDetectors();
}

/** Creates all detector pixels within the StructuredDetector.
 */
void StructuredDetector::createDetectors() {
  auto minDetId = m_idStart;
  auto maxDetId = m_idStart;

  for (size_t ix = 0; ix < m_xPixels; ix++) {
    // Create an ICompAssembly for each x-column
    std::ostringstream oss_col;

    oss_col << this->getName() << "(x=" << ix << ")";
    CompAssembly *xColumn = new CompAssembly(oss_col.str(), this);

    for (size_t iy = 0; iy < m_yPixels; iy++) {
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

/** Creates new hexahedral detector pixel at row x column y using the
 *   detector vertex values.
 * @param parent :: The parent component assembly
 * @param name :: The pixel name identifier
 * @param x :: The pixel row
 * @param y :: The pixel column
 * @param id :: The pixel ID
 * @return newly created detector.
 */
Detector *StructuredDetector::addDetector(CompAssembly *parent, const std::string &name, size_t x, size_t y,
                                          detid_t id) {
  auto w = m_xPixels + 1;

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
  auto xpos = (xlb + xlf + xrf + xrb) / 4;
  auto ypos = (ylb + ylf + yrf + yrb) / 4;

  // Translate detector shape to origin
  xlf -= xpos;
  xrf -= xpos;
  xrb -= xpos;
  xlb -= xpos;
  ylf -= ypos;
  yrf -= ypos;
  yrb -= ypos;
  ylb -= ypos;

  std::shared_ptr<Mantid::Geometry::IObject> detectorShape =
      ShapeFactory{}.createHexahedralShape(xlb, xlf, xrf, xrb, ylb, ylf, yrf, yrb);

  // Create detector
  auto detector = new Detector(name, id, detectorShape, parent);

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

std::shared_ptr<const IComponent> StructuredDetector::getComponentByName(const std::string &cname, int nlevels) const {
  // exact matches
  if (cname == this->getName())
    return std::shared_ptr<const IComponent>(this);

  // cache the detector's name as all the other names are longer
  // The extra ( is because all children of this have that as the next character
  // and this prevents Bank11 matching Bank 1
  const std::string MEMBER_NAME = this->getName() + "(";

  // check that the searched for name starts with the detector's
  // name as they are generated
  if (cname.substr(0, MEMBER_NAME.length()) != MEMBER_NAME) {
    return std::shared_ptr<const IComponent>();
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
bool StructuredDetector::isValid(const V3D & /*point*/) const {
  throw Kernel::Exception::NotImplementedError("StructuredDetector::isValid() is not implemented.");
}

/// Does the point given lie on the surface of this object component?
bool StructuredDetector::isOnSide(const V3D & /*point*/) const {
  throw Kernel::Exception::NotImplementedError("StructuredDetector::isOnSide() is not implemented.");
}

/// Checks whether the track given will pass through this Component.
int StructuredDetector::interceptSurface(Track & /*track*/) const {
  throw Kernel::Exception::NotImplementedError("StructuredDetector::interceptSurface() is not implemented.");
}

/// Finds the approximate solid angle covered by the component when viewed from
/// the point given
double StructuredDetector::solidAngle(const Geometry::SolidAngleParams & /*params*/) const {
  throw Kernel::Exception::NotImplementedError("StructuredDetector::solidAngle() is not implemented.");
}

/// Try to find a point that lies within (or on) the object
int StructuredDetector::getPointInObject(V3D & /*point*/) const {
  throw Kernel::Exception::NotImplementedError("StructuredDetector::getPointInObject() is not implemented.");
}

/**
 * Get the bounding box and store it in the given object. This is cached after
 * the first call.
 * @param assemblyBox :: A BoundingBox object that will be overwritten
 */
void StructuredDetector::getBoundingBox(BoundingBox &assemblyBox) const {
  if (hasComponentInfo()) {
    assemblyBox = m_map->componentInfo().boundingBox(index(), &assemblyBox);
    return;
  }
  if (!m_cachedBoundingBox) {
    m_cachedBoundingBox = new BoundingBox();
    // Get all the corners
    BoundingBox compBox;

    std::shared_ptr<Detector> det = getAtXY(0, 0);
    det->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    det = getAtXY(this->xPixels() - 1, 0);
    det->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    det = getAtXY(this->xPixels() - 1, this->yPixels() - 1);
    det->getBoundingBox(compBox);
    m_cachedBoundingBox->grow(compBox);
    det = getAtXY(0, this->yPixels() - 1);
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
  Handle()->render();
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

  Handle()->initialize();
}

/// Returns the shape of the Object
const std::shared_ptr<const IObject> StructuredDetector::shape() const {
  // --- Create a hexahedral shape for your pixels ----
  auto w = this->xPixels() + 1;
  auto xlb = m_xvalues[0];
  auto xlf = m_xvalues[w * m_yPixels];
  auto xrf = m_xvalues[(w * m_yPixels) + m_xPixels];
  auto xrb = m_xvalues[w];
  auto ylb = m_yvalues[0];
  auto ylf = m_yvalues[(w * m_yPixels)];
  auto yrf = m_yvalues[(w * m_yPixels) + m_xPixels];
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

const Kernel::Material StructuredDetector::material() const { return Kernel::Material(); }

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
  os << "************************\n";
  os << "Number of children :" << ass.nelements() << '\n';
  ass.printChildren(os);
  return os;
}

size_t StructuredDetector::registerContents(class ComponentVisitor &componentVisitor) const {
  return componentVisitor.registerStructuredBank(*this);
}

} // namespace Mantid::Geometry
