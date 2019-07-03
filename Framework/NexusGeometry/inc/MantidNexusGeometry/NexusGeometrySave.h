// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_
#define MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/EigenConversionHelpers.h"

#include "MantidNexusGeometry/DllConfig.h"
#include <string>

namespace Mantid {

namespace Kernel {

class ProgressBase;
} // namespace Kernel

namespace Geometry {
class ComponentInfo;
}

namespace NexusGeometry {

MANTID_NEXUSGEOMETRY_DLL Eigen::Affine3d
toEigenTransform(const V3D vector, const Quat quaternion);

MANTID_NEXUSGEOMETRY_DLL void
saveInstrument(const Geometry::ComponentInfo &compInfo,
               const std::string &fullPath,
               Kernel::ProgressBase *reporter = nullptr);

} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_ */
