#ifndef MANTIDNEXUSGEOMETRY_TUBE_H
#define MANTIDNEXUSGEOMETRY_TUBE_H
#include "MantidNexusGeometry/DllConfig.h"
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
class MANTID_NEXUSGEOMETRY_DLL Tube {
public:
  Tube(const Mantid::Geometry::IObject &firstDetectorShape,
       Eigen::Vector3d firstDetectorPosition, int firstDetectorId);
  ~Tube();

  const Eigen::Vector3d &position() const;
  const std::vector<Eigen::Vector3d> &detPositions() const;
  const std::vector<int> &detIDs() const;
  boost::shared_ptr<const Mantid::Geometry::IObject> shape() const;
  const double height() const;
  const double radius() const;
  bool addDetectorIfCoLinear(const Eigen::Vector3d &pos, int detID);
  const size_t size() const;

private:
  Eigen::Vector3d m_axis;
  double m_baseHeight;
  double m_height;
  Eigen::Vector3d m_halfHeightVec; // stored for faster calculations
  Eigen::Vector3d m_baseVec;       // stored for faster calculations
  double m_radius;
  std::vector<Eigen::Vector3d> m_positions;
  std::vector<int> m_detIDs;
  Eigen::Vector3d
      m_p1; ///< First point which defines the line in space the tube lies along
  Eigen::Vector3d m_p2; ///< Second point which defines the line in space the
                        ///< tube lies along

private:
  bool checkCoLinear(const Eigen::Vector3d &pos) const;
};
} // namespace detail
} // namespace NexusGeometry
} // namespace Mantid

#endif // MANTIDNEXUSGEOMETRY_TUBE_H
