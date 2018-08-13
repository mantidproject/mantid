#include "MantidNexusGeometry/Tube.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidNexusGeometry/NexusShapeFactory.h"

namespace Mantid {
namespace NexusGeometry {

namespace detail {

Tube::Tube(const Mantid::Geometry::IObject &firstDetectorShape,
           Eigen::Vector3d firstDetectorPosition, int firstDetectorId)
    : m_radius(firstDetectorShape.getGeometryHandler()->shapeInfo().radius()),
      m_baseHeight(
          firstDetectorShape.getGeometryHandler()->shapeInfo().height()) {
  // Get axis along which cylinder lies
  m_axis = Kernel::toVector3d(
      firstDetectorShape.getGeometryHandler()->shapeInfo().points()[1]);
  // Set position and id of first detector in tube
  m_positions.push_back(firstDetectorPosition);
  m_detIDs.push_back(firstDetectorId);

  // points which define the line the tube sits on
  m_p1 = m_axis + firstDetectorPosition;
  m_p2 = firstDetectorPosition;

  m_height = m_baseHeight;

  auto norm = m_axis.norm();
  auto factor = (m_baseHeight / 2.0) / norm;
  m_halfHeightVec = m_axis * factor;
  m_baseVec = firstDetectorPosition - m_halfHeightVec;
}

Tube::~Tube() {}

const Eigen::Vector3d &Tube::position() const { return m_baseVec; }

const size_t Tube::size() const { return m_positions.size(); }

const std::vector<Eigen::Vector3d> &Tube::detPositions() const {
  return m_positions;
}

const std::vector<int> &Tube::detIDs() const { return m_detIDs; }

boost::shared_ptr<const Mantid::Geometry::IObject> Tube::shape() const {
  Eigen::Matrix<double, 3, 3> points;
  // calcualte height vector;
  // Centre shape about (0, 0, 0)
  auto p1 = m_positions.front();
  auto p2 = m_positions.back();
  p1 = p1 - m_halfHeightVec;
  p2 = p2 + m_halfHeightVec;
  auto centre = (p1 + p2) / 2;

  points.col(0) = p1 - centre;
  points.col(2) = p2 - centre;

  // Find point on outer circle
  auto normVec = p1.cross(p2);
  auto norm = normVec.norm();
  auto factor = m_radius / norm;
  points.col(1) = (normVec * factor) + (points.col(0));

  return NexusShapeFactory::createCylinder(points);
}

const double Tube::height() const { return m_height; }

const double Tube::radius() const { return m_radius; }

bool Tube::addDetectorIfCoLinear(const Eigen::Vector3d &pos, int detID) {
  if (checkCoLinear(pos)) {
    m_positions.push_back(pos);
    m_detIDs.push_back(detID);

    // calculate half height vector for detector
    auto dist = (m_baseVec) - (pos + m_halfHeightVec);

    if (dist.norm() > m_height)
      m_height = dist.norm();
    return true;
  }

  return false;
}

bool Tube::checkCoLinear(const Eigen::Vector3d &pos) const {
  // Check if pos is on the same line as p1 and p2
  auto numVec = ((m_p2 - m_p1).cross(m_p1 - pos));
  auto denomVec = (m_p2 - m_p1);
  auto num = numVec.norm();

  if (num == 0)
    return true;

  return denomVec.norm() == 0.0;
}
} // namespace detail
} // namespace NexusGeometry
} // namespace Mantid