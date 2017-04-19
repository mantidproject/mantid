#ifndef MANTID_API_COMPONENTINFO_H_
#define MANTID_API_COMPONENTINFO_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <unordered_map>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace Mantid {

namespace Geometry {

class IComponent;
}

namespace Beamline {
class ComponentInfo;
}

namespace API {

/** ComponentInfo : Provides a component centric view on to the instrument.
  Indexes are per component.

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
  Beamline::ComponentInfo &m_componentInfo;
  /// Collection of component ids
  boost::shared_ptr<std::vector<Mantid::Geometry::IComponent *>> m_componentIds;
  /// Map of component ids to indexes
  boost::shared_ptr<std::unordered_map<Geometry::IComponent *, size_t>>
      m_compIDToIndex;

public:
  ComponentInfo(Mantid::Beamline::ComponentInfo &componentInfo,
                const std::vector<Mantid::Geometry::IComponent *> componentIds);
  std::vector<size_t> detectorIndices(size_t componentIndex) const;
  std::vector<Mantid::Geometry::IComponent *> componentIds() const;
  size_t size() const;
  Mantid::Kernel::V3D position(const size_t componentIndex) const;
  Mantid::Kernel::Quat rotation(const size_t componentIndex) const;
  void setPosition(const size_t componentIndex,
                   const Mantid::Kernel::V3D &position);
  void setRotation(const size_t componentIndex, const Kernel::Quat &rotation);
  size_t indexOf(Geometry::IComponent *id) const;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_COMPONENTINFO_H_ */
