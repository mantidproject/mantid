#ifndef MANTID_API_INFOCOMPONENTVISITOR_H_
#define MANTID_API_INFOCOMPONENTVISITOR_H_

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
#include <cstddef>
#include <utility>
#include <vector>

namespace Mantid {

namespace Geometry {
class IComponent;
class ICompAssembly;
class IDetector;
}

namespace API {

class DetectorInfo;

/** InfoComponentVisitor : Visitor for components with access to Info wrapping
  features.

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
class MANTID_API_DLL InfoComponentVisitor
    : public Mantid::Geometry::ComponentVisitor {
private:
  /// Detectors components always specified first
  std::vector<Mantid::Geometry::IComponent *> m_componentIds;

  /// Detector indexes
  std::vector<size_t> m_detectorIndices;

  /// Reference to the detector info.
  const Mantid::API::DetectorInfo &m_detectorInfo;

  /// Only Assemblies and other NON-detectors yield ranges
  std::vector<std::pair<size_t, size_t>> m_ranges;

  /// Internal counter for detectors
  size_t m_detectorCounter;

public:
  InfoComponentVisitor(const Mantid::API::DetectorInfo &detectorInfo);

  virtual void registerComponentAssembly(
      const Mantid::Geometry::ICompAssembly &bank) override;

  virtual void registerGenericComponent(
      const Mantid::Geometry::IComponent &component) override;
  virtual void
  registerDetector(const Mantid::Geometry::IDetector &detector) override;

  std::vector<Mantid::Geometry::IComponent *> componentIds() const;
  std::vector<std::pair<size_t, size_t>> componentDetectorRanges() const;
  std::vector<size_t> detectorIndices() const;
  size_t size() const;
};
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_INFOCOMPONENTVISITOR_H_ */
