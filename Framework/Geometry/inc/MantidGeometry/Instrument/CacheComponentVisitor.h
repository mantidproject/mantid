#ifndef MANTID_GEOMETRY_CACHECOMPONENTVISITOR_H_
#define MANTID_GEOMETRY_CACHECOMPONENTVISITOR_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
#include <vector>

namespace Mantid {
namespace Geometry {

/** CacheComponentVisitor : Visits an instrument collecting key, unparameterised
 * information such as the mapping information ComponentID -> component index

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
class MANTID_GEOMETRY_DLL CacheComponentVisitor
    : public Mantid::Geometry::ComponentVisitor {
public:
  CacheComponentVisitor() = default;
  virtual size_t registerComponentAssembly(
      const class Geometry::ICompAssembly &assembly) override;
  virtual size_t registerGenericComponent(
      const class Geometry::IComponent &component) override;
  virtual size_t
  registerDetector(const class Geometry::IDetector &detector) override;
  std::vector<class Geometry::IComponent *> componentIds() const;

private:
  std::vector<class Geometry::IComponent *> m_detectorComponentIds;
  std::vector<class Geometry::IComponent *> m_componentIds;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_CACHECOMPONENTVISITOR_H_ */
