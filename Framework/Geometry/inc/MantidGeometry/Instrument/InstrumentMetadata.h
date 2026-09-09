// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "MantidTypes/Core/DateAndTime.h"
#include <string>

namespace Mantid {
namespace Geometry {

/** InstrumentMetadata : metadata related to the entire instrument
 * - valid-from/to dates
 * - source IDF filename
 * - raw IDF XML text
 * - default instrument-view type/axis
 * - physical/neutronic instrument split for indirect-geometry instruments
 * Created at instrument load and read-only thereafter.
 */
class MANTID_GEOMETRY_DLL InstrumentMetadata {
public:
  InstrumentMetadata(const Types::Core::DateAndTime &validFromDate, const Types::Core::DateAndTime &validToDate,
                     std::string filename, std::string xmlText, std::string defaultView, std::string defaultAxis,
                     Instrument_const_sptr physicalInstrument);

  const Types::Core::DateAndTime &validFromDate() const;
  const Types::Core::DateAndTime &validToDate() const;
  const std::string &filename() const;
  const std::string &xmlText() const;
  const std::string &defaultView() const;
  const std::string &defaultAxis() const;
  /// INDIRECT GEOMETRY INSTRUMENTS ONLY : the physical instrument, if one was specified.
  /// This is a legacy Instrument 1.0 object. Replacing it needs its own nested ComponentInfo/DetectorInfo
  /// design (tracked separately).
  Instrument_const_sptr physicalInstrument() const;

private:
  Types::Core::DateAndTime m_validFromDate;
  Types::Core::DateAndTime m_validToDate;
  std::string m_filename;
  std::string m_xmlText;
  std::string m_defaultView;
  std::string m_defaultAxis;
  Instrument_const_sptr m_physicalInstrument;
};

} // namespace Geometry
} // namespace Mantid
