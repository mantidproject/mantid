// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidTypes/Core/DateAndTime.h"
#include <memory>
#include <string>

namespace Mantid {
namespace Geometry {
class ComponentInfo;
class DetectorInfo;

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
                     std::shared_ptr<const ComponentInfo> physicalComponentInfo = nullptr,
                     std::shared_ptr<const DetectorInfo> physicalDetectorInfo = nullptr);

  const Types::Core::DateAndTime &validFromDate() const;
  const Types::Core::DateAndTime &validToDate() const;
  const std::string &filename() const;
  const std::string &xmlText() const;
  const std::string &defaultView() const;
  const std::string &defaultAxis() const;
  /// INDIRECT GEOMETRY INSTRUMENTS ONLY : true if a physical instrument was specified,
  /// distinct from the neutronic one held by the workspace.
  bool hasPhysicalInstrument() const;
  /// INDIRECT GEOMETRY INSTRUMENTS ONLY : the ComponentInfo of the physical instrument.
  /// Throws if there is no physical instrument.
  const ComponentInfo &physicalComponentInfo() const;
  /// INDIRECT GEOMETRY INSTRUMENTS ONLY : the DetectorInfo of the physical instrument.
  /// Throws if there is no physical instrument.
  const DetectorInfo &physicalDetectorInfo() const;

private:
  Types::Core::DateAndTime m_validFromDate;
  Types::Core::DateAndTime m_validToDate;
  std::string m_filename;
  std::string m_xmlText;
  std::string m_defaultView;
  std::string m_defaultAxis;
  // The ComponentInfo is cross-linked to the DetectorInfo, so the two are always held together
  std::shared_ptr<const ComponentInfo> m_physicalComponentInfo;
  std::shared_ptr<const DetectorInfo> m_physicalDetectorInfo;
};

} // namespace Geometry
} // namespace Mantid
