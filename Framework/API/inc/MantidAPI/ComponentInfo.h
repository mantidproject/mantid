#ifndef MANTID_API_COMPONENTINFO_H_
#define MANTID_API_COMPONENTINFO_H_

#include "MantidAPI/DllConfig.h"
#include <unordered_map>
#include <vector>

namespace Mantid {

namespace Geometry {

class IComponent;
}

namespace Beamline {
class ComponentInfo;
}

namespace API {

/** ComponentInfo : Provides a component centric view on the instrument. Indexes
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
class MANTID_API_DLL ComponentInfo {
private:
  /// Reference to the actual ComponentInfo object (non-wrapping part).
  const Beamline::ComponentInfo &m_componentInfo;
  std::vector<Mantid::Geometry::IComponent *> m_componentIds;
  std::unordered_map<Geometry::IComponent *, size_t> m_compIDToIndex;

public:
  ComponentInfo() = default;
  ComponentInfo(const Mantid::Beamline::ComponentInfo &componentInfo,
                std::vector<Mantid::Geometry::IComponent *> &&componentIds);
  std::vector<size_t> detectorIndexes(size_t componentIndex) const;
  size_t size() const;
  size_t indexOf(Geometry::IComponent *id) const;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_COMPONENTINFO_H_ */
