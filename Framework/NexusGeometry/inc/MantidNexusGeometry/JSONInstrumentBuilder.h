// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Instrument_fwd.h"
#include "MantidNexusGeometry/DllConfig.h"
#include <memory>
#include <string>
#include <vector>

namespace Mantid {
namespace NexusGeometry {
class JSONGeometryParser;
struct Chopper;
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
  std::unique_ptr<JSONGeometryParser> m_parser;
};

} // namespace NexusGeometry
} // namespace Mantid
