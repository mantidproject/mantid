#include "MantidQtWidgets/InstrumentView/PanelsSurface.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedDetector.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/V3D.h"

#include <QApplication>
#include <QCursor>
#include <QMessageBox>
#include <QtDebug>

using namespace Mantid::Geometry;
using Mantid::Beamline::ComponentType;

namespace {

/// static logger
Mantid::Kernel::Logger g_log("PanelsSurface");

/**
 * Given the z axis, define the x and y ones.
 * @param zaxis :: A given vector in 3d space to become the z axis of a
 * coordinate system.
 * @param xaxis :: An output arbitrary vector perpendicular to zaxis.
 * @param yaxis :: An output arbitrary vector perpendicular to both zaxis and
 * xaxis.
 */
void setupBasisAxes(const Mantid::Kernel::V3D &zaxis,
                    Mantid::Kernel::V3D &xaxis, Mantid::Kernel::V3D &yaxis) {
  double R, theta, phi;
  zaxis.getSpherical(R, theta, phi);
  if (theta <= 45.0) {
    xaxis = Mantid::Kernel::V3D(1, 0, 0);
  } else if (phi <= 45.0) {
    xaxis = Mantid::Kernel::V3D(0, 1, 0);
  } else {
    xaxis = Mantid::Kernel::V3D(0, 0, 1);
  }
  yaxis = zaxis.cross_prod(xaxis);
  yaxis.normalize();
  xaxis = yaxis.cross_prod(zaxis);
}

std::vector<Mantid::Kernel::V3D>
retrievePanelCorners(const Mantid::Geometry::ComponentInfo &componentInfo,
                     size_t rootIndex) {
  auto panel = componentInfo.quadrilateralComponent(rootIndex);
  return {componentInfo.position(panel.bottomLeft),
          componentInfo.position(panel.bottomRight),
          componentInfo.position(panel.topRight),
          componentInfo.position(panel.topLeft)};
}

Mantid::Kernel::V3D
calculatePanelNormal(const std::vector<Mantid::Kernel::V3D> &panelCorners) {
  // find the normal
  auto xaxis = panelCorners[1] - panelCorners[0];
  auto yaxis = panelCorners[3] - panelCorners[0];
  auto normal = xaxis.cross_prod(yaxis);
  normal.normalize();
  return normal;
}

size_t findParentBank(const ComponentInfo &componentInfo, size_t rootIndex) {
  // Search for parent component which contains tubes
  auto parent = componentInfo.parent(rootIndex);
  while (componentInfo.children(parent).size() <= 1)
    parent = componentInfo.parent(parent);

  return parent;
}

std::vector<size_t> findBankTubes(const ComponentInfo &componentInfo,
                                  size_t rootIndex) {
  auto comps = componentInfo.componentsInSubtree(rootIndex);
  std::vector<size_t> tubes;

  for (auto comp : comps) {
    if (componentInfo.componentType(comp) == ComponentType::OutlineComposite)
      tubes.push_back(comp);
  }

  return tubes;
}

bool isBankFlat(const ComponentInfo &componentInfo, size_t bankIndex,
                const std::vector<size_t> &tubes,
                const Mantid::Kernel::V3D &normal) {
  for (auto tube : tubes) {
    const auto &children = componentInfo.children(tube);
    auto vector = componentInfo.position(children[0]) -
                  componentInfo.position(children[1]);
    vector.normalize();
    if (fabs(vector.scalar_prod(normal)) > Mantid::Kernel::Tolerance) {
      g_log.warning() << "Assembly " << componentInfo.name(bankIndex)
                      << " isn't flat.\n";
      return false;
    }
  }
  return true;
}

Mantid::Kernel::V3D calculateBankNormal(const ComponentInfo &componentInfo,
                                        const std::vector<size_t> &tubes) {
  // calculate normal from first two tubes in bank as before
  const auto &tube0 = componentInfo.children(tubes[0]);
  const auto &tube1 = componentInfo.children(tubes[1]);
  auto pos = componentInfo.position(tube0[0]);
  auto x = componentInfo.position(tube0[1]) - pos;
  x.normalize();

  auto y = componentInfo.position(tube1[0]) - pos;
  y.normalize();
  auto normal = x.cross_prod(y);

  if (normal.nullVector()) {
    y = componentInfo.position(tube1[1]) - componentInfo.position(tube1[0]);
    y.normalize();
    normal = x.cross_prod(y);
  }

  normal.normalize();

  if (normal.nullVector())
    g_log.warning() << "Colinear Assembly.\n";

  return normal;
}
} // namespace

namespace MantidQt {
namespace MantidWidgets {

/** Constructor
 * @param s The surface of the panel
 */
FlatBankInfo::FlatBankInfo(PanelsSurface *s)
    : rotation(), startDetectorIndex(0), endDetectorIndex(0), polygon(),
      surface(s) {}

/**
 * Translate the bank by a vector.
 * @param shift :: Translation vector.
 */
void FlatBankInfo::translate(const QPointF &shift) {
  double du = shift.x();
  double dv = shift.y();
  polygon.translate(shift);
  for (size_t i = startDetectorIndex; i <= endDetectorIndex; ++i) {
    UnwrappedDetector &udet = surface->m_unwrappedDetectors[i];
    udet.u += du;
    udet.v += dv;
  }
}

PanelsSurface::PanelsSurface(const InstrumentActor *rootActor,
                             const Mantid::Kernel::V3D &origin,
                             const Mantid::Kernel::V3D &axis)
    : UnwrappedSurface(rootActor), m_pos(origin), m_zaxis(axis) {
  setupAxes();
  init();
}

PanelsSurface::~PanelsSurface() { clearBanks(); }

/**
 * Initialize the surface.
 */
void PanelsSurface::init() {
  m_unwrappedDetectors.clear();

  size_t ndet = m_instrActor->ndetectors();
  m_unwrappedDetectors.resize(ndet);
  if (ndet == 0)
    return;

  clearBanks();
  constructFromComponentInfo();
  spreadBanks();

  RectF surfaceRect;
  for (int i = 0; i < m_flatBanks.size(); ++i) {
    RectF rect(m_flatBanks[i]->polygon.boundingRect());
    surfaceRect.unite(rect);
  }

  m_height_max = 0.1;
  m_width_max = 0.1;
  m_viewRect = RectF(surfaceRect);

  double du = m_viewRect.width() * 0.05;
  double dv = m_viewRect.height() * 0.05;
  m_viewRect.adjust(QPointF(-du, -dv), QPointF(du, dv));

  m_u_min = m_viewRect.x0();
  m_u_max = m_viewRect.x1();
  m_v_min = m_viewRect.y0();
  m_v_max = m_viewRect.y1();
}

void PanelsSurface::project(const Mantid::Kernel::V3D &, double &, double &,
                            double &, double &) const {
  throw std::runtime_error(
      "Cannot project an arbitrary point to this surface.");
}

void PanelsSurface::rotate(const UnwrappedDetector &udet,
                           Mantid::Kernel::Quat &R) const {
  const auto &detectorInfo = m_instrActor->detectorInfo();
  int index = m_detector2bankMap[udet.detIndex];
  FlatBankInfo &info = *m_flatBanks[index];
  R = info.rotation * detectorInfo.rotation(udet.detIndex);
}

/**
 * Define a coordinate system for this projection.
 */
void PanelsSurface::setupAxes() {
  setupBasisAxes(m_zaxis, m_xaxis, m_yaxis);
  m_origin.rx() = m_xaxis.scalar_prod(m_pos);
  m_origin.ry() = m_yaxis.scalar_prod(m_pos);
}

//-----------------------------------------------------------------------------------------------//

/**
 * Add a flat bank from an assembly of detectors.
 * @param normal :: Normal vector to the bank's plane.
 * @param detectors :: List of detectorIndices.
 */
void PanelsSurface::addFlatBankOfDetectors(
    const Mantid::Kernel::V3D &normal, const std::vector<size_t> &detectors) {
  int index = m_flatBanks.size();
  // save bank info
  FlatBankInfo *info = new FlatBankInfo(this);
  m_flatBanks << info;
  // record the first detector index of the bank
  info->startDetectorIndex = detectors.front();
  info->endDetectorIndex = detectors.back();

  // keep reference position on the bank's plane
  const auto &detectorInfo = m_instrActor->detectorInfo();
  auto pos0 = detectorInfo.position(detectors[0]);
  auto pos1 = detectorInfo.position(detectors[1]) - pos0;

  info->rotation = calcBankRotation(pos0, normal);
  info->rotation.rotate(pos1);
  pos1 += pos0;
  QPointF p0(m_xaxis.scalar_prod(pos0), m_yaxis.scalar_prod(pos0));
  QPointF p1(m_xaxis.scalar_prod(pos1), m_yaxis.scalar_prod(pos1));
  QVector<QPointF> vert;
  vert << p1 << p0;
  info->polygon = QPolygonF(vert);
  for (auto detector : detectors) {
    addDetector(detector, pos0, index, info->rotation);
    UnwrappedDetector &udet = m_unwrappedDetectors[detector];
    info->polygon << QPointF(udet.u, udet.v);
  }
}

void PanelsSurface::processStructured(const std::vector<size_t> &children,
                                      size_t rootIndex) {
  int index = m_flatBanks.size();
  const auto &componentInfo = m_instrActor->componentInfo();
  auto corners = retrievePanelCorners(componentInfo, rootIndex);
  auto normal = calculatePanelNormal(corners);
  // save bank info
  FlatBankInfo *info = new FlatBankInfo(this);
  m_flatBanks << info;
  // find the rotation to put the bank on the plane
  info->rotation = calcBankRotation(corners[0], normal);
  // record the first detector index of the bank
  info->startDetectorIndex = children.front();
  info->endDetectorIndex = children.back();
  // set the outline
  QVector<QPointF> verts;
  for (auto &corner : corners) {
    auto pos = corner - corners[0];
    info->rotation.rotate(pos);
    pos += corners[0];
    verts << QPointF(pos.X(), pos.Y());
  }

  info->polygon = QPolygonF(verts);

  for (auto child : children)
    addDetector(child, corners[0], index, info->rotation);
}

void PanelsSurface::processTubes(size_t rootIndex, std::vector<bool> &visited) {
  const auto &componentInfo = m_instrActor->componentInfo();
  auto bankIndex = findParentBank(componentInfo, rootIndex);
  auto name = componentInfo.name(bankIndex);
  auto tubes = findBankTubes(componentInfo, bankIndex);
  auto normal = calculateBankNormal(componentInfo, tubes);

  if (normal.nullVector() ||
      !isBankFlat(componentInfo, bankIndex, tubes, normal))
    return;

  // save bank info
  auto index = m_flatBanks.size();
  FlatBankInfo *info = new FlatBankInfo(this);
  m_flatBanks << info;
  // record the first detector index of the bank
  info->startDetectorIndex = componentInfo.children(tubes.front()).front();
  info->endDetectorIndex = componentInfo.children(tubes.back()).back();

  auto pos0 =
      componentInfo.position(componentInfo.children(tubes.front()).front());
  auto pos1 =
      componentInfo.position(componentInfo.children(tubes.front()).back());

  info->rotation = calcBankRotation(pos0, normal);
  pos1 -= pos0;
  info->rotation.rotate(pos1);
  pos1 += pos0;

  QPointF p0(m_xaxis.scalar_prod(pos0), m_yaxis.scalar_prod(pos0));
  QPointF p1(m_xaxis.scalar_prod(pos1), m_yaxis.scalar_prod(pos1));
  QVector<QPointF> vert;
  vert << p0 << p1;
  info->polygon = QPolygonF(vert);

  for (auto tube : tubes) {
    const auto &children = componentInfo.children(tube);
    visited[tube] = true;
    for (auto child : children) {
      if (visited[child])
        continue;
      visited[child] = true;
      addDetector(child, pos0, index, info->rotation);
    }

    auto &udet0 = m_unwrappedDetectors[children.front()];
    auto &udet1 = m_unwrappedDetectors[children.back()];
    QPointF p2(udet0.u, udet0.v);
    QPointF p3(udet1.u, udet1.v);
    //      add a quadrilateral formed by end points of two nearest tubes
    //      assumption is made here that any two adjacent tubes in an assembly's
    //      children's list
    //      are close to each other
    vert << p0 << p1 << p3 << p2;
    info->polygon = info->polygon.united(QPolygonF(vert));
    p0 = p2;
    p1 = p3;
  }
}

std::pair<std::vector<size_t>, Mantid::Kernel::V3D>
PanelsSurface::processUnstructured(const std::vector<size_t> &children,
                                   size_t rootIndex,
                                   std::vector<bool> &visited) {
  Mantid::Kernel::V3D normal;
  const auto &detectorInfo = m_instrActor->detectorInfo();
  Mantid::Kernel::V3D pos0 = detectorInfo.position(children[0]);
  Mantid::Kernel::V3D y = detectorInfo.position(children[1]) - pos0;
  Mantid::Kernel::V3D x;

  visited[children[0]] = true;
  visited[children[1]] = true;

  y.normalize();
  // at first set the normal to an arbitrary vector orthogonal to
  // the line between the first two detectors
  setupBasisAxes(y, normal, x);

  bool normalFound = false;
  const auto &componentInfo = m_instrActor->componentInfo();
  std::vector<size_t> detectors;
  detectors.reserve(children.size());
  detectors.push_back(children[0]);
  detectors.push_back(children[1]);

  for (auto child : children) {
    if (visited[child])
      continue;
    visited[child] = true;
    if (detectorInfo.isMonitor(child))
      continue;

    auto pos = detectorInfo.position(child);

    if (fabs(normal.scalar_prod(pos - pos0)) > Mantid::Kernel::Tolerance) {
      if (!normalFound) {
        // when first non-colinear detector is found set the normal
        x = pos - pos0;
        x.normalize();
        normal = x.cross_prod(y);
        normal.normalize();
        x = y.cross_prod(normal);
        normalFound = true;
      } else {
        g_log.warning() << "Assembly " << componentInfo.name(rootIndex)
                        << " isn't flat.\n";
        break;
      }
    }
    detectors.push_back(child);
  }
  return std::make_pair(detectors, normal);
}

boost::optional<std::pair<std::vector<size_t>, Mantid::Kernel::V3D>>
PanelsSurface::findFlatPanels(size_t rootIndex,
                              const std::vector<size_t> &children,
                              std::vector<bool> &visited) {
  const auto &componentInfo = m_instrActor->componentInfo();
  auto parentIndex = componentInfo.parent(rootIndex);
  auto componentType = componentInfo.componentType(parentIndex);
  if (componentType == ComponentType::Rectangular ||
      componentType == ComponentType::Structured) {
    /* Do nothing until the root index of the structured bank. */
    return boost::none;
  }

  componentType = componentInfo.componentType(rootIndex);
  if (componentType == ComponentType::Rectangular ||
      componentType == ComponentType::Structured) {
    processStructured(children, rootIndex);
    for (auto child : children)
      visited[child] = true;
    return boost::none;
  }

  if (componentType == ComponentType::OutlineComposite) {
    processTubes(rootIndex, visited);
    for (auto child : children)
      visited[child] = true;
    return boost::none;
  }

  return processUnstructured(children, rootIndex, visited);
}

void PanelsSurface::constructFromComponentInfo() {
  const auto &componentInfo = m_instrActor->componentInfo();
  std::vector<bool> visited(componentInfo.size(), false);

  for (size_t i = 0; i < componentInfo.size() - 1; ++i) {
    auto children = componentInfo.detectorsInSubtree(i);

    if (children.size() > 1 && !visited[i]) {
      auto res = findFlatPanels(i, children, visited);
      if (res != boost::none) {
        std::vector<size_t> detectors;
        Mantid::Kernel::V3D normal;
        std::tie(detectors, normal) = res.get();
        if (detectors.size() > 1)
          addFlatBankOfDetectors(normal, detectors);
      }
    } else if (children.size() > 0 &&
               componentInfo.parent(children[0]) == componentInfo.root()) {
      auto child = children[0];
      visited[child] = true;
    }
  }
}

/**
 * Calculate the rotation needed to place a bank on the projection plane.
 *
 * @param detPos :: Position of a detector of the bank.
 * @param normal :: Normal to the bank's plane.
 */
Mantid::Kernel::Quat
PanelsSurface::calcBankRotation(const Mantid::Kernel::V3D &detPos,
                                Mantid::Kernel::V3D normal) const {
  if (normal.cross_prod(m_zaxis).nullVector()) {
    return Mantid::Kernel::Quat();
  }

  // signed shortest distance from the bank's plane to the origin (m_pos)
  double a = normal.scalar_prod(m_pos - detPos);
  // if a is negative the origin is on the "back" side of the plane
  // (the "front" side is facing in the direction of the normal)
  if (a < 0.0) {
    // we need to flip the normal to make the side looking at the origin to be
    // the front one
    normal *= -1;
  }

  return Mantid::Kernel::Quat(normal, m_zaxis);
}

void PanelsSurface::addDetector(size_t detIndex,
                                const Mantid::Kernel::V3D &refPos, int index,
                                Mantid::Kernel::Quat &rotation) {
  const auto &detectorInfo = m_instrActor->detectorInfo();

  auto pos = detectorInfo.position(detIndex);
  m_detector2bankMap[detIndex] = index;
  // get the colour
  UnwrappedDetector udet(m_instrActor->getColor(detIndex), detIndex);
  // apply bank's rotation
  pos -= refPos;
  rotation.rotate(pos);
  pos += refPos;
  udet.u = m_xaxis.scalar_prod(pos);
  udet.v = m_yaxis.scalar_prod(pos);
  udet.uscale = udet.vscale = 1.0;
  this->calcSize(udet);
  m_unwrappedDetectors[detIndex] = udet;
}

/**
 * Spread the banks over the projection plane so that they don't overlap.
 *
 */
void PanelsSurface::spreadBanks() {
  int heavy = findLargestBank();
  for (int i = 0; i < m_flatBanks.size(); ++i) {
    // leave the largest bank where it is
    if (i == heavy)
      continue;
    FlatBankInfo *info = m_flatBanks[i];
    QPolygonF poly = info->polygon;
    QRectF rect = poly.boundingRect();
    // define direction of movement for the bank: radially away from origin
    QPointF centre = rect.center();
    QPointF dir = centre - m_origin;
    qreal length = sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    if (length < 1e-5) {
      dir.setX(1.0);
      dir.setY(0.0);
    } else {
      dir /= length;
    }
    qreal step =
        (fabs(rect.width() * dir.x()) + fabs(rect.height() * dir.y())) / 4;
    dir *= step;
    if (step == 0.0)
      continue;
    // move the bank until it doesn't overlap with anything else
    while (isOverlapped(poly, i)) {
      poly.translate(dir);
    }
    // move all detectors of the bank
    info->translate(poly.boundingRect().center() - centre);
  }
}

/**
 * Find index of the largest bank.
 */
int PanelsSurface::findLargestBank() const {
  double maxArea = 0.0;
  int index = 0;
  for (int i = 0; i < m_flatBanks.size(); ++i) {
    const FlatBankInfo *info = m_flatBanks[i];
    QRectF rect = info->polygon.boundingRect();
    double area = rect.height() * rect.width();
    if (area > maxArea) {
      index = i;
      maxArea = area;
    }
  }
  return index;
}

/**
 * Test if a polygon overlaps with any of the flat banks.
 * @param polygon :: A polygon to test.
 * @param iexclude :: Index of a flat bank which should be excluded from the
 * test.
 */
bool PanelsSurface::isOverlapped(QPolygonF &polygon, int iexclude) const {
  for (int i = 0; i < m_flatBanks.size(); ++i) {
    if (i == iexclude)
      continue;
    QPolygonF poly = polygon.intersected(m_flatBanks[i]->polygon);
    if (poly.size() > 0)
      return true;
  }
  return false;
}

/**
 * Remove all found flat banks
 */
void PanelsSurface::clearBanks() {
  for (int i = 0; i < m_flatBanks.size(); ++i) {
    if (m_flatBanks[i])
      delete m_flatBanks[i];
  }
  m_flatBanks.clear();
}

} // namespace MantidWidgets
} // namespace MantidQt
