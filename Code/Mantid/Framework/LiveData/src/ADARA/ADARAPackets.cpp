#include "MantidLiveData/ADARA/ADARAPackets.h"

#include <boost/lexical_cast.hpp>
#include <string.h>

using namespace ADARA;

static bool validate_status(uint16_t val) {
  VariableStatus::Enum e = static_cast<VariableStatus::Enum>(val);

  /* No default case so that we get warned when new status values
   * get added.
   */
  switch (e) {
  case VariableStatus::OK:
  case VariableStatus::READ_ERROR:
  case VariableStatus::WRITE_ERROR:
  case VariableStatus::HIHI_LIMIT:
  case VariableStatus::HIGH_LIMIT:
  case VariableStatus::LOLO_LIMIT:
  case VariableStatus::LOW_LIMIT:
  case VariableStatus::BAD_STATE:
  case VariableStatus::CHANGED_STATE:
  case VariableStatus::NO_COMMUNICATION:
  case VariableStatus::COMMUNICATION_TIMEOUT:
  case VariableStatus::HARDWARE_LIMIT:
  case VariableStatus::BAD_CALCULATION:
  case VariableStatus::INVALID_SCAN:
  case VariableStatus::LINK_FAILED:
  case VariableStatus::INVALID_STATE:
  case VariableStatus::BAD_SUBROUTINE:
  case VariableStatus::UNDEFINED_ALARM:
  case VariableStatus::DISABLED:
  case VariableStatus::SIMULATED:
  case VariableStatus::READ_PERMISSION:
  case VariableStatus::WRITE_PERMISSION:
  case VariableStatus::UPSTREAM_DISCONNECTED:
  case VariableStatus::NOT_REPORTED:
    return false;
  }

  return true;
}

static bool validate_severity(uint16_t val) {
  VariableSeverity::Enum e = static_cast<VariableSeverity::Enum>(val);

  /* No default case so that we get warned when new severities get added.
   */
  switch (e) {
  case VariableSeverity::OK:
  case VariableSeverity::MINOR_ALARM:
  case VariableSeverity::MAJOR_ALARM:
  case VariableSeverity::INVALID:
  case VariableSeverity::NOT_REPORTED:
    return false;
  }

  return true;
}

Packet::Packet(const uint8_t *data, uint32_t len)
    : PacketHeader(data), m_data(data), m_len(len), m_allocated(false) {}

Packet::Packet(const Packet &pkt)
    : PacketHeader(pkt.packet()), m_allocated(true) {
  m_data = new uint8_t[pkt.packet_length()];
  m_len = pkt.packet_length();
  memcpy(const_cast<uint8_t *>(m_data), pkt.packet(), m_len);
}

Packet::~Packet() {
  if (m_allocated)
    delete[] m_data;
}

/* ------------------------------------------------------------------------ */

RawDataPkt::RawDataPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields((const uint32_t *)payload()) {
  if (m_payload_len < (6 * sizeof(uint32_t)))
    throw invalid_packet("RawDataPacket is too short");
}

RawDataPkt::RawDataPkt(const RawDataPkt &pkt)
    : Packet(pkt), m_fields((const uint32_t *)payload()) {}

/* ------------------------------------------------------------------------ */

RTDLPkt::RTDLPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields((const uint32_t *)payload()) {
  if (m_payload_len != 120)
    throw invalid_packet("RTDL Packet is incorrect length");

  if ((m_fields[4] >> 24) != 4)
    throw invalid_packet("Missing ring period");
}

RTDLPkt::RTDLPkt(const RTDLPkt &pkt)
    : Packet(pkt), m_fields((const uint32_t *)payload()) {}

/* ------------------------------------------------------------------------ */

SourceListPkt::SourceListPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len) {}

SourceListPkt::SourceListPkt(const SourceListPkt &pkt) : Packet(pkt) {}

/* ------------------------------------------------------------------------ */

BankedEventPkt::BankedEventPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields((const uint32_t *)payload()) {
  if (m_payload_len < (4 * sizeof(uint32_t)))
    throw invalid_packet("BankedEvent packet is too short");

  m_lastFieldIndex = (payload_length() / 4) - 1;
}

BankedEventPkt::BankedEventPkt(const BankedEventPkt &pkt)
    : Packet(pkt), m_fields((const uint32_t *)payload()),
      m_lastFieldIndex((payload_length() / 4) - 1) {}

// The fact that events are wrapped up in banks which are wrapped up in source
// sections is abstracted away (with the exception of checking the COR flag and
// TOF offset fields for each source).  All we've got is firstEvent() and
// nextEvent().  nextEvent() will be smart enough to skip over the source
// section headers and bank headers.

// A packet can have 0 or more sources and each source can have 0 or more
// events.
// That means the only payload we're guarenteed to have is the first 4 fields.
// After that, we've got to start checking against the payload len...
const Event *BankedEventPkt::firstEvent() const {
  m_curEvent = NULL;
  m_curFieldIndex = 4;
  while (m_curEvent == NULL && m_curFieldIndex <= m_lastFieldIndex) {
    // Start of a new source
    firstEventInSource();
  }

  return m_curEvent;
}

const Event *BankedEventPkt::nextEvent() const {
  if (m_curEvent) // If we're null, it's because we've already incremented past
                  // the last event
  {
    m_curEvent = NULL;
    m_curFieldIndex +=
        2; // go to where the next event will start (if there is a next event)

    // have we passed the end of the bank?
    if (m_curFieldIndex < (m_bankStartIndex + 2 + (2 * m_eventCount))) {
      // this is the easy case - the next event is in the current bank
      m_curEvent = (const Event *)&m_fields[m_curFieldIndex];
    } else {
      m_bankNum++;
      while (m_bankNum <= m_bankCount && m_curEvent == NULL) {
        firstEventInBank();
        if (m_curEvent == NULL) {
          // Increment banknum because there were no events in the bank we
          // just tested
          m_bankNum++;
        }
      }

      // If we still haven't found an event, check for more source sections
      while (m_curEvent == NULL && m_curFieldIndex < m_lastFieldIndex) {
        firstEventInSource();
      }
    }
  }

  return m_curEvent;
}

// Helper functions for firstEvent() & nextEvent()

// Assumes m_curFieldIndex points to the start of a source section.
// Sets m_curEvent to the first event in that source (or NULL if
// the source is empty).  Sets m_curFieldIndex pointing at the
// event or at the start of the next source if there were no events.
void BankedEventPkt::firstEventInSource() const {
  m_sourceStartIndex =
      m_curFieldIndex; // index into m_fields for the start of this source
  m_bankCount = m_fields[m_sourceStartIndex + 3];
  if (m_bankCount > 0) {
    // The != 0 comparison avoids a warning on MSVC about performance of forcing
    // a uint32_t to a bool
    m_TOFOffset = ((m_fields[m_sourceStartIndex + 2] & 0x7FFFFFFF) != 0);
    m_isCorrected = ((m_fields[m_sourceStartIndex + 2] & 0x80000000) != 0);
    m_bankNum = 1; // banks are numbered from 1 to m_bankCount.
    m_curFieldIndex = m_sourceStartIndex + 4;

    while (m_bankNum <= m_bankCount && m_curEvent == NULL) {
      firstEventInBank();
      if (m_curEvent == NULL) {
        // Increment banknum because there were no events in the bank we
        // just tested
        m_bankNum++;
      }
    }
  } else // no banks in this source, skip to the next source
  {
    m_curFieldIndex += 4;
    m_curEvent = NULL;
  }
}

// Assumes m_curFieldIndex points at the start of a bank.  Sets m_curEvent
// to the first event in that bank (or NULL if the bank is empty).  Sets
// m_curFieldIndex to the first event if it exists or to the start of the
// next bank if the bank is empty, or to the start of the next source if
// was the last bank.
void BankedEventPkt::firstEventInBank() const {
  m_bankStartIndex =
      m_curFieldIndex; // index into m_fields for the start of this bank
  m_bankId = m_fields[m_bankStartIndex];
  m_eventCount = m_fields[m_bankStartIndex + 1];
  m_curFieldIndex = m_bankStartIndex + 2;
  if (m_eventCount > 0) {
    m_curEvent = (const Event *)&m_fields[m_curFieldIndex];
  } else {
    m_curEvent = NULL;
  }
}

/* ------------------------------------------------------------------------ */

BeamMonitorPkt::BeamMonitorPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields((const uint32_t *)payload()),
      m_sectionStartIndex(0), m_eventNum(0) {
  if (m_payload_len < (4 * sizeof(uint32_t)))
    throw invalid_packet("BeamMonitor packet is too short");
}

BeamMonitorPkt::BeamMonitorPkt(const BeamMonitorPkt &pkt)
    : Packet(pkt), m_fields((const uint32_t *)payload()),
      m_sectionStartIndex(0), m_eventNum(0) {}

#define EVENT_COUNT_MASK 0x003FFFFF // lower 22 bits
bool BeamMonitorPkt::nextSection() const
// Returns true if there is a next section.  False if there isn't.
{
  bool RV = false; // assume we're at the last section
  unsigned newSectionStart;
  if (m_sectionStartIndex == 0) {
    newSectionStart = 4;
  } else {
    unsigned eventCount = m_fields[m_sectionStartIndex] & EVENT_COUNT_MASK;
    newSectionStart = m_sectionStartIndex + 3 + eventCount;
  }

  if ((newSectionStart * 4) < m_payload_len) {
    RV = true;
    m_sectionStartIndex = newSectionStart;
    m_eventNum = 0; // reset the counter for the nextEvent() function
  }

  return RV;
}

uint32_t BeamMonitorPkt::getSectionMonitorID() const {
  // Monitor ID is the upper 10 bits
  return (m_fields[m_sectionStartIndex] >> 22);
}

uint32_t BeamMonitorPkt::getSectionEventCount() const {
  return m_fields[m_sectionStartIndex] & EVENT_COUNT_MASK;
}

uint32_t BeamMonitorPkt::getSectionSourceID() const {
  return m_fields[m_sectionStartIndex + 1];
}

uint32_t BeamMonitorPkt::getSectionTOFOffset() const {
  // need to mask off the high bit
  return m_fields[m_sectionStartIndex + 2] & 0x7FFFFFFF;
}

bool BeamMonitorPkt::sectionTOFCorrected() const {
  // only want the high bit
  return ((m_fields[m_sectionStartIndex + 2] & 0x80000000) != 0);
}

#define CYCLE_MASK 0x7FE00000 // bits 30 to 21 (inclusive)
#define TOF_MASK 0x001FFFFF   // bits 20 to 0 (inclusive)
bool BeamMonitorPkt::nextEvent(bool &risingEdge, uint32_t &cycle,
                               uint32_t &tof) const {
  bool RV = false;
  if (m_sectionStartIndex != 0 && m_eventNum < getSectionEventCount()) {
    uint32_t rawEvent = m_fields[m_sectionStartIndex + 3 + m_eventNum];

    risingEdge = ((rawEvent & 0x80000000) != 0);
    cycle = (rawEvent & CYCLE_MASK) >> 21;
    tof = (rawEvent & TOF_MASK);

    m_eventNum++;
    RV = true;
  }

  return RV;
}

/* ------------------------------------------------------------------------ */

PixelMappingPkt::PixelMappingPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len) {}

PixelMappingPkt::PixelMappingPkt(const PixelMappingPkt &pkt) : Packet(pkt) {}

/* ------------------------------------------------------------------------ */

RunStatusPkt::RunStatusPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields((const uint32_t *)payload()) {
  if (m_payload_len != (3 * sizeof(uint32_t)))
    throw invalid_packet("RunStatus packet is incorrect size");
}

RunStatusPkt::RunStatusPkt(const RunStatusPkt &pkt)
    : Packet(pkt), m_fields((const uint32_t *)payload()) {}

/* ------------------------------------------------------------------------ */

RunInfoPkt::RunInfoPkt(const uint8_t *data, uint32_t len) : Packet(data, len) {
  uint32_t size = *(const uint32_t *)payload();
  const char *xml = (const char *)payload() + sizeof(uint32_t);

  if (m_payload_len < sizeof(uint32_t))
    throw invalid_packet("RunInfo packet is too short");
  if (m_payload_len < (size + sizeof(uint32_t)))
    throw invalid_packet("RunInfo packet has oversize string");

  /* TODO it would be better to create the string on access
   * rather than object construction; the user may not care.
   */
  m_xml.assign(xml, size);
}

RunInfoPkt::RunInfoPkt(const RunInfoPkt &pkt) : Packet(pkt), m_xml(pkt.m_xml) {}

/* ------------------------------------------------------------------------ */

TransCompletePkt::TransCompletePkt(const uint8_t *data, uint32_t len)
    : Packet(data, len) {
  uint32_t size = *(const uint32_t *)payload();
  const char *reason = (const char *)payload() + sizeof(uint32_t);

  m_status = (uint16_t)(size >> 16);
  size &= 0xffff;
  if (m_payload_len < sizeof(uint32_t))
    throw invalid_packet("TransComplete packet is too short");
  if (m_payload_len < (size + sizeof(uint32_t)))
    throw invalid_packet("TransComplete packet has oversize "
                         "string");

  /* TODO it would be better to create the string on access
   * rather than object construction; the user may not care.
   */
  m_reason.assign(reason, size);
}

TransCompletePkt::TransCompletePkt(const TransCompletePkt &pkt)
    : Packet(pkt), m_reason(pkt.m_reason) {}

/* ------------------------------------------------------------------------ */

ClientHelloPkt::ClientHelloPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len) {
  if (m_payload_len != sizeof(uint32_t))
    throw invalid_packet("ClientHello packet is incorrect size");

  m_reqStart = *(const uint32_t *)payload();
}

ClientHelloPkt::ClientHelloPkt(const ClientHelloPkt &pkt)
    : Packet(pkt), m_reqStart(pkt.m_reqStart) {}

/* ------------------------------------------------------------------------ */

AnnotationPkt::AnnotationPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields((const uint32_t *)payload()) {
  if (m_payload_len < (2 * sizeof(uint32_t)))
    throw invalid_packet("AnnotationPkt packet is incorrect size");

  uint16_t size = m_fields[0] & 0xffff;
  if (m_payload_len < (size + (2 * sizeof(uint32_t))))
    throw invalid_packet("AnnotationPkt packet has oversize "
                         "string");
}

AnnotationPkt::AnnotationPkt(const AnnotationPkt &pkt)
    : Packet(pkt), m_fields((const uint32_t *)payload()) {}

/* ------------------------------------------------------------------------ */

SyncPkt::SyncPkt(const uint8_t *data, uint32_t len) : Packet(data, len) {
  uint32_t size = *(const uint32_t *)(payload() + 24);

  if (m_payload_len < 28)
    throw invalid_packet("Sync packet is too small");
  if (m_payload_len < (size + 28))
    throw invalid_packet("Sync packet has oversize string");
}

SyncPkt::SyncPkt(const SyncPkt &pkt) : Packet(pkt) {}

/* ------------------------------------------------------------------------ */

HeartbeatPkt::HeartbeatPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len) {
  if (m_payload_len)
    throw invalid_packet("Heartbeat packet is incorrect size");
}

HeartbeatPkt::HeartbeatPkt(const HeartbeatPkt &pkt) : Packet(pkt) {}

/* ------------------------------------------------------------------------ */

GeometryPkt::GeometryPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len) {
  uint32_t size = *(const uint32_t *)payload();
  const char *xml = (const char *)payload() + sizeof(uint32_t);

  if (m_payload_len < sizeof(uint32_t))
    throw invalid_packet("Geometry packet is too short");
  if (m_payload_len < (size + sizeof(uint32_t)))
    throw invalid_packet("Geometry packet has oversize string");

  /* TODO it would be better to create the string on access
   * rather than object construction; the user may not care.
   */
  m_xml.assign(xml, size);
}

GeometryPkt::GeometryPkt(const GeometryPkt &pkt)
    : Packet(pkt), m_xml(pkt.m_xml) {}

/* ------------------------------------------------------------------------ */

BeamlineInfoPkt::BeamlineInfoPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len) {
  const char *info = (const char *)payload() + sizeof(uint32_t);
  uint32_t sizes = *(const uint32_t *)payload();
  uint32_t id_len, shortName_len, longName_len, info_len;

  if (m_payload_len < sizeof(uint32_t))
    throw invalid_packet("Beamline info packet is too short");

  longName_len = sizes & 0xff;
  shortName_len = (sizes >> 8) & 0xff;
  id_len = (sizes >> 16) & 0xff;

  info_len = id_len + shortName_len + longName_len;

  if (m_payload_len < (info_len + sizeof(uint32_t)))
    throw invalid_packet("Beamline info packet has undersize data");

  m_id.assign(info, id_len);
  info += id_len;
  m_shortName.assign(info, shortName_len);
  info += shortName_len;
  m_longName.assign(info, longName_len);
}

BeamlineInfoPkt::BeamlineInfoPkt(const BeamlineInfoPkt &pkt)
    : Packet(pkt), m_id(pkt.m_id), m_shortName(pkt.m_shortName),
      m_longName(pkt.m_longName) {}

/* ------------------------------------------------------------------------ */

DeviceDescriptorPkt::DeviceDescriptorPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len) {
  const uint32_t *fields = (const uint32_t *)payload();
  uint32_t size;

  if (m_payload_len < (2 * sizeof(uint32_t)))
    throw invalid_packet("DeviceDescriptor packet is too short");
  size = fields[1];
  if (m_payload_len < (size + (2 * sizeof(uint32_t))))
    throw invalid_packet("DeviceDescriptor packet has oversize "
                         "string");

  /* TODO it would be better to create the string on access
   * rather than object construction; the user may not care.
   */
  m_devId = fields[0];
  m_desc.assign((const char *)&fields[2], size);
}

DeviceDescriptorPkt::DeviceDescriptorPkt(const DeviceDescriptorPkt &pkt)
    : Packet(pkt), m_devId(pkt.m_devId), m_desc(pkt.m_desc) {}

/* ------------------------------------------------------------------------ */

VariableU32Pkt::VariableU32Pkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields((const uint32_t *)payload()) {
  if (m_payload_len != (4 * sizeof(uint32_t))) {
    std::string msg("VariableValue (U32) packet is incorrect "
                    "length: ");
    msg += boost::lexical_cast<std::string>(m_payload_len);
    throw invalid_packet(msg);
  }
  if (validate_status(status())) {
    std::string msg("VariableValue (U32) packet has invalid "
                    "status: ");
    msg += boost::lexical_cast<std::string>(status());
    throw invalid_packet(msg);
  }
  if (validate_severity(severity())) {
    std::string msg("VariableValue (U32) packet has invalid "
                    "severity: ");
    msg += boost::lexical_cast<std::string>(severity());
    throw invalid_packet(msg);
  }
}

VariableU32Pkt::VariableU32Pkt(const VariableU32Pkt &pkt)
    : Packet(pkt), m_fields((const uint32_t *)payload()) {}

/* ------------------------------------------------------------------------ */

VariableDoublePkt::VariableDoublePkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields((const uint32_t *)payload()) {
  if (m_payload_len != (sizeof(double) + (3 * sizeof(uint32_t)))) {
    std::string msg("VariableValue (double) packet is incorrect "
                    "length: ");
    msg += boost::lexical_cast<std::string>(m_payload_len);
    throw invalid_packet(msg);
  }
  if (validate_status(status())) {
    std::string msg("VariableValue (double) packet has invalid "
                    "status: ");
    msg += boost::lexical_cast<std::string>(status());
    throw invalid_packet(msg);
  }
  if (validate_severity(severity())) {
    std::string msg("VariableValue (double) packet has invalid "
                    "severity: ");
    msg += boost::lexical_cast<std::string>(severity());
    throw invalid_packet(msg);
  }
}

VariableDoublePkt::VariableDoublePkt(const VariableDoublePkt &pkt)
    : Packet(pkt), m_fields((const uint32_t *)payload()) {}

/* ------------------------------------------------------------------------ */

VariableStringPkt::VariableStringPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields((const uint32_t *)payload()) {
  uint32_t size;

  if (m_payload_len < (4 * sizeof(uint32_t))) {
    std::string msg("VariableValue (string) packet is too short ");
    msg += boost::lexical_cast<std::string>(m_payload_len);
    throw invalid_packet(msg);
  }
  size = m_fields[3];
  if (m_payload_len < (size + (2 * sizeof(uint32_t)))) {
    std::string msg("VariableValue (string) packet has oversize "
                    "string: ");
    msg += boost::lexical_cast<std::string>(size);
    msg += " vs payload ";
    msg += boost::lexical_cast<std::string>(m_payload_len);
    throw invalid_packet(msg);
  }

  if (validate_status(status())) {
    std::string msg("VariableValue (string) packet has invalid "
                    "status: ");
    msg += boost::lexical_cast<std::string>(status());
    throw invalid_packet(msg);
  }
  if (validate_severity(severity())) {
    std::string msg("VariableValue (string) packet has invalid "
                    "severity: ");
    msg += boost::lexical_cast<std::string>(severity());
    throw invalid_packet(msg);
  }

  /* TODO it would be better to create the string on access
   * rather than object construction; the user may not care.
   */
  m_val.assign((const char *)&m_fields[4], size);
}

VariableStringPkt::VariableStringPkt(const VariableStringPkt &pkt)
    : Packet(pkt), m_fields((const uint32_t *)payload()), m_val(pkt.m_val) {}
