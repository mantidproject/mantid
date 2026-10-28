// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/InstrumentMetadata.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

#include <stdexcept>

namespace Mantid::Geometry {

InstrumentMetadata::InstrumentMetadata(const Types::Core::DateAndTime &validFromDate,
                                       const Types::Core::DateAndTime &validToDate, std::string filename,
                                       std::string xmlText, std::string defaultView, std::string defaultAxis,
                                       std::shared_ptr<const ComponentInfo> physicalComponentInfo,
                                       std::shared_ptr<const DetectorInfo> physicalDetectorInfo)
    : m_validFromDate(validFromDate), m_validToDate(validToDate), m_filename(std::move(filename)),
      m_xmlText(std::move(xmlText)), m_defaultView(std::move(defaultView)), m_defaultAxis(std::move(defaultAxis)),
      m_physicalComponentInfo(std::move(physicalComponentInfo)),
      m_physicalDetectorInfo(std::move(physicalDetectorInfo)) {
  if (static_cast<bool>(m_physicalComponentInfo) != static_cast<bool>(m_physicalDetectorInfo)) {
    throw std::invalid_argument(
        "InstrumentMetadata: the physical ComponentInfo and DetectorInfo must be given together");
  }
}

const Types::Core::DateAndTime &InstrumentMetadata::validFromDate() const { return m_validFromDate; }

const Types::Core::DateAndTime &InstrumentMetadata::validToDate() const { return m_validToDate; }

const std::string &InstrumentMetadata::filename() const { return m_filename; }

const std::string &InstrumentMetadata::xmlText() const { return m_xmlText; }

const std::string &InstrumentMetadata::defaultView() const { return m_defaultView; }

const std::string &InstrumentMetadata::defaultAxis() const { return m_defaultAxis; }

bool InstrumentMetadata::hasPhysicalInstrument() const { return static_cast<bool>(m_physicalComponentInfo); }

const ComponentInfo &InstrumentMetadata::physicalComponentInfo() const {
  if (!m_physicalComponentInfo) {
    throw std::runtime_error("Cannot return reference to NULL physical ComponentInfo");
  }
  return *m_physicalComponentInfo;
}

const DetectorInfo &InstrumentMetadata::physicalDetectorInfo() const {
  if (!m_physicalDetectorInfo) {
    throw std::runtime_error("Cannot return reference to NULL physical DetectorInfo");
  }
  return *m_physicalDetectorInfo;
}

} // namespace Mantid::Geometry
