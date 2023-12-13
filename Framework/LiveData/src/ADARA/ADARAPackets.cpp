// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/ADARA/ADARAPackets.h"

#include <cstring>

using namespace ADARA;

static bool validate_status(uint16_t val) {
  auto e = static_cast<VariableStatus::Enum>(val);

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
  auto e = static_cast<VariableSeverity::Enum>(val);

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

Packet::Packet(const uint8_t *data, uint32_t len) : PacketHeader(data), m_data(data), m_len(len), m_allocated(false) {}

Packet::Packet(const Packet &pkt) : PacketHeader(pkt.packet()), m_allocated(true) {
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
    : Packet(data, len), m_fields(reinterpret_cast<const uint32_t *>(payload())) {
  if (m_version == 0x00 && m_payload_len < (6 * sizeof(uint32_t)))
    throw invalid_packet("RawDataPacket V0 is too short");
  else if (m_version > ADARA::PacketType::RAW_EVENT_VERSION && m_payload_len < (6 * sizeof(uint32_t)))
    throw invalid_packet("Newer RawDataPacket is too short");
}

RawDataPkt::RawDataPkt(const RawDataPkt &pkt) : Packet(pkt), m_fields(reinterpret_cast<const uint32_t *>(payload())) {}

/* ------------------------------------------------------------------------ */

MappedDataPkt::MappedDataPkt(const uint8_t *data, uint32_t len) : RawDataPkt(data, len) {
  if (m_version == 0x00 && m_payload_len < (6 * sizeof(uint32_t)))
    throw invalid_packet("MappedDataPacket V0 is too short");
  else if (m_version > ADARA::PacketType::MAPPED_EVENT_VERSION && m_payload_len < (6 * sizeof(uint32_t)))
    throw invalid_packet("Newer MappedDataPacket is too short");
}

/* ------------------------------------------------------------------------ */

RTDLPkt::RTDLPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields(reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(payload())))
// Note: RTDLPkt m_fields can't be "const", as we Modify Pulse Charge!
{
  if (m_version == 0x00 && m_payload_len != 120)
    throw invalid_packet("RTDL V0 Packet is incorrect length");
  else if (m_version > ADARA::PacketType::RTDL_VERSION && m_payload_len < 120)
    throw invalid_packet("Newer RTDL Packet is too short");

  if ((m_fields[4] >> 24) != 4)
    throw invalid_packet("Missing ring period");
}

RTDLPkt::RTDLPkt(const RTDLPkt &pkt)
    : Packet(pkt), m_fields(reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(payload())))
// Note: RTDLPkt m_fields can't be "const", as we Modify Pulse Charge!
{}

/* ------------------------------------------------------------------------ */

SourceListPkt::SourceListPkt(const uint8_t *data, uint32_t len) : Packet(data, len) {}

/* ------------------------------------------------------------------------ */

BankedEventPkt::BankedEventPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields(reinterpret_cast<const uint32_t *>(payload())), m_curEvent(nullptr),
      m_lastFieldIndex(0), m_curFieldIndex(0), m_sourceStartIndex(0), m_bankCount(0), m_TOFOffset(0),
      m_isCorrected(false), m_bankNum(0), m_bankStartIndex(0), m_bankId(0), m_eventCount(0) {
  if (m_version == 0x00 && m_payload_len < (4 * sizeof(uint32_t)))
    throw invalid_packet("BankedEvent V0 packet is too short");
  else if (m_version == 0x01 && m_payload_len < (4 * sizeof(uint32_t)))
    throw invalid_packet("BankedEvent V1 packet is too short");
  else if (m_version > ADARA::PacketType::BANKED_EVENT_VERSION && m_payload_len < (4 * sizeof(uint32_t)))
    throw invalid_packet("Newer BankedEvent packet is too short");

  m_lastFieldIndex = (payload_length() / 4) - 1;
}

BankedEventPkt::BankedEventPkt(const BankedEventPkt &pkt)
    : Packet(pkt), m_fields(reinterpret_cast<const uint32_t *>(payload())), m_curEvent(nullptr), m_lastFieldIndex(0),
      m_curFieldIndex(0), m_sourceStartIndex(0), m_bankCount(0), m_TOFOffset(0), m_isCorrected(false), m_bankNum(0),
      m_bankStartIndex(0), m_bankId(0), m_eventCount(0) {
  m_lastFieldIndex = ((payload_length() / 4) - 1);
}

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
  m_curEvent = nullptr;
  m_curFieldIndex = 4;
  while (m_curEvent == nullptr && m_curFieldIndex <= m_lastFieldIndex) {
    // Start of a new source
    firstEventInSource();
  }

  return m_curEvent;
}

const Event *BankedEventPkt::nextEvent() const {
  if (m_curEvent) // If we're null, it's because we've already incremented past
                  // the last event
  {
    m_curEvent = nullptr;
    m_curFieldIndex += 2; // go to where the next event will start (if there is a next event)

    // have we passed the end of the bank?
    if (m_curFieldIndex < (m_bankStartIndex + 2 + (2 * m_eventCount))) {
      // this is the easy case - the next event is in the current bank
      m_curEvent = reinterpret_cast<const Event *>(&m_fields[m_curFieldIndex]);
    } else {
      m_bankNum++;
      while (m_bankNum <= m_bankCount && m_curEvent == nullptr) {
        firstEventInBank();
        if (m_curEvent == nullptr) {
          // Increment banknum because there were no events in the bank we
          // just tested
          m_bankNum++;
        }
      }

      // If we still haven't found an event, check for more source sections
      while (m_curEvent == nullptr && m_curFieldIndex < m_lastFieldIndex) {
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
  m_sourceStartIndex = m_curFieldIndex; // index into m_fields for the start of this source
  m_bankCount = m_fields[m_sourceStartIndex + 3];
  if (m_bankCount > 0) {
    // The != 0 comparison avoids a warning on MSVC about performance of forcing
    // a uint32_t to a bool
    m_TOFOffset = ((m_fields[m_sourceStartIndex + 2] & 0x7FFFFFFF) != 0);
    m_isCorrected = ((m_fields[m_sourceStartIndex + 2] & 0x80000000) != 0);
    m_bankNum = 1; // banks are numbered from 1 to m_bankCount.
    m_curFieldIndex = m_sourceStartIndex + 4;

    while (m_bankNum <= m_bankCount && m_curEvent == nullptr) {
      firstEventInBank();
      if (m_curEvent == nullptr) {
        // Increment banknum because there were no events in the bank we
        // just tested
        m_bankNum++;
      }
    }
  } else // no banks in this source, skip to the next source
  {
    m_curFieldIndex += 4;
    m_curEvent = nullptr;
  }
}

// Assumes m_curFieldIndex points at the start of a bank.  Sets m_curEvent
// to the first event in that bank (or NULL if the bank is empty).  Sets
// m_curFieldIndex to the first event if it exists or to the start of the
// next bank if the bank is empty, or to the start of the next source if
// was the last bank.
void BankedEventPkt::firstEventInBank() const {
  m_bankStartIndex = m_curFieldIndex; // index into m_fields for the start of this bank
  m_bankId = m_fields[m_bankStartIndex];
  m_eventCount = m_fields[m_bankStartIndex + 1];
  m_curFieldIndex = m_bankStartIndex + 2;
  if (m_eventCount > 0) {
    m_curEvent = reinterpret_cast<const Event *>(&m_fields[m_curFieldIndex]);
  } else {
    m_curEvent = nullptr;
  }
}

/* ------------------------------------------------------------------------ */

BeamMonitorPkt::BeamMonitorPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields(reinterpret_cast<const uint32_t *>(payload())), m_sectionStartIndex(0),
      m_eventNum(0) {
  if (m_version == 0x00 && m_payload_len < (4 * sizeof(uint32_t)))
    throw invalid_packet("BeamMonitor V0 packet is too short");
  else if (m_version == 0x01 && m_payload_len < (4 * sizeof(uint32_t)))
    throw invalid_packet("BeamMonitor V1 packet is too short");
  else if (m_version > ADARA::PacketType::BEAM_MONITOR_EVENT_VERSION && m_payload_len < (4 * sizeof(uint32_t)))
    throw invalid_packet("Newer BeamMonitor packet is too short");
}

BeamMonitorPkt::BeamMonitorPkt(const BeamMonitorPkt &pkt)
    : Packet(pkt), m_fields(reinterpret_cast<const uint32_t *>(payload())), m_sectionStartIndex(0), m_eventNum(0) {}

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

uint32_t BeamMonitorPkt::getSectionEventCount() const { return m_fields[m_sectionStartIndex] & EVENT_COUNT_MASK; }

uint32_t BeamMonitorPkt::getSectionSourceID() const { return m_fields[m_sectionStartIndex + 1]; }

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
bool BeamMonitorPkt::nextEvent(bool &risingEdge, uint32_t &cycle, uint32_t &tof) const {
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

PixelMappingPkt::PixelMappingPkt(const uint8_t *data, uint32_t len) : Packet(data, len) {}

/* ------------------------------------------------------------------------ */

RunStatusPkt::RunStatusPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields(reinterpret_cast<const uint32_t *>(payload())) {
  if (m_version == 0x00 && m_payload_len != (3 * sizeof(uint32_t)))
    throw invalid_packet("RunStatus V0 packet is incorrect length");
  else if (m_version > ADARA::PacketType::RUN_STATUS_VERSION && m_payload_len < (3 * sizeof(uint32_t)))
    throw invalid_packet("Newer RunStatus packet is too short");
}

RunStatusPkt::RunStatusPkt(const RunStatusPkt &pkt)
    : Packet(pkt), m_fields(reinterpret_cast<const uint32_t *>(payload())) {}

/* ------------------------------------------------------------------------ */

RunInfoPkt::RunInfoPkt(const uint8_t *data, uint32_t len) : Packet(data, len) {
  uint32_t size = *reinterpret_cast<const uint32_t *>(payload());
  const char *xml = reinterpret_cast<const char *>(payload()) + sizeof(uint32_t);

  if (m_version == 0x00 && m_payload_len < sizeof(uint32_t))
    throw invalid_packet("RunInfo V0 packet is too short");
  else if (m_version > ADARA::PacketType::RUN_INFO_VERSION && m_payload_len < sizeof(uint32_t))
    throw invalid_packet("Newer RunInfo packet is too short");

  if (m_version == 0x00 && m_payload_len < (size + sizeof(uint32_t)))
    throw invalid_packet("RunInfo V0 packet has oversize string");
  else if (m_version > ADARA::PacketType::RUN_INFO_VERSION && m_payload_len < (size + sizeof(uint32_t)))
    throw invalid_packet("Newer RunInfo packet has oversize string");

  /* TODO it would be better to create the string on access
   * rather than object construction; the user may not care.
   */
  m_xml.assign(xml, size);
}

/* ------------------------------------------------------------------------ */

TransCompletePkt::TransCompletePkt(const uint8_t *data, uint32_t len) : Packet(data, len) {
  uint32_t size = *reinterpret_cast<const uint32_t *>(payload());
  const char *reasonStr = reinterpret_cast<const char *>(payload()) + sizeof(uint32_t);

  if (m_version == 0x00 && m_payload_len < sizeof(uint32_t))
    throw invalid_packet("TransComplete V0 packet is too short");
  else if (m_version > ADARA::PacketType::TRANS_COMPLETE_VERSION && m_payload_len < sizeof(uint32_t))
    throw invalid_packet("Newer TransComplete packet is too short");

  m_status = static_cast<uint16_t>(size >> 16);
  size &= 0xffff;
  if (m_version == 0x00 && m_payload_len < (size + sizeof(uint32_t)))
    throw invalid_packet("TransComplete V0 packet has oversize string");
  else if (m_version > ADARA::PacketType::TRANS_COMPLETE_VERSION && m_payload_len < (size + sizeof(uint32_t))) {
    throw invalid_packet("Newer TransComplete packet has oversize string");
  }

  /* TODO it would be better to create the string on access
   * rather than object construction; the user may not care.
   */
  m_reason.assign(reasonStr, size);
}

TransCompletePkt::TransCompletePkt(const TransCompletePkt &pkt)
    : Packet(pkt), m_status(VariableStatus::NOT_REPORTED), m_reason(pkt.m_reason) {}

/* ------------------------------------------------------------------------ */

ClientHelloPkt::ClientHelloPkt(const uint8_t *data, uint32_t len) : Packet(data, len) {
  if (m_version == 0x00 && m_payload_len != sizeof(uint32_t))
    throw invalid_packet("ClientHello V0 packet is incorrect size");
  else if (m_version == 0x01 && m_payload_len != (2 * sizeof(uint32_t)))
    throw invalid_packet("ClientHello V1 packet is incorrect size");
  else if (m_version > ADARA::PacketType::CLIENT_HELLO_VERSION && m_payload_len < (2 * sizeof(uint32_t)))
    throw invalid_packet("Newer ClientHello packet is too short");

  const auto *fields = reinterpret_cast<const uint32_t *>(payload());
  m_reqStart = fields[0];
  m_clientFlags = (m_version > 0) ? fields[1] : 0;
}

/* ------------------------------------------------------------------------ */

AnnotationPkt::AnnotationPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields(reinterpret_cast<const uint32_t *>(payload())) {
  if (m_version == 0x00 && m_payload_len < (2 * sizeof(uint32_t)))
    throw invalid_packet("AnnotationPkt V0 packet is incorrect size");
  else if (m_version > ADARA::PacketType::STREAM_ANNOTATION_VERSION && m_payload_len < (2 * sizeof(uint32_t))) {
    throw invalid_packet("Newer AnnotationPkt packet is incorrect size");
  }

  uint16_t size = m_fields[0] & 0xffff;
  if (m_version == 0x00 && m_payload_len < (size + (2 * sizeof(uint32_t))))
    throw invalid_packet("AnnotationPkt V0 packet has oversize string");
  else if (m_version > ADARA::PacketType::STREAM_ANNOTATION_VERSION &&
           m_payload_len < (size + (2 * sizeof(uint32_t)))) {
    throw invalid_packet("Newer AnnotationPkt packet has oversize string");
  }
}

AnnotationPkt::AnnotationPkt(const AnnotationPkt &pkt)
    : Packet(pkt), m_fields(reinterpret_cast<const uint32_t *>(payload())) {}

/* ------------------------------------------------------------------------ */

SyncPkt::SyncPkt(const uint8_t *data, uint32_t len) : Packet(data, len) {
  uint32_t size = *reinterpret_cast<const uint32_t *>(payload() + 24);

  if (m_version == 0x00 && m_payload_len < 28)
    throw invalid_packet("Sync V0 packet is too small");
  else if (m_version > ADARA::PacketType::SYNC_VERSION && m_payload_len < 28)
    throw invalid_packet("Newer Sync packet is too small");

  if (m_version == 0x00 && m_payload_len < (size + 28))
    throw invalid_packet("Sync V0 packet has oversize string");
  else if (m_version > ADARA::PacketType::SYNC_VERSION && m_payload_len < (size + 28))
    throw invalid_packet("Newer Sync packet has oversize string");
}

/* ------------------------------------------------------------------------ */

HeartbeatPkt::HeartbeatPkt(const uint8_t *data, uint32_t len) : Packet(data, len) {
  if (m_version == 0x00 && m_payload_len != 0)
    throw invalid_packet("Heartbeat V0 packet is incorrect size");
  else if (m_version > ADARA::PacketType::HEARTBEAT_VERSION && m_payload_len == 0)
    throw invalid_packet("Newer ClientHello packet is too short");
  // ...any newer version would have to grow, lol...? ;-D
}

/* ------------------------------------------------------------------------ */

GeometryPkt::GeometryPkt(const uint8_t *data, uint32_t len) : Packet(data, len) {
  uint32_t size = *reinterpret_cast<const uint32_t *>(payload());
  const char *xml = reinterpret_cast<const char *>(payload()) + sizeof(uint32_t);

  if (m_version == 0x00 && m_payload_len < sizeof(uint32_t))
    throw invalid_packet("Geometry V0 packet is too short");
  else if (m_version > ADARA::PacketType::GEOMETRY_VERSION && m_payload_len < sizeof(uint32_t))
    throw invalid_packet("Newer Geometry packet is too short");

  if (m_version == 0x00 && m_payload_len < (size + sizeof(uint32_t)))
    throw invalid_packet("Geometry V0 packet has oversize string");
  else if (m_version > ADARA::PacketType::GEOMETRY_VERSION && m_payload_len < (size + sizeof(uint32_t)))
    throw invalid_packet("Newer Geometry packet has oversize string");

  /* TODO it would be better to create the string on access
   * rather than object construction; the user may not care.
   */
  m_xml.assign(xml, size);
}

/* ------------------------------------------------------------------------ */

BeamlineInfoPkt::BeamlineInfoPkt(const uint8_t *data, uint32_t len) : Packet(data, len) {
  const char *info = reinterpret_cast<const char *>(payload()) + sizeof(uint32_t);
  uint32_t sizes = *reinterpret_cast<const uint32_t *>(payload());
  uint32_t id_len, shortName_len, longName_len, info_len;

  if (m_version == 0x00 && m_payload_len < sizeof(uint32_t))
    throw invalid_packet("Beamline Info V0 packet is too short");
  else if (m_version == 0x01 && m_payload_len < sizeof(uint32_t))
    throw invalid_packet("Beamline Info V1 packet is too short");
  else if (m_version > ADARA::PacketType::BEAMLINE_INFO_VERSION && m_payload_len < sizeof(uint32_t))
    throw invalid_packet("Newer Beamline Info packet is too short");

  longName_len = sizes & 0xff;
  shortName_len = (sizes >> 8) & 0xff;
  id_len = (sizes >> 16) & 0xff;
  m_targetStationNumber = (sizes >> 24) & 0xff; // formerly "Unused" in V0...

  // Unspecified (Version 0 Packet) Target Number Defaults to 1.
  if (m_targetStationNumber == 0)
    m_targetStationNumber = 1;

  info_len = id_len + shortName_len + longName_len;

  if (m_payload_len < (info_len + sizeof(uint32_t)))
    throw invalid_packet("Beamline info packet has undersize data");

  m_id.assign(info, id_len);
  info += id_len;
  m_shortName.assign(info, shortName_len);
  info += shortName_len;
  m_longName.assign(info, longName_len);
}

/* ------------------------------------------------------------------------ */

BeamMonitorConfigPkt::BeamMonitorConfigPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields(reinterpret_cast<const uint32_t *>(payload())) {
  size_t sectionSize = sizeof(double) + (4 * sizeof(uint32_t));

  if (m_version == 0x00 && m_payload_len != (sizeof(uint32_t) + (beamMonCount() * sectionSize))) {
    std::string msg("BeamMonitorConfig V0 packet is incorrect length: ");
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  } else if (m_version > ADARA::PacketType::BEAM_MONITOR_CONFIG_VERSION &&
             m_payload_len < (sizeof(uint32_t) + (beamMonCount() * sectionSize))) {
    std::string msg("Newer BeamMonitorConfig packet is too short: ");
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  }
}

BeamMonitorConfigPkt::BeamMonitorConfigPkt(const BeamMonitorConfigPkt &pkt)
    : Packet(pkt), m_fields(reinterpret_cast<const uint32_t *>(payload())) {}

/* ------------------------------------------------------------------------ */

DetectorBankSetsPkt::DetectorBankSetsPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields(reinterpret_cast<const uint32_t *>(payload())), m_sectionOffsets(nullptr),
      m_after_banks_offset(nullptr) {
  // Get Number of Detector Bank Sets...
  //    - Basic Packet Size Sanity Check

  if (m_version == 0x00 && m_payload_len < sizeof(uint32_t)) {
    std::string msg("DetectorBankSets V0 packet is too short for Count! ");
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  } else if (m_version > ADARA::PacketType::DETECTOR_BANK_SETS_VERSION && m_payload_len < sizeof(uint32_t)) {
    std::string msg("Newer DetectorBankSets packet is too short for Count! ");
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  }

  uint32_t numSets = detBankSetCount();

  // Don't Allocate Anything if there are No Detector Bank Sets...
  if (numSets < 1)
    return;

  m_sectionOffsets = new uint32_t[numSets];

  m_after_banks_offset = new uint32_t[numSets];

  // Traverse Detector Bank Sets...
  //    - Set Section Offsets
  //    - Set "After Banks" Offsets

  // Base Section Sizes (w/o Bank Ids)
  uint32_t baseSectionOffsetPart1 = 0 + m_name_offset // name
                                    + 2;              // flags & bank id count
  uint32_t baseSectionOffsetPart2 = 0 + 3             // histo params
                                    + 2               // throttle rate (double)
                                    + m_suffix_offset;
  uint32_t baseSectionOffsetNoBanks = baseSectionOffsetPart1 + baseSectionOffsetPart2;

  // Running Section Offset (in number of uint32_t elements)
  uint32_t offset = 1; // for Detector Bank Set Count...

  for (uint32_t i = 0; i < numSets; i++) {
    // Section Offset
    m_sectionOffsets[i] = offset;

    if (m_version == 0x00 && m_payload_len < ((offset + baseSectionOffsetNoBanks) * sizeof(uint32_t))) {
      std::string msg("DetectorBankSets V0 packet: too short for Set ");
      msg += std::to_string(i + 1);
      msg += " of ";
      msg += std::to_string(numSets);
      msg += " sectionOffset=";
      msg += std::to_string(offset);
      msg += " baseSectionOffsetNoBanks=";
      msg += std::to_string(baseSectionOffsetNoBanks);
      msg += " payload_len=";
      msg += std::to_string(m_payload_len);
      delete[] m_sectionOffsets;
      m_sectionOffsets = nullptr;
      delete[] m_after_banks_offset;
      m_after_banks_offset = nullptr;
      throw invalid_packet(msg);
    } else if (m_version > ADARA::PacketType::DETECTOR_BANK_SETS_VERSION &&
               m_payload_len < ((offset + baseSectionOffsetNoBanks) * sizeof(uint32_t))) {
      std::string msg("Newer DetectorBankSets packet: too short for Set ");
      msg += std::to_string(i + 1);
      msg += " of ";
      msg += std::to_string(numSets);
      msg += " sectionOffset=";
      msg += std::to_string(offset);
      msg += " baseSectionOffsetNoBanks=";
      msg += std::to_string(baseSectionOffsetNoBanks);
      msg += " payload_len=";
      msg += std::to_string(m_payload_len);
      delete[] m_sectionOffsets;
      m_sectionOffsets = nullptr;
      delete[] m_after_banks_offset;
      m_after_banks_offset = nullptr;
      throw invalid_packet(msg);
    }

    // Offset thru end of Bank Ids list...
    offset += baseSectionOffsetPart1 + bankCount(i); // just in time m_sectionOffset delivery...!

    // Save as "After Banks" Offset...
    m_after_banks_offset[i] = offset;

    // Rest of Set Offset...
    offset += baseSectionOffsetPart2;
  }

  // Final Payload Size Check... ;-D
  if (m_version == 0x00 && m_payload_len < (offset * sizeof(uint32_t))) {
    std::string msg("DetectorBankSets V0 packet: overall too short ");
    msg += " numSets=";
    msg += std::to_string(numSets);
    msg += " baseSectionOffsetNoBanks=";
    msg += std::to_string(baseSectionOffsetNoBanks);
    msg += " final sectionOffset=";
    msg += std::to_string(offset);
    msg += " payload_len=";
    msg += std::to_string(m_payload_len);
    delete[] m_sectionOffsets;
    m_sectionOffsets = nullptr;
    delete[] m_after_banks_offset;
    m_after_banks_offset = nullptr;
    throw invalid_packet(msg);
  } else if (m_version > ADARA::PacketType::DETECTOR_BANK_SETS_VERSION && m_payload_len < (offset * sizeof(uint32_t))) {
    std::string msg("Newer DetectorBankSets packet: overall too short ");
    msg += " numSets=";
    msg += std::to_string(numSets);
    msg += " baseSectionOffsetNoBanks=";
    msg += std::to_string(baseSectionOffsetNoBanks);
    msg += " final sectionOffset=";
    msg += std::to_string(offset);
    msg += " payload_len=";
    msg += std::to_string(m_payload_len);
    delete[] m_sectionOffsets;
    m_sectionOffsets = nullptr;
    delete[] m_after_banks_offset;
    m_after_banks_offset = nullptr;
    throw invalid_packet(msg);
  }
}

DetectorBankSetsPkt::~DetectorBankSetsPkt() {
  delete[] m_sectionOffsets;
  delete[] m_after_banks_offset;
}

/* ------------------------------------------------------------------------ */

DataDonePkt::DataDonePkt(const uint8_t *data, uint32_t len) : Packet(data, len) {
  if (m_version == 0x00 && m_payload_len != 0)
    throw invalid_packet("DataDone V0 packet is incorrect size");
  else if (m_version > ADARA::PacketType::DATA_DONE_VERSION && m_payload_len == 0)
    throw invalid_packet("Newer DataDone packet is too short");
  // ...any newer version would have to grow, lol...? ;-D
}

/* ------------------------------------------------------------------------ */

DeviceDescriptorPkt::DeviceDescriptorPkt(const uint8_t *data, uint32_t len) : Packet(data, len) {
  const auto *fields = reinterpret_cast<const uint32_t *>(payload());
  uint32_t size;

  if (m_version == 0x00 && m_payload_len < (2 * sizeof(uint32_t)))
    throw invalid_packet("DeviceDescriptor V0 packet is too short");
  else if (m_version > ADARA::PacketType::DEVICE_DESC_VERSION && m_payload_len < (2 * sizeof(uint32_t)))
    throw invalid_packet("Newer DeviceDescriptor packet is too short");

  size = fields[1];
  if (m_version == 0x00 && m_payload_len < (size + (2 * sizeof(uint32_t)))) {
    throw invalid_packet("DeviceDescriptor V0 packet has oversize string");
  } else if (m_version > ADARA::PacketType::DEVICE_DESC_VERSION && m_payload_len < (size + (2 * sizeof(uint32_t)))) {
    throw invalid_packet("Newer DeviceDescriptor packet has oversize string");
  }

  /* TODO it would be better to create the string on access
   * rather than object construction; the user may not care.
   */
  m_devId = fields[0];
  m_desc.assign(reinterpret_cast<const char *>(&fields[2]), size);
}

/* ------------------------------------------------------------------------ */

VariableU32Pkt::VariableU32Pkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields(reinterpret_cast<const uint32_t *>(payload())) {
  if (m_version == 0x00 && m_payload_len != (4 * sizeof(uint32_t))) {
    std::string msg("VariableValue (U32) V0 packet is incorrect length: ");
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  } else if (m_version > ADARA::PacketType::VAR_VALUE_U32_VERSION && m_payload_len < (4 * sizeof(uint32_t))) {
    std::string msg("Newer VariableValue (U32) packet is too short: ");
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  }

  if (validate_status(status())) {
    std::string msg("VariableValue (U32) packet has invalid status: ");
    msg += std::to_string(status());
    throw invalid_packet(msg);
  }

  if (validate_severity(severity())) {
    std::string msg("VariableValue (U32) packet has invalid severity: ");
    msg += std::to_string(severity());
    throw invalid_packet(msg);
  }
}

VariableU32Pkt::VariableU32Pkt(const VariableU32Pkt &pkt)
    : Packet(pkt), m_fields(reinterpret_cast<const uint32_t *>(payload())) {}

/* ------------------------------------------------------------------------ */

VariableDoublePkt::VariableDoublePkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields(reinterpret_cast<const uint32_t *>(payload())) {
  if (m_version == 0x00 && m_payload_len != (sizeof(double) + (3 * sizeof(uint32_t)))) {
    std::string msg("VariableValue (Double) V0 packet is incorrect length: ");
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  } else if (m_version > ADARA::PacketType::VAR_VALUE_DOUBLE_VERSION &&
             m_payload_len < (sizeof(double) + (3 * sizeof(uint32_t)))) {
    std::string msg("Newer VariableValue (Double) packet is too short: ");
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  }

  if (validate_status(status())) {
    std::string msg("VariableValue (double) packet has invalid status: ");
    msg += std::to_string(status());
    throw invalid_packet(msg);
  }

  if (validate_severity(severity())) {
    std::string msg("VariableValue (double) packet has invalid severity: ");
    msg += std::to_string(severity());
    throw invalid_packet(msg);
  }
}

VariableDoublePkt::VariableDoublePkt(const VariableDoublePkt &pkt)
    : Packet(pkt), m_fields(reinterpret_cast<const uint32_t *>(payload())) {}

/* ------------------------------------------------------------------------ */

VariableStringPkt::VariableStringPkt(const uint8_t *data, uint32_t len)
    : Packet(data, len), m_fields(reinterpret_cast<const uint32_t *>(payload())) {
  uint32_t size;

  if (m_version == 0x00 && m_payload_len < (4 * sizeof(uint32_t))) {
    std::string msg("VariableValue (String) V0 packet is too short ");
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  } else if (m_version > ADARA::PacketType::VAR_VALUE_STRING_VERSION && m_payload_len < (4 * sizeof(uint32_t))) {
    std::string msg("Newer VariableValue (String) packet is too short ");
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  }

  size = m_fields[3];
  if (m_version == 0x00 && m_payload_len < (size + (4 * sizeof(uint32_t)))) {
    std::string msg("VariableValue (String) V0 packet has oversize string: ");
    msg += std::to_string(size);
    msg += " vs payload ";
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  } else if (m_version > ADARA::PacketType::VAR_VALUE_STRING_VERSION &&
             m_payload_len < (size + (4 * sizeof(uint32_t)))) {
    std::string msg("Newer VariableValue (String) packet has oversize string: ");
    msg += std::to_string(size);
    msg += " vs payload ";
    msg += std::to_string(m_payload_len);
    throw invalid_packet(msg);
  }

  if (validate_status(status())) {
    std::string msg("VariableValue (string) packet has invalid status: ");
    msg += std::to_string(status());
    throw invalid_packet(msg);
  }

  if (validate_severity(severity())) {
    std::string msg("VariableValue (string) packet has invalid severity: ");
    msg += std::to_string(severity());
    throw invalid_packet(msg);
  }

  /* TODO it would be better to create the string on access
   * rather than object construction; the user may not care.
   */
  m_val.assign(reinterpret_cast<const char *>(&m_fields[4]), size);
}

VariableStringPkt::VariableStringPkt(const VariableStringPkt &pkt)
    : Packet(pkt), m_fields(reinterpret_cast<const uint32_t *>(payload())), m_val(pkt.m_val) {}
