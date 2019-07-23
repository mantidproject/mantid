// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

/*
 * NexusGeometrySave
 *
 * Save Beamline NXInstrument from Memory to disk
 *
 *@author Takudzwa Makoni, RAL (UKRI), ISIS
 *@date 22/07/2019
 */

#ifndef MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_
#define MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_

#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidKernel/EigenConversionHelpers.h"
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
    const std::string &fullPath, Kernel::ProgressBase *reporter = nullptr);
} // namespace NexusGeometrySave
} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_ */
