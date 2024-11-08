// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexusGeometry/AbstractLogger.h"
#include "MantidNexusGeometry/DllConfig.h"
#include <memory>
#include <string>

namespace Mantid {
namespace Geometry {

class Instrument;
}

namespace NexusGeometry {

namespace NexusGeometryParser {

/** createInstrument : Responsible for parsing a nexus geometry file and
  creating an in-memory Mantid instrument.
  This variant extracts the instrument from the first (and assumed only) NXentry in the file.
*/
MANTID_NEXUSGEOMETRY_DLL std::unique_ptr<const Mantid::Geometry::Instrument>
createInstrument(const std::string &fileName, std::unique_ptr<AbstractLogger> logger);

/** createInstrument : Responsible for parsing a nexus geometry file and
  creating an in-memory Mantid instrument.
  This variant extracts the instrument from a specific NXentry in the file.
*/
MANTID_NEXUSGEOMETRY_DLL std::unique_ptr<const Mantid::Geometry::Instrument>
createInstrument(const std::string &fileName, const std::string &parentGroupName,
                 std::unique_ptr<AbstractLogger> logger);

MANTID_NEXUSGEOMETRY_DLL std::string getMangledName(const std::string &fileName, const std::string &instName);

} // namespace NexusGeometryParser
} // namespace NexusGeometry
} // namespace Mantid
