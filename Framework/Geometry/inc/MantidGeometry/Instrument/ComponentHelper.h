// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_COMPONENTHELPERS_H_
#define MANTID_GEOMETRY_COMPONENTHELPERS_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Kernel {
// Forward declarations
class Quat;
class V3D;
} // namespace Kernel

namespace Geometry {
// Forward declarations
class CSGObject;
class IComponent;
class ParameterMap;

/**
  A set of helper functions for dealing with components, i.e. movement, rotation
  that require interaction with the ParameterMap.
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

MANTID_GEOMETRY_DLL Geometry::Instrument_sptr
createMinimalInstrument(const Mantid::Kernel::V3D &sourcePos,
                        const Mantid::Kernel::V3D &samplePos,
                        const Mantid::Kernel::V3D &detectorPos);

MANTID_GEOMETRY_DLL Geometry::Instrument_sptr
createVirtualInstrument(Kernel::V3D sourcePos, Kernel::V3D samplePos,
                        const std::vector<Kernel::V3D> &vecdetpos,
                        const std::vector<detid_t> &vecdetid);

MANTID_GEOMETRY_DLL boost::shared_ptr<Geometry::CSGObject>
createSphere(double radius, const Kernel::V3D &centre, const std::string &id);

MANTID_GEOMETRY_DLL std::string
sphereXML(double radius, const Kernel::V3D &centre, const std::string &id);
} // namespace ComponentHelper
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_COMPONENTHELPERS_H_ */
