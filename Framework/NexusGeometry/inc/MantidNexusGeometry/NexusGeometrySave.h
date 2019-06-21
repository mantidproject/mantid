// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_
#define MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_

#include "MantidNexusGeometry/DllConfig.h"
#include <memory>
#include <iostream>

namespace Mantid {

namespace Kernel {

class ProgressBase;
} // namespace Kernel

namespace Geometry {
class ComponentInfo;
}

namespace NexusGeometry {


MANTID_NEXUSGEOMETRY_DLL void
saveInstrument(const Geometry::ComponentInfo &compInfo,
               const std::string &fullPath,
               Kernel::ProgressBase *reporter = nullptr);

} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVE_H_ */
