// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

/*
 * NexusGeometrySave:
 * Saves geometry and metadata from memory
 * to disk in Nexus file format for Instrument 2.0.
 *
 * @author Takudzwa Makoni, RAL (UKRI), ISIS
 * @date 07/08/2019
 */

#ifndef MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_
#define MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_

#include "MantidNexusGeometry/DllConfig.h"
#include <memory>
#include <string>

namespace Mantid {

namespace Kernel {

class ProgressBase;
class V3D;
class Quat;

} // namespace Kernel

namespace Geometry {
class ComponentInfo;
class DetectorInfo;
} // namespace Geometry

namespace NexusGeometry {
namespace NexusGeometrySave {

MANTID_NEXUSGEOMETRY_DLL void saveInstrument(
    const std::pair<std::unique_ptr<Geometry::ComponentInfo>,
                    std::unique_ptr<Geometry::DetectorInfo>> &instrPair,
    const std::string &fullPath, const std::string &rootPath,
    Kernel::ProgressBase *reporter = nullptr);
} // namespace NexusGeometrySave
} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_ */
