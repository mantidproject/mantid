// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/PixelAssembly.h"
#include "MantidGeometry/IComponent.h"
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
#include <boost/regex.hpp>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace Mantid::Geometry {

using Kernel::V3D;

namespace {

bool checkValidOrderString(const std::string &order) {
  static const boost::regex exp("xyz|xzy|yzx|yxz|zyx|zxy");
  return boost::regex_match(order, exp);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructors / clone
// ---------------------------------------------------------------------------

/** Valued constructor.
 *  @param name       Name of the assembly.
 *  @param reference  Optional parent component.
 */
PixelAssembly::PixelAssembly(const std::string &name, IComponent *reference)
    : Component(name, reference), IObjComponent(nullptr), m_gridBase(nullptr) {
  init();
  setGeometryHandler(new GeometryHandler(this));
}

/** Parametrized constructor — wraps an existing PixelAssembly with a ParameterMap.
 *  @param base  The base (un-parametrized) PixelAssembly.
 *  @param map   Pointer to the ParameterMap.
 */
PixelAssembly::PixelAssembly(const PixelAssembly *base, const ParameterMap *map)
    : Component(base, map), IObjComponent(nullptr), m_gridBase(base) {
  init();
  setGeometryHandler(new GeometryHandler(this));
}

/** Returns true when the proposed name matches the PixelAssembly naming convention. */
bool PixelAssembly::compareName(const std::string &proposedMatch) {
  static const boost::regex exp("pixel_?assembly", boost::regex::icase);
  return boost::regex_match(proposedMatch, exp);
}

void PixelAssembly::init() {
  m_xpixels = m_ypixels = m_zpixels = 0;
  m_xstart = m_ystart = m_zstart = 0.0;
  m_xstep = m_ystep = m_zstep = 0.0;
  m_idstart = 0;
  m_idFillOrder = {'x', 'y', 'z'};
  m_idstep = 0;
}

PixelAssembly *PixelAssembly::clone() const { return new PixelAssembly(*this); }

// ---------------------------------------------------------------------------
// IVirtualBank: pixel-count accessors
// ---------------------------------------------------------------------------

size_t PixelAssembly::xpixels() const { return isParametrized() ? m_gridBase->m_xpixels : m_xpixels; }
size_t PixelAssembly::ypixels() const { return isParametrized() ? m_gridBase->m_ypixels : m_ypixels; }
size_t PixelAssembly::zpixels() const { return isParametrized() ? m_gridBase->m_zpixels : m_zpixels; }

// ---------------------------------------------------------------------------
// IVirtualBank: step-size accessors (scale-factor aware)
// ---------------------------------------------------------------------------

double PixelAssembly::xstep() const {
  if (isParametrized()) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scalex"))
      scaling = m_map->get(m_gridBase, "scalex")->value<double>();
    return m_gridBase->m_xstep * scaling;
  }
  return m_xstep;
}

double PixelAssembly::ystep() const {
  if (isParametrized()) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scaley"))
      scaling = m_map->get(m_gridBase, "scaley")->value<double>();
    return m_gridBase->m_ystep * scaling;
  }
  return m_ystep;
}

double PixelAssembly::zstep() const {
  if (isParametrized()) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scalez"))
      scaling = m_map->get(m_gridBase, "scalez")->value<double>();
    return m_gridBase->m_zstep * scaling;
  }
  return m_zstep;
}

// ---------------------------------------------------------------------------
// IVirtualBank: start-position accessors (scale-factor aware)
// ---------------------------------------------------------------------------

double PixelAssembly::xstart() const {
  if (isParametrized()) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scalex"))
      scaling = m_map->get(m_gridBase, "scalex")->value<double>();
    return m_gridBase->m_xstart * scaling;
  }
  return m_xstart;
}

double PixelAssembly::ystart() const {
  if (isParametrized()) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scaley"))
      scaling = m_map->get(m_gridBase, "scaley")->value<double>();
    return m_gridBase->m_ystart * scaling;
  }
  return m_ystart;
}

double PixelAssembly::zstart() const {
  if (isParametrized()) {
    double scaling = 1.0;
    if (m_map->contains(m_gridBase, "scalez"))
      scaling = m_map->get(m_gridBase, "scalez")->value<double>();
    return m_gridBase->m_zstart * scaling;
  }
  return m_zstart;
}

// ---------------------------------------------------------------------------
// IVirtualBank: ID-scheme accessors
// ---------------------------------------------------------------------------

detid_t PixelAssembly::referentDetectorID() const { return isParametrized() ? m_gridBase->m_idstart : m_idstart; }

std::array<char, 3> PixelAssembly::idFillOrder() const {
  return isParametrized() ? m_gridBase->m_idFillOrder : m_idFillOrder;
}

int PixelAssembly::idstep() const { return isParametrized() ? m_gridBase->m_idstep : m_idstep; }

// ---------------------------------------------------------------------------
// IVirtualBank: referent detector
// ---------------------------------------------------------------------------

/** Constructs and returns a Detector for the referent pixel (0,0,0) on demand.
 *  The returned object is independent (no parent link) and is positioned at
 *  the pixel's absolute world position.
 */
std::shared_ptr<Detector> PixelAssembly::referentDetector() const {
  std::shared_ptr<IObject> const &pxShape = isParametrized() ? m_gridBase->m_shape : m_shape;
  if (!pxShape)
    return nullptr;
  std::string const name = this->getName() + "(0,0,0)";
  auto det = std::make_shared<Detector>(name, static_cast<int>(referentDetectorID()), pxShape, nullptr);
  det->setPos(getPosAtXYZ(0, 0, 0));
  det->setRot(this->getRotation());
  return det;
}

// ---------------------------------------------------------------------------
// Position helpers
// ---------------------------------------------------------------------------

/** Absolute position of pixel (x, y, z) in the instrument frame.
 *
 *  NOTE: consistent with GridDetector::getPosAtXYZ — the relative position is
 *  added directly to the bank position without rotating by the bank orientation.
 *  This matches the existing behavior for unrotated banks.
 */
V3D PixelAssembly::getPosAtXYZ(int x, int y, int z) const { return this->getPos() + getRelativePosAtXYZ(x, y, z); }

// ---------------------------------------------------------------------------
// Per-pixel bounding box
// ---------------------------------------------------------------------------

/** Compute the axis-aligned world-space bounding box for pixel (x, y, z).
 *
 *  Replicates the ObjComponent::getBoundingBox pattern:
 *    1. Start from the pixel shape's local bounding box.
 *    2. Rotate by the bank's orientation.
 *    3. Translate to the pixel's absolute position.
 */
void PixelAssembly::getBoundingBoxAtXYZ(int const x, int const y, int const z, BoundingBox &box) const {
  std::shared_ptr<IObject> const &pxShape = isParametrized() ? m_gridBase->m_shape : m_shape;
  if (!pxShape || !pxShape->hasValidShape()) {
    box = BoundingBox();
    return;
  }
  BoundingBox const &shapeBox = pxShape->getBoundingBox();
  if (shapeBox.isNull()) {
    box = BoundingBox();
    return;
  }
  // 1. Copy the pixel shape's local bounding box.
  box = BoundingBox(shapeBox);
  // 2. Rotate into world axes using the bank's orientation.
  this->getRotation().rotateBB(box.xMin(), box.yMin(), box.zMin(), box.xMax(), box.yMax(), box.zMax());
  // 3. Translate to the pixel's absolute position.
  V3D const pos = getPosAtXYZ(x, y, z);
  box.xMin() += pos.X();
  box.xMax() += pos.X();
  box.yMin() += pos.Y();
  box.yMax() += pos.Y();
  box.zMin() += pos.Z();
  box.zMax() += pos.Z();
}

// ---------------------------------------------------------------------------
// Component lookup
// ---------------------------------------------------------------------------

/** Find a component by name.  PixelAssembly has no children, so only the
 *  assembly itself can be found.
 *
 *  NOTE: This is NOT an override of IComponent::getComponentByName (which does
 *  not exist); it is a freestanding convenience method for direct callers that
 *  hold a PixelAssembly*.
 */
std::shared_ptr<const IComponent> PixelAssembly::getComponentByName(const std::string &cname) const {
  if (cname == this->getName())
    return std::shared_ptr<const IComponent>(this, NoDeleting());
  return {};
}

// ---------------------------------------------------------------------------
// Visitor registration
// ---------------------------------------------------------------------------

size_t PixelAssembly::registerContents(ComponentVisitor &componentVisitor) const {
  return componentVisitor.registerVirtualBank(*this);
}

// ---------------------------------------------------------------------------
// Pixel shape accessor
// ---------------------------------------------------------------------------

std::shared_ptr<const IObject> PixelAssembly::pixelShape() const {
  return isParametrized() ? m_gridBase->m_shape : m_shape;
}

// ---------------------------------------------------------------------------
// IObjComponent methods
// ---------------------------------------------------------------------------

bool PixelAssembly::isValid(const V3D & /*point*/) const {
  throw Kernel::Exception::NotImplementedError("PixelAssembly::isValid() is not implemented.");
}

bool PixelAssembly::isOnSide(const V3D & /*point*/) const {
  throw Kernel::Exception::NotImplementedError("PixelAssembly::isOnSide() is not implemented.");
}

int PixelAssembly::interceptSurface(Track & /*track*/) const {
  throw Kernel::Exception::NotImplementedError("PixelAssembly::interceptSurface() is not implemented.");
}

double PixelAssembly::solidAngle(const Geometry::SolidAngleParams & /*params*/) const {
  throw Kernel::Exception::NotImplementedError("PixelAssembly::solidAngle() is not implemented.");
}

int PixelAssembly::getPointInObject(V3D & /*point*/) const {
  throw Kernel::Exception::NotImplementedError("PixelAssembly::getPointInObject() is not implemented.");
}

/** Compute the overall bounding box for this assembly.
 *
 *  If the ComponentInfo cache is available (parametrized path), use it.
 *  Otherwise sample the 8 corners of the pixel grid.
 */
void PixelAssembly::getBoundingBox(BoundingBox &assemblyBox) const {
  if (isParametrized() && hasComponentInfo()) {
    assemblyBox = m_map->componentInfo().boundingBox(index(), &assemblyBox);
    return;
  }
  BoundingBox bb;
  BoundingBox compBox;
  int const nx = static_cast<int>(xpixels()) - 1;
  int const ny = static_cast<int>(ypixels()) - 1;
  int const nz = static_cast<int>(zpixels()) - 1;
  getBoundingBoxAtXYZ(0, 0, 0, compBox);
  bb.grow(compBox);
  getBoundingBoxAtXYZ(nx, 0, 0, compBox);
  bb.grow(compBox);
  getBoundingBoxAtXYZ(nx, ny, 0, compBox);
  bb.grow(compBox);
  getBoundingBoxAtXYZ(0, ny, 0, compBox);
  bb.grow(compBox);
  getBoundingBoxAtXYZ(0, 0, nz, compBox);
  bb.grow(compBox);
  getBoundingBoxAtXYZ(nx, 0, nz, compBox);
  bb.grow(compBox);
  getBoundingBoxAtXYZ(nx, ny, nz, compBox);
  bb.grow(compBox);
  getBoundingBoxAtXYZ(0, ny, nz, compBox);
  bb.grow(compBox);
  assemblyBox = bb;
}

void PixelAssembly::draw() const {
  if (Handle() == nullptr)
    return;
  Handle()->render();
}

void PixelAssembly::drawObject() const { draw(); }

void PixelAssembly::initDraw() const {
  if (Handle() == nullptr)
    return;
  Handle()->initialize();
}

/** Returns a cuboid representing the whole assembly extent (used for rendering). */
const std::shared_ptr<const IObject> PixelAssembly::shape() const {
  double const szX = static_cast<double>(xpixels());
  double const szY = static_cast<double>(ypixels());
  double const szZ = static_cast<double>(zpixels());
  std::ostringstream xml;
  xml << " <cuboid id=\"detector-shape\"> "
      << "<left-front-bottom-point x=\"" << szX << "\" y=\"" << -szY << "\" z=\"" << -szZ << "\"  /> "
      << "<left-front-top-point  x=\"" << szX << "\" y=\"" << -szY << "\" z=\"" << szZ << "\"  /> "
      << "<left-back-bottom-point  x=\"" << -szX << "\" y=\"" << -szY << "\" z=\"" << -szZ << "\"  /> "
      << "<right-front-bottom-point  x=\"" << szX << "\" y=\"" << szY << "\" z=\"" << -szZ << "\"  /> "
      << "</cuboid>";
  Geometry::ShapeFactory shapeCreator;
  return std::static_pointer_cast<const IObject>(shapeCreator.createShape(xml.str()));
}

const Kernel::Material PixelAssembly::material() const { return Kernel::Material(); }

// ---------------------------------------------------------------------------
// initialize / validateInput / initializeValues
// ---------------------------------------------------------------------------

void PixelAssembly::validateInput() const {
  if (!checkValidOrderString(std::string{m_idFillOrder.begin(), m_idFillOrder.end()}))
    throw std::invalid_argument("PixelAssembly::initialize(): idFillOrder must be a permutation of 'xyz'.");
  if (m_xpixels == 0)
    throw std::invalid_argument("PixelAssembly::initialize(): xpixels must be > 0.");
  if (m_ypixels == 0)
    throw std::invalid_argument("PixelAssembly::initialize(): ypixels must be > 0.");
  if (m_zpixels == 0)
    throw std::invalid_argument("PixelAssembly::initialize(): zpixels must be > 0 (pass 1 for a flat bank).");
}

void PixelAssembly::initializeValues(std::shared_ptr<IObject> shape, size_t xpixels, double xstart, double xstep,
                                     size_t ypixels, double ystart, double ystep, size_t zpixels, double zstart,
                                     double zstep, detid_t idstart, const std::string &idFillOrder, int idstep) {
  m_xpixels = xpixels;
  m_ypixels = ypixels;
  // Normalise: 0 layers means flat bank (treat as 1 layer).
  m_zpixels = (zpixels == 0) ? size_t(1) : zpixels;
  m_xstart = xstart;
  m_ystart = ystart;
  m_zstart = zstart;
  m_xstep = xstep;
  m_ystep = ystep;
  m_zstep = zstep;
  m_shape = std::move(shape);
  m_idstart = idstart;
  m_idFillOrder = {idFillOrder[0], idFillOrder[1], idFillOrder[2]};
  m_idstep = idstep;
  validateInput();
}

/** Store the grid parameters.  No child Detector objects are created. */
void PixelAssembly::initialize(std::shared_ptr<IObject> shape, size_t xpixels, double xstart, double xstep,
                               size_t ypixels, double ystart, double ystep, size_t zpixels, double zstart, double zstep,
                               detid_t idstart, const std::string &idFillOrder, int idstep) {
  if (isParametrized())
    throw std::runtime_error("PixelAssembly::initialize() called on a parametrized PixelAssembly.");
  initializeValues(std::move(shape), xpixels, xstart, xstep, ypixels, ystart, ystep, zpixels, zstart, zstep, idstart,
                   idFillOrder, idstep);
}

// ---------------------------------------------------------------------------
// Stream operator
// ---------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &os, const PixelAssembly &assm) {
  assm.printSelf(os);
  os << "PixelAssembly: " << assm.xpixels() << " x " << assm.ypixels() << " x " << assm.zpixels() << " pixels\n";
  return os;
}

} // namespace Mantid::Geometry
