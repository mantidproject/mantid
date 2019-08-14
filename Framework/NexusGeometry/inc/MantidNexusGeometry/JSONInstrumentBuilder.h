// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDER_H_
#define MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDER_H_

#include "MantidGeometry/Instrument_fwd.h"
#include "MantidNexusGeometry/DllConfig.h"
#include "MantidNexusGeometry/JSONGeometryParser.h"
#include <string>
#include <vector>

namespace Mantid {
namespace NexusGeometry {
/** JSONInstrumentBuilder : Builds in-memory instrument from json string
 * representing Nexus instrument geometry.
 */
class MANTID_NEXUSGEOMETRY_DLL JSONInstrumentBuilder {
public:
  explicit JSONInstrumentBuilder(const std::string &jsonGeometry);
  ~JSONInstrumentBuilder() = default;

  /// Choppers are not first-class citizens in mantid currently so forward this
  /// on from the parser
  const std::vector<Chopper> &choppers() const;
  Geometry::Instrument_const_uptr buildGeometry() const;

private:
  JSONGeometryParser m_parser;
};

} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDER_H_ */
