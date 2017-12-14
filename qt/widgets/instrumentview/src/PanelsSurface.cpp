#include "MantidQtWidgets/InstrumentView/CompAssemblyActor.h"
#include "MantidQtWidgets/InstrumentView/GLActorVisitor.h"
#include "MantidQtWidgets/InstrumentView/ObjCompAssemblyActor.h"
#include "MantidQtWidgets/InstrumentView/PanelsSurface.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedDetector.h"
#include "MantidQtWidgets/InstrumentView/RectangularDetectorActor.h"
#include "MantidQtWidgets/InstrumentView/StructuredDetectorActor.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/V3D.h"

#include <QApplication>
#include <QCursor>
#include <QMessageBox>
#include <QtDebug>

using namespace Mantid::Geometry;

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
  auto panel = componentInfo.structuredPanel(rootIndex);
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
} // namespace

namespace MantidQt {
namespace MantidWidgets {

/** Constructor
* @param s The surface of the panel
*/
FlatBankInfo::FlatBankInfo(PanelsSurface *s)
    : id(nullptr), rotation(), startDetectorIndex(0), endDetectorIndex(0),
      polygon(), surface(s) {}

/**
* Translate the bank by a vector.
* @param shift :: Translation vector.
*/
void FlatBankInfo::translate(const QPointF &shift) {
  double du = shift.x();
  double dv = shift.y();
  polygon.translate(shift);
  for (size_t i = startDetectorIndex; i < endDetectorIndex; ++i) {
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
  m_assemblies.clear();

  size_t ndet = m_instrActor->ndetectors();
  if (ndet == 0)
    return;

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
  int index = m_detector2bankMap[udet.detID];
  FlatBankInfo &info = *m_flatBanks[index];
  R = info.rotation * udet.rotation;
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
* @param bankId :: Component ID of the bank.
* @param normal :: Normal vector to the bank's plane.
* @param detectors :: List of detectorIndices. 
*/
void PanelsSurface::addFlatBankOfDetectors(
    ComponentID bankId, const Mantid::Kernel::V3D &normal,
    const std::vector<size_t> &detectors) {
  int index = m_flatBanks.size();
  // save bank info
  FlatBankInfo *info = new FlatBankInfo(this);
  m_flatBanks << info;
  info->id = bankId;
  // record the first detector index of the bank
  info->startDetectorIndex = m_unwrappedDetectors.size();
  int nelem = detectors.size();
  m_unwrappedDetectors.reserve(m_unwrappedDetectors.size() + nelem);

  // keep reference position on the bank's plane
  const auto &detectorInfo = m_instrActor->getDetectorInfo();
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
    // update the outline polygon
    UnwrappedDetector &udet = *(m_unwrappedDetectors.end() - 1);
    auto p2 = QPointF(udet.u, udet.v);
    vert.clear();
    vert << p0 << p1 << p2;
    info->polygon = info->polygon.united(QPolygonF(vert));
  }
  // record the end detector index of the bank
  info->endDetectorIndex = m_unwrappedDetectors.size();
}

void PanelsSurface::processStructured(const std::vector<size_t> &children,
                                      size_t rootIndex) {
  int index = m_flatBanks.size();
  const auto &componentInfo = m_instrActor->getComponentInfo();
  auto corners = retrievePanelCorners(componentInfo, rootIndex);
  auto normal = calculatePanelNormal(corners);
  // save bank info
  FlatBankInfo *info = new FlatBankInfo(this);
  m_flatBanks << info;
  // set bank ID
  info->id = componentInfo.componentID(rootIndex)->getComponentID();
  // find the rotation to put the bank on the plane
  info->rotation = calcBankRotation(corners[0], normal);
  // record the first detector index of the bank
  info->startDetectorIndex = m_unwrappedDetectors.size();
  // set the outline
  QVector<QPointF> verts;
  for (auto &corner : corners) {
    auto pos = corner - corners[0];
    info->rotation.rotate(pos);
    pos += corners[0];
    verts << QPointF(pos.X(), pos.Y());
  }

  info->polygon = QPolygonF(verts);

  auto nelem = children.size();
  m_unwrappedDetectors.reserve(m_unwrappedDetectors.size() + nelem);

  for (auto child : children)
    addDetector(child, corners[0], index, info->rotation);
  // record the end detector index of the bank
  info->endDetectorIndex = m_unwrappedDetectors.size();
}

std::pair<std::vector<size_t>, Mantid::Kernel::V3D>
PanelsSurface::processUnstructured(const std::vector<size_t> &children,
                                   size_t rootIndex,
                                   std::vector<bool> &visited) {
  Mantid::Kernel::V3D normal;
  const auto &detectorInfo = m_instrActor->getDetectorInfo();
  Mantid::Kernel::V3D pos0;
  Mantid::Kernel::V3D x, y;
  bool normalFound = false;
  const auto &componentInfo = m_instrActor->getComponentInfo();
  std::vector<size_t> detectors;
  detectors.reserve(children.size());
  for (auto child : children) {
    if (visited[child])
      continue;
    visited[child] = true;

    if (detectorInfo.isMonitor(child))
      continue;
    auto pos = detectorInfo.position(child);
    if (child == children[0])
      pos0 = pos;
    else if (child == children[1]) {
      // at first set the normal to an argbitrary vector orthogonal to
      // the line between the first two detectors
      y = pos - pos0;
      y.normalize();
      setupBasisAxes(y, normal, x);
    } else if (fabs(normal.scalar_prod(pos - pos0)) >
               Mantid::Kernel::Tolerance) {
      if (!normalFound) {
        // when first non-colinear detector is found set the normal
        x = pos - pos0;
        x.normalize();
        normal = x.cross_prod(y);
        normal.normalize();
        x = y.cross_prod(normal);
        normalFound = true;
      } else {
        // TODO: Replace somehow with componentInfo name method.
        /*g_log.warning() << "Assembly " << componentInfo->name(rootIndex)
        << " isn't flat.\n";*/
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
  const auto &componentInfo = m_instrActor->getComponentInfo();
  auto parentIndex = componentInfo.parent(rootIndex);

  if (componentInfo.isStructuredBank(parentIndex)) {
    /* Do nothing until the root index of the structured bank. */
    return boost::none;
  }

  if (componentInfo.isStructuredBank(rootIndex)) {
    processStructured(children, rootIndex);
    for (auto child : children)
      visited[child] = true;
    return boost::none;
  }

  return processUnstructured(children, rootIndex, visited);
}

void PanelsSurface::constructFromComponentInfo() {
  const auto componentInfo = m_instrActor->getComponentInfo();
  std::vector<bool> visited(componentInfo.size(), false);

  for (size_t i = 0; i < componentInfo.size()-1; ++i) {
    auto children = componentInfo.detectorsInSubtree(i);
    
    if (children.size() > 1) {
      auto res = findFlatPanels(i, children, visited);
      if (res != boost::none) {
        std::vector<size_t> detectors;
        Mantid::Kernel::V3D normal;
        std::tie(detectors, normal) = res.get();
        if (!detectors.empty())
          addFlatBankOfDetectors(componentInfo.componentID(i)->getComponentID(),
            normal, detectors);
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
  const auto &detectorInfo = m_instrActor->getDetectorInfo();
  const auto &componentInfo = m_instrActor->getComponentInfo();

  Mantid::Kernel::V3D pos = detectorInfo.position(detIndex);
  Mantid::detid_t detid = detectorInfo.detectorIDs()[detIndex];
  m_detector2bankMap[detid] = index;
  // get the colour
  unsigned char color[3];
  m_instrActor->getColor(detid).getUB3(&color[0]);
  UnwrappedDetector udet(
      color[0], color[1], color[2], detid, pos, detectorInfo.rotation(detIndex),
      componentInfo.scaleFactor(detIndex), componentInfo.shape(detIndex));
  // apply bank's rotation
  pos -= refPos;
  rotation.rotate(pos);
  pos += refPos;
  udet.u = m_xaxis.scalar_prod(pos);
  udet.v = m_yaxis.scalar_prod(pos);
  udet.uscale = udet.vscale = 1.0;
  this->calcSize(udet);
  m_unwrappedDetectors.push_back(udet);
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

} // MantidWidgets
} // MantidQt
