#ifndef MANTID_GEOMETRY_CAN_H_
#define MANTID_GEOMETRY_CAN_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/Object.h"

namespace Mantid {
namespace Geometry {

/**
  Models a Can, which is used to hold a sample in the beam. It gets most
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
class MANTID_GEOMETRY_DLL Can final : public Object {
public:
  Can() = default;
  Can(std::string sampleTemplateXML);

  const std::string &sampleShapeTemplate() const;

private:
  std::string m_sampleShapeTemplate;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_CAN_H_ */
