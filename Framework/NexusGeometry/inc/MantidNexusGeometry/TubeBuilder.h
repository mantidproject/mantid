// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidNexusGeometry/DllConfig.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <memory>
#include <vector>

namespace Mantid {
namespace Geometry {
class IObject;
}

namespace NexusGeometry {
namespace detail {

/** TubeBuilder : Builder for wrapping the creation of a tube as a collection of
Colinear detectors with cylindrical shape.
*/
class MANTID_NEXUSGEOMETRY_DLL TubeBuilder {
public:
  TubeBuilder(const Mantid::Geometry::IObject &pixelShape, const Eigen::Vector3d &firstDetectorPosition,
              int firstDetectorId);
  const Eigen::Vector3d &tubePosition() const;
  const std::vector<Eigen::Vector3d> &detPositions() const;
  const std::vector<int> &detIDs() const;
  std::shared_ptr<const Mantid::Geometry::IObject> shape() const;
  double tubeHeight() const;
  double tubeRadius() const;
  bool addDetectorIfCoLinear(const Eigen::Vector3d &pos, int detID);
  size_t size() const;

private:
  Eigen::Vector3d m_axis;
  double m_pixelHeight;
  double m_tubeHeight;
  Eigen::Vector3d m_halfHeightVec; // stored for faster calculations
  Eigen::Vector3d m_baseVec;       // stored for faster calculations
  double m_pixelRadius;
  std::vector<Eigen::Vector3d> m_positions;
  std::vector<int> m_detIDs;
  Eigen::Vector3d m_p1; ///< First point which defines the line in space the tube lies along
  Eigen::Vector3d m_p2; ///< Second point which defines the line in space the
                        ///< tube lies along

private:
  bool checkCoLinear(const Eigen::Vector3d &pos) const;
};
} // namespace detail
} // namespace NexusGeometry
} // namespace Mantid
