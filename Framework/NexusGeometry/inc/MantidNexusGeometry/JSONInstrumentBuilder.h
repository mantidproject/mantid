// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDER_H_
#define MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDER_H_

#include "MantidNexusGeometry/DllConfig.h"
#include <memory>
#include <string>

namespace Mantid {
namespace NexusGeometry {

class JSONGeometryParser;

/** JSONInstrumentBuilder : Builds in-memory instrument from json string
 * representing Nexus instrument geometry.
 */
class MANTID_NEXUSGEOMETRY_DLL JSONInstrumentBuilder {
public:
  JSONInstrumentBuilder() = delete;
  JSONInstrumentBuilder(const std::string &jsonGeometry);
  ~JSONInstrumentBuilder() = default;

private:
  void buildGeometry();

private:
  std::unique_ptr<JSONGeometryParser> m_parser;
};

} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDER_H_ */