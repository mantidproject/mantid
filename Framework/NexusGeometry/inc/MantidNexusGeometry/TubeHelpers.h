// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/IDTypes.h"
#include "MantidNexusGeometry/TubeBuilder.h"
#include <Eigen/Core>
#include <vector>
// Eigen typedefs
using Pixels = Eigen::Matrix<double, 3, Eigen::Dynamic>;

namespace Mantid {
namespace Geometry {
class IObject;
}
namespace NexusGeometry {
namespace detail {
class Tube;
}
namespace TubeHelpers {
MANTID_NEXUSGEOMETRY_DLL std::vector<detail::TubeBuilder> findAndSortTubes(const Mantid::Geometry::IObject &detShape,
                                                                           const Pixels &detPositions,
                                                                           const std::vector<Mantid::detid_t> &detIDs);
MANTID_NEXUSGEOMETRY_DLL std::vector<Mantid::detid_t> notInTubes(const std::vector<detail::TubeBuilder> &tubes,
                                                                 std::vector<Mantid::detid_t> detIDs);
} // namespace TubeHelpers
} // namespace NexusGeometry
} // namespace Mantid
