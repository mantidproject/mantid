#ifndef MANTID_BEAMLINE_COMPONENTINFO_H_
#define MANTID_BEAMLINE_COMPONENTINFO_H_

#include "MantidBeamline/DllConfig.h"
#include "MantidKernel/cow_ptr.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <utility>
#include <cstddef>
#include <Eigen/Geometry>

namespace Mantid {
namespace Beamline {
class DetectorInfo;
/** ComponentInfo : Provides a component centric view on to the instrument.
  Indexes
  are per component.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_BEAMLINE_DLL ComponentInfo {

private:
  boost::shared_ptr<const std::vector<size_t>> m_assemblySortedDetectorIndices;
  boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>> m_ranges;
  Mantid::Kernel::cow_ptr<std::vector<Eigen::Vector3d>> m_positions;
  Mantid::Kernel::cow_ptr<std::vector<Eigen::Quaterniond>> m_rotations;
  const size_t m_size = 0;
  boost::shared_ptr<DetectorInfo> m_detectorInfo;

public:
  ComponentInfo();
  ComponentInfo(
      boost::shared_ptr<const std::vector<size_t>>
          assemblySortedDetectorIndices,
      boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>> ranges,
  boost::shared_ptr<std::vector<Eigen::Vector3d>> positions,
                  boost::shared_ptr<std::vector<Eigen::Quaterniond>> rotations,
                  boost::shared_ptr<DetectorInfo> detectorInfo);

  std::vector<size_t> detectorIndices(const size_t componentIndex) const;
  size_t size() const;
  bool operator==(const ComponentInfo &other) const;
  bool operator!=(const ComponentInfo &other) const;
  bool isDetector(const size_t componentIndex) const;
  Eigen::Vector3d position(const size_t componentIndex) const;
  Eigen::Quaterniond rotation(const size_t componentIndex) const;
  void setPosition(const size_t componentIndex,
                    const Eigen::Vector3d &position);
  void setRotation(const size_t componentIndex,
                    const Eigen::Quaterniond &rotation);
};
} // namespace Beamline
} // namespace Mantid

#endif /* MANTID_BEAMLINE_COMPONENTINFO_H_ */
