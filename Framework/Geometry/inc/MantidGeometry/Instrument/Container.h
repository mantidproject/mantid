#ifndef MANTID_GEOMETRY_CONTAINER_H_
#define MANTID_GEOMETRY_CONTAINER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/Object.h"
#include <unordered_map>

namespace Mantid {
namespace Geometry {

/**
  Models a Container is used to hold a sample in the beam. It gets most
  of its functionality from Geometry::Object but can also hold a
  definition of what the sample geometry itself would be. If the sample shape
  definition is set then we term this a constriained sample geometry.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_GEOMETRY_DLL Container final : public Object {
public:
  typedef std::unordered_map<std::string, double> ShapeArgs;

  Container() = default;
  Container(std::string xml);

  bool hasSampleShape() const;
  Object_sptr createSampleShape(const ShapeArgs &args) const;

  void setSampleShape(const std::string &sampleShapeXML);

private:
  std::string m_sampleShapeXML;
};

/// Typdef for a shared pointer
typedef boost::shared_ptr<Container> Container_sptr;
/// Typdef for a shared pointer to a const object
typedef boost::shared_ptr<const Container> Container_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_CONTAINER_H_ */
