#ifndef MANTID_API_APICOMPONENTVISITOR_H_
#define MANTID_API_APICOMPONENTVISITOR_H_

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
#include <cstddef>

namespace Mantid {

namespace Geometry {
class IComponent;
class ICompAssembly;
class IDetector;
}

namespace API {

class DetectorInfo;

/** APIComponentVisitor : Visitor for components with access to API wrapping
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
class MANTID_API_DLL APIComponentVisitor
    : public Mantid::Geometry::ComponentVisitor {
private:
  std::vector<Mantid::Geometry::IComponent *> m_componentIds;
  std::vector<std::vector<size_t>> m_componentDetectorIndexes;
  const Mantid::API::DetectorInfo &m_detectorInfo;

public:
  APIComponentVisitor(const Mantid::API::DetectorInfo &detectorInfo);

  virtual void registerComponentAssembly(
      const Mantid::Geometry::ICompAssembly &bank,
      std::vector<size_t> &parentDetectorIndexes) override;

  virtual void
  registerGenericComponent(const Mantid::Geometry::IComponent &component,
                           std::vector<size_t> &) override;
  virtual void
  registerDetector(const Mantid::Geometry::IDetector &detector,
                   std::vector<size_t> &parentDetectorIndexes) override;

  virtual ~APIComponentVisitor();
  std::vector<Mantid::Geometry::IComponent *> componentIds() const;
  std::vector<std::vector<size_t>> componentDetectorIndexes() const;
};
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_APICOMPONENTVISITOR_H_ */
