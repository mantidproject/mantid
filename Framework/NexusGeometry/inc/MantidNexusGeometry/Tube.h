#ifndef MANTIDNEXUSGEOMETRY_TUBE_H
#define MANTIDNEXUSGEOMETRY_TUBE_H
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <boost/shared_ptr.hpp>
#include <vector>

namespace Mantid {
namespace Geometry {
class IObject;
}

namespace NexusGeometry {
namespace detail {
class Tube {
public:
  Tube(const Mantid::Geometry::IObject &firstDetectorShape,
       Eigen::Vector3d firstDetectorPosition, int firstDetectorId);
  const Eigen::Vector3d &position() const;
  const std::vector<Eigen::Vector3d> &detPositions() const;
  const std::vector<int> &detIDs() const;
  boost::shared_ptr<const Mantid::Geometry::IObject> shape() const;
  bool addDetectorIfCoLinear(const Eigen::Vector3d &pos, int detID);

private:
  Eigen::Vector3d m_axis;
  double m_height = 0;
  const double m_radius;
  std::vector<Eigen::Vector3d> m_positions;
  std::vector<int> m_detIDs;
  Eigen::Vector3d m_p1;
  Eigen::Vector3d m_p2;

private:
  bool checkCoLinear(const Eigen::Vector3d &pos) const;
};
} // namespace detail
} // namespace NexusGeometry
} // namespace Mantid

#endif // MANTIDNEXUSGEOMETRY_TUBE_H
