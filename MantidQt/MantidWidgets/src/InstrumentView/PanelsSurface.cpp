#include "MantidQtMantidWidgets/InstrumentView/CompAssemblyActor.h"
#include "MantidQtMantidWidgets/InstrumentView/GLActorVisitor.h"
#include "MantidQtMantidWidgets/InstrumentView/ObjCompAssemblyActor.h"
#include "MantidQtMantidWidgets/InstrumentView/PanelsSurface.h"
#include "MantidQtMantidWidgets/InstrumentView/RectangularDetectorActor.h"
#include "MantidQtMantidWidgets/InstrumentView/StructuredDetectorActor.h"

#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
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
}

namespace MantidQt {
namespace MantidWidgets {

/** Constructor
* @param s The surface of the panel
*/
FlatBankInfo::FlatBankInfo(PanelsSurface *s)
    : id(0), rotation(), startDetectorIndex(0), endDetectorIndex(0), polygon(),
      surface(s) {}

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

  // Pre-calculate all the detector positions (serial because
  // I suspect the IComponent->getPos() method to not be properly thread safe)
  m_instrActor->cacheDetPos();

  findFlatBanks();
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

/**
* Given the z axis, define the x and y ones.
* @param zaxis :: A given vector in 3d space to become the z axis of a
* coordinate system.
* @param xaxis :: An output arbitrary vector perpendicular to zaxis.
* @param yaxis :: An output arbitrary vector perpendicular to both zaxis and
* xaxis.
*/
void PanelsSurface::setupBasisAxes(const Mantid::Kernel::V3D &zaxis,
                                   Mantid::Kernel::V3D &xaxis,
                                   Mantid::Kernel::V3D &yaxis) const {
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

//-----------------------------------------------------------------------------------------------//

class FlatBankFinder : public GLActorConstVisitor {
  PanelsSurface &m_surface;

public:
  explicit FlatBankFinder(PanelsSurface &surface) : m_surface(surface) {}

  bool visit(const GLActor *) override { return false; }
  bool visit(const GLActorCollection *) override { return false; }
  bool visit(const ComponentActor *) override { return false; }
  bool visit(const InstrumentActor *) override { return false; }
  bool visit(const ObjCompAssemblyActor *) override { return false; }

  bool visit(const CompAssemblyActor *actor) override {
    m_surface.addObjCompAssemblies(actor->getComponent()->getComponentID());
    return false;
  }

  bool visit(const RectangularDetectorActor *actor) override {
    m_surface.addRectangularDetector(actor->getComponent()->getComponentID());
    return false;
  }

  bool visit(const StructuredDetectorActor *actor) override {
    m_surface.addStructuredDetector(actor->getComponent()->getComponentID());
    return false;
  }
};

/**
* Traverse the instrument tree and find the banks which detectors lie in the
* same plane.
*
*/
void PanelsSurface::findFlatBanks() {
  clearBanks();
  FlatBankFinder finder(*this);
  m_instrActor->accept(finder);
}

//-----------------------------------------------------------------------------------------------//

/**
* Add a flat bank from an assembly of ObjCompAssemblies.
* @param bankId :: Component ID of the bank.
* @param normal :: Normal vector to the bank's plane.
* @param objCompAssemblies :: List of component IDs. Each component must cast to
* ObjCompAssembly.
*/
void PanelsSurface::addFlatBank(ComponentID bankId,
                                const Mantid::Kernel::V3D &normal,
                                QList<ComponentID> objCompAssemblies) {
  int index = m_flatBanks.size();
  // save bank info
  FlatBankInfo *info = new FlatBankInfo(this);
  m_flatBanks << info;
  info->id = bankId;
  // record the first detector index of the bank
  info->startDetectorIndex = m_unwrappedDetectors.size();
  bool doneRotation = false;
  // keep reference position on the bank's plane
  Mantid::Kernel::V3D pos0;
  QPointF p0, p1;
  Mantid::Geometry::Instrument_const_sptr instr = m_instrActor->getInstrument();
  // loop over the assemblies and process the detectors
  foreach (ComponentID id, objCompAssemblies) {
    Mantid::Geometry::ICompAssembly_const_sptr assembly =
        boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(
            instr->getComponentByID(id));
    assert(assembly);
    int nelem = assembly->nelements();
    m_unwrappedDetectors.reserve(m_unwrappedDetectors.size() + nelem);
    for (int i = 0; i < nelem; ++i) {
      // setup detector info
      auto det = boost::dynamic_pointer_cast<const Mantid::Geometry::IDetector>(
          assembly->getChild(i));
      if (!doneRotation) {
        pos0 = det->getPos();
        // find the rotation to put the bank on the plane
        info->rotation = calcBankRotation(pos0, normal);
        Mantid::Kernel::V3D pos1 = assembly->getChild(nelem - 1)->getPos();
        pos1 -= pos0;
        info->rotation.rotate(pos1);
        pos1 += pos0;
        // start forming the outline polygon
        p0.rx() = m_xaxis.scalar_prod(pos0);
        p0.ry() = m_yaxis.scalar_prod(pos0);
        p1.rx() = m_xaxis.scalar_prod(pos1);
        p1.ry() = m_yaxis.scalar_prod(pos1);
        QVector<QPointF> vert;
        vert << p1 << p0;
        info->polygon = QPolygonF(vert);
        doneRotation = true;
      }
      // add the detector
      addDetector(*det, pos0, index, info->rotation);
    }
    // update the outline polygon
    UnwrappedDetector &udet0 = *(m_unwrappedDetectors.end() - nelem);
    UnwrappedDetector &udet1 = m_unwrappedDetectors.back();
    //      get the tube end points
    QPointF p3 = QPointF(udet0.u, udet0.v);
    QPointF p4 = QPointF(udet1.u, udet1.v);
    QVector<QPointF> vert;
    //      add a quadrilateral formed by end points of two nearest tubes
    //      assumption is made here that any two adjacent tubes in an assembly's
    //      children's list
    //      are close to each other
    vert << p0 << p1 << p4 << p3;
    info->polygon = info->polygon.united(QPolygonF(vert));
    p0 = p3;
    p1 = p4;
  }
  // record the end detector index of the bank
  info->endDetectorIndex = m_unwrappedDetectors.size();
}

/**
* Add a flat bank from an assembly of detectors.
* @param bankId :: Component ID of the bank.
* @param normal :: Normal vector to the bank's plane.
* @param detectors :: List of component IDs. Each component must cast to
* Detector.
*/
void PanelsSurface::addFlatBankOfDetectors(ComponentID bankId,
                                           const Mantid::Kernel::V3D &normal,
                                           QList<ComponentID> detectors) {
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
  Mantid::Kernel::V3D pos0, pos1;
  QPointF p0, p1;
  Mantid::Geometry::Instrument_const_sptr instr = m_instrActor->getInstrument();
  // loop over the detectors
  for (int i = 0; i < detectors.size(); ++i) {
    ComponentID id = detectors[i];
    auto det = boost::dynamic_pointer_cast<const Mantid::Geometry::IDetector>(
        instr->getComponentByID(id));

    if (i == 0) {
      pos0 = det->getPos();
    } else if (i == 1) {
      // find the rotation to put the bank on the plane
      info->rotation = calcBankRotation(pos0, normal);
      pos1 = det->getPos();
      pos1 -= pos0;
      info->rotation.rotate(pos1);
      pos1 += pos0;
      // start forming the outline polygon
      p0.rx() = m_xaxis.scalar_prod(pos0);
      p0.ry() = m_yaxis.scalar_prod(pos0);
      p1.rx() = m_xaxis.scalar_prod(pos1);
      p1.ry() = m_yaxis.scalar_prod(pos1);
      QVector<QPointF> vert;
      vert << p1 << p0;
      info->polygon = QPolygonF(vert);
    }
    // add the detector
    addDetector(*det, pos0, index, info->rotation);
    // update the outline polygon
    UnwrappedDetector &udet = *(m_unwrappedDetectors.end() - 1);
    QPointF p2 = QPointF(udet.u, udet.v);
    QVector<QPointF> vert;
    vert << p0 << p1 << p2;
    info->polygon = info->polygon.united(QPolygonF(vert));
  }

  // record the end detector index of the bank
  info->endDetectorIndex = m_unwrappedDetectors.size();
}

/**
* Add a component assembly containing a flat array of ObjCompAssemblies.
* @param bankId :: Component id of an assembly.
*/
void PanelsSurface::addObjCompAssemblies(ComponentID bankId) {
  Mantid::Geometry::Instrument_const_sptr instr = m_instrActor->getInstrument();
  boost::shared_ptr<const Mantid::Geometry::CompAssembly> assembly =
      boost::dynamic_pointer_cast<const Mantid::Geometry::CompAssembly>(
          instr->getComponentByID(bankId));

  size_t nelem = static_cast<size_t>(assembly->nelements());
  // assemblies with one element cannot be flat (but its element can be)
  if (nelem == 1) {
    return;
  }

  QList<ComponentID> objCompAssemblies;
  // normal to the plane, undefined at first
  Mantid::Kernel::V3D normal(0, 0, 0);
  Mantid::Kernel::V3D x, y, pos;
  for (size_t i = 0; i < nelem; ++i) {
    auto elem = assembly->getChild((int)i);
    ObjCompAssembly *objCompAssembly =
        dynamic_cast<ObjCompAssembly *>(elem.get());
    if (!objCompAssembly) {
      CompAssembly *compAssembly = dynamic_cast<CompAssembly *>(elem.get());
      if (!compAssembly || compAssembly->nelements() != 1) {
        // m_surface.g_log.warning() << "Not a CompAssembly\n";
        addCompAssembly(bankId);
        return;
      }
      elem = compAssembly->getChild(0);
      objCompAssembly = dynamic_cast<ObjCompAssembly *>(elem.get());
      if (!objCompAssembly) {
        // m_surface.g_log.warning() << "Not a ObjCompAssembly\n";
        return;
      }
    }
    if (i == 0) {
      pos = objCompAssembly->getChild(0)->getPos();
      x = objCompAssembly->getChild(1)->getPos() - pos;
      x.normalize();
    } else if (i == 1) {
      y = objCompAssembly->getChild(0)->getPos() - pos;
      y.normalize();
      normal = x.cross_prod(y);
      if (normal.nullVector()) {
        y = objCompAssembly->getChild(1)->getPos() -
            objCompAssembly->getChild(0)->getPos();
        y.normalize();
        normal = x.cross_prod(y);
      }
      if (normal.nullVector()) {
        g_log.warning() << "Colinear ObjCompAssemblies\n";
        return;
      }
      normal.normalize();
    } else {
      Mantid::Kernel::V3D vector = objCompAssembly->getChild(0)->getPos() -
                                   objCompAssembly->getChild(1)->getPos();
      vector.normalize();
      if (fabs(vector.scalar_prod(normal)) > Mantid::Kernel::Tolerance) {
        g_log.warning() << "Assembly " << assembly->getName()
                        << " isn't flat.\n";
        return;
      }
    }
    objCompAssemblies << objCompAssembly->getComponentID();
  }
  if (!objCompAssemblies.isEmpty()) {
    addFlatBank(assembly->getComponentID(), normal, objCompAssemblies);
  }
}

/**
* Add an assembly if its detectors are in the same plane.
* @param bankId :: Component id of an assembly.
*/
void PanelsSurface::addCompAssembly(ComponentID bankId) {
  Mantid::Geometry::Instrument_const_sptr instr = m_instrActor->getInstrument();
  boost::shared_ptr<const Mantid::Geometry::CompAssembly> assembly =
      boost::dynamic_pointer_cast<const Mantid::Geometry::CompAssembly>(
          instr->getComponentByID(bankId));

  size_t nelem = static_cast<size_t>(assembly->nelements());
  // normal to the plane, undefined at first
  Mantid::Kernel::V3D normal, x, y;
  Mantid::Kernel::V3D pos0;
  bool normalFound = false;
  QList<ComponentID> detectors;
  const auto &detectorInfo = m_instrActor->getWorkspace()->detectorInfo();
  for (size_t i = 0; i < nelem; ++i) {
    auto elem = assembly->getChild((int)i);
    Mantid::Geometry::IDetector_const_sptr det =
        boost::dynamic_pointer_cast<const Mantid::Geometry::IDetector>(elem);
    if (!det) {
      return;
    }
    size_t detIndex = detectorInfo.indexOf(det->getID());
    if (detectorInfo.isMonitor(detIndex))
      continue;
    Mantid::Kernel::V3D pos = detectorInfo.position(detIndex);
    if (i == 0) {
      pos0 = pos;
    } else if (i == 1) {
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
        g_log.warning() << "Assembly " << assembly->getName()
                        << " isn't flat.\n";
        return;
      }
    }
    detectors << det->getComponentID();
  }

  // normalFound doesn't have to be true at this point
  // if it is false then the normal was found by the first guess

  // add the detectors
  if (!detectors.isEmpty()) {
    addFlatBankOfDetectors(bankId, normal, detectors);
  }
}

/**
* Add a rectangular detector which is flat.
* @param bankId :: Component id of a rectangular detector.
*/
void PanelsSurface::addRectangularDetector(ComponentID bankId) {
  Mantid::Geometry::Instrument_const_sptr instr = m_instrActor->getInstrument();
  Mantid::Geometry::RectangularDetector_const_sptr rectDetector =
      boost::dynamic_pointer_cast<const Mantid::Geometry::RectangularDetector>(
          instr->getComponentByID(bankId));

  int nx = rectDetector->xpixels();
  int ny = rectDetector->ypixels();
  Mantid::Kernel::V3D pos0 = rectDetector->getAtXY(0, 0)->getPos();
  Mantid::Kernel::V3D pos1 = rectDetector->getAtXY(nx - 1, 0)->getPos();
  Mantid::Kernel::V3D pos2 = rectDetector->getAtXY(nx - 1, ny - 1)->getPos();
  Mantid::Kernel::V3D pos3 = rectDetector->getAtXY(0, ny - 1)->getPos();

  // find the normal
  Mantid::Kernel::V3D xaxis = pos1 - pos0;
  Mantid::Kernel::V3D yaxis = pos3 - pos0;
  Mantid::Kernel::V3D normal = xaxis.cross_prod(yaxis);
  normal.normalize();

  int index = m_flatBanks.size();
  // save bank info
  FlatBankInfo *info = new FlatBankInfo(this);
  m_flatBanks << info;
  info->id = bankId;
  // find the rotation to put the bank on the plane
  info->rotation = calcBankRotation(pos0, normal);
  // record the first detector index of the bank
  info->startDetectorIndex = m_unwrappedDetectors.size();
  // set the outline
  QVector<QPointF> verts;
  Mantid::Kernel::V3D pos = pos0;
  verts << QPointF(pos.X(), pos.Y());

  pos = pos1 - pos0;
  info->rotation.rotate(pos);
  pos += pos0;
  verts << QPointF(pos.X(), pos.Y());

  pos = pos2 - pos0;
  info->rotation.rotate(pos);
  pos += pos0;
  verts << QPointF(pos.X(), pos.Y());

  pos = pos3 - pos0;
  info->rotation.rotate(pos);
  pos += pos0;
  verts << QPointF(pos.X(), pos.Y());

  info->polygon = QPolygonF(verts);

  int nelem = rectDetector->nelements();
  m_unwrappedDetectors.reserve(m_unwrappedDetectors.size() + nelem);

  for (int i = 0; i < nx; ++i)
    for (int j = 0; j < ny; ++j) {
      Mantid::Geometry::IDetector_const_sptr det = rectDetector->getAtXY(i, j);
      addDetector(*det, pos0, index, info->rotation);
    }

  // record the end detector index of the bank
  info->endDetectorIndex = m_unwrappedDetectors.size();
}

/**
* Add a structured detector which is flat.
* @param bankId :: Component id of a structured detector.
*/
void PanelsSurface::addStructuredDetector(
    Mantid::Geometry::ComponentID bankId) {
  Mantid::Geometry::Instrument_const_sptr instr = m_instrActor->getInstrument();
  Mantid::Geometry::StructuredDetector_const_sptr structDetector =
      boost::dynamic_pointer_cast<const Mantid::Geometry::StructuredDetector>(
          instr->getComponentByID(bankId));

  auto nx = structDetector->xPixels();
  auto ny = structDetector->yPixels();
  Mantid::Kernel::V3D pos0 = structDetector->getAtXY(0, 0)->getPos();
  Mantid::Kernel::V3D pos1 = structDetector->getAtXY(nx - 1, 0)->getPos();
  Mantid::Kernel::V3D pos2 = structDetector->getAtXY(nx - 1, ny - 1)->getPos();
  Mantid::Kernel::V3D pos3 = structDetector->getAtXY(0, ny - 1)->getPos();

  // find the normal
  Mantid::Kernel::V3D xaxis = pos1 - pos0;
  Mantid::Kernel::V3D yaxis = pos3 - pos0;
  Mantid::Kernel::V3D normal = xaxis.cross_prod(yaxis);
  normal.normalize();

  int index = m_flatBanks.size();
  // save bank info
  FlatBankInfo *info = new FlatBankInfo(this);
  m_flatBanks << info;
  info->id = bankId;
  // find the rotation to put the bank on the plane
  info->rotation = calcBankRotation(pos0, normal);
  // record the first detector index of the bank
  info->startDetectorIndex = m_unwrappedDetectors.size();
  // set the outline
  QVector<QPointF> verts;
  Mantid::Kernel::V3D pos = pos0;
  verts << QPointF(pos.X(), pos.Y());

  pos = pos1 - pos0;
  info->rotation.rotate(pos);
  pos += pos0;
  verts << QPointF(pos.X(), pos.Y());

  pos = pos2 - pos0;
  info->rotation.rotate(pos);
  pos += pos0;
  verts << QPointF(pos.X(), pos.Y());

  pos = pos3 - pos0;
  info->rotation.rotate(pos);
  pos += pos0;
  verts << QPointF(pos.X(), pos.Y());

  info->polygon = QPolygonF(verts);

  int nelem = structDetector->nelements();
  m_unwrappedDetectors.reserve(m_unwrappedDetectors.size() + nelem);

  for (size_t i = 0; i < nx; ++i)
    for (size_t j = 0; j < ny; ++j) {
      Mantid::Geometry::IDetector_const_sptr det =
          structDetector->getAtXY(i, j);
      addDetector(*det, pos0, index, info->rotation);
    }

  // record the end detector index of the bank
  info->endDetectorIndex = m_unwrappedDetectors.size();
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

void PanelsSurface::addDetector(const Mantid::Geometry::IDetector &det,
                                const Mantid::Kernel::V3D &refPos, int index,
                                Mantid::Kernel::Quat &rotation) {
  // setup detector info
  Mantid::Kernel::V3D pos = det.getPos();
  Mantid::detid_t detid = det.getID();
  m_detector2bankMap[detid] = index;
  // get the colour
  unsigned char color[3];
  m_instrActor->getColor(detid).getUB3(&color[0]);
  UnwrappedDetector udet(color, det);
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
