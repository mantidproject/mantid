#ifndef MANTID_GEOMETRY_COMPONENTHELPERS_H_
#define MANTID_GEOMETRY_COMPONENTHELPERS_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Kernel {
// Forward declarations
class Quat;
class V3D;
}

namespace Geometry {
// Forward declarations
class IComponent;
class ParameterMap;

/**
  A set of helper functions for dealing with components, i.e. movement, rotation
  that require
  interaction with the ParamterMap.

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
namespace ComponentHelper {
/**
 * \enum TransformType
 * \brief Specifies how a transformation should be interpreted
 */
enum TransformType {
  Absolute = 0, ///< The value is the absolute new value
  Relative = 1  ///< The transformation given is relative to the original value
};

/// Move a component
MANTID_GEOMETRY_DLL void moveComponent(const IComponent &comp,
                                       ParameterMap &pmap,
                                       const Kernel::V3D &pos,
                                       const TransformType positionType);
/// Rotate a component
MANTID_GEOMETRY_DLL void rotateComponent(const IComponent &comp,
                                         ParameterMap &pmap,
                                         const Kernel::Quat &rot,
                                         const TransformType positionType);

MANTID_GEOMETRY_DLL Geometry::Instrument_sptr
createMinimalInstrument(const Mantid::Kernel::V3D &sourcePos,
                        const Mantid::Kernel::V3D &samplePos,
                        const Mantid::Kernel::V3D &detectorPos);

MANTID_GEOMETRY_DLL Geometry::Instrument_sptr
createVirtualInstrument(Kernel::V3D sourcePos, Kernel::V3D samplePos,
                        const std::vector<Kernel::V3D> &vecdetpos,
                        const std::vector<detid_t> &vecdetid);

MANTID_GEOMETRY_DLL Object_sptr
createSphere(double radius, const Kernel::V3D &centre, const std::string &id);

MANTID_GEOMETRY_DLL std::string
sphereXML(double radius, const Kernel::V3D &centre, const std::string &id);
}
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_COMPONENTHELPERS_H_ */
