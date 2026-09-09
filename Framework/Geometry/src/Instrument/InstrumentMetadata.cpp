// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/InstrumentMetadata.h"

namespace Mantid::Geometry {

InstrumentMetadata::InstrumentMetadata(const Types::Core::DateAndTime &validFromDate,
                                       const Types::Core::DateAndTime &validToDate, std::string filename,
                                       std::string xmlText, std::string defaultView, std::string defaultAxis,
                                       Instrument_const_sptr physicalInstrument)
    : m_validFromDate(validFromDate), m_validToDate(validToDate), m_filename(std::move(filename)),
      m_xmlText(std::move(xmlText)), m_defaultView(std::move(defaultView)), m_defaultAxis(std::move(defaultAxis)),
      m_physicalInstrument(std::move(physicalInstrument)) {}

const Types::Core::DateAndTime &InstrumentMetadata::validFromDate() const { return m_validFromDate; }

const Types::Core::DateAndTime &InstrumentMetadata::validToDate() const { return m_validToDate; }

const std::string &InstrumentMetadata::filename() const { return m_filename; }

const std::string &InstrumentMetadata::xmlText() const { return m_xmlText; }

const std::string &InstrumentMetadata::defaultView() const { return m_defaultView; }

const std::string &InstrumentMetadata::defaultAxis() const { return m_defaultAxis; }

Instrument_const_sptr InstrumentMetadata::physicalInstrument() const { return m_physicalInstrument; }

} // namespace Mantid::Geometry
