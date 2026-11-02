// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/InstrumentMetadata.h"

namespace Mantid::Geometry {

InstrumentMetadata::InstrumentMetadata(const Types::Core::DateAndTime &validFromDate,
                                       const Types::Core::DateAndTime &validToDate, const std::string &filename,
                                       const std::string &xmlText, const std::string &defaultView,
                                       const std::string &defaultAxis, const Instrument_const_sptr &physicalInstrument)
    : m_validFromDate(validFromDate), m_validToDate(validToDate), m_filename(filename), m_xmlText(xmlText),
      m_defaultView(defaultView), m_defaultAxis(defaultAxis), m_physicalInstrument(physicalInstrument) {}

const Types::Core::DateAndTime &InstrumentMetadata::validFromDate() const { return m_validFromDate; }

const Types::Core::DateAndTime &InstrumentMetadata::validToDate() const { return m_validToDate; }

const std::string &InstrumentMetadata::filename() const { return m_filename; }

const std::string &InstrumentMetadata::xmlText() const { return m_xmlText; }

const std::string &InstrumentMetadata::defaultView() const { return m_defaultView; }

const std::string &InstrumentMetadata::defaultAxis() const { return m_defaultAxis; }

Instrument_const_sptr InstrumentMetadata::physicalInstrument() const { return m_physicalInstrument; }

void InstrumentMetadata::setFileName(const std::string &filename) { m_filename = filename; }

void InstrumentMetadata::setXmlText(const std::string &xmlText) { m_xmlText = xmlText; }

void InstrumentMetadata::setDefaultView(const std::string &defaultView) { m_defaultView = defaultView; }

void InstrumentMetadata::setDefaultViewAxis(const std::string &defaultAxis) { m_defaultAxis = defaultAxis; }

} // namespace Mantid::Geometry
