// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+

#include "ADARAPacketBuilders.h"
#include "ADARAPackets.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iterator>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Helper: write a 4-byte little-endian uint32_t into a byte vector at offset
// ---------------------------------------------------------------------------

namespace {

void writeU32LE(std::vector<uint8_t> &buf, std::size_t offset, uint32_t val) {
  buf[offset + 0] = static_cast<uint8_t>(val & 0xFF);
  buf[offset + 1] = static_cast<uint8_t>((val >> 8) & 0xFF);
  buf[offset + 2] = static_cast<uint8_t>((val >> 16) & 0xFF);
  buf[offset + 3] = static_cast<uint8_t>((val >> 24) & 0xFF);
}

void appendU32LE(std::vector<uint8_t> &buf, uint32_t val) {
  buf.push_back(static_cast<uint8_t>(val & 0xFF));
  buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
  buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
  buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

// Verify the packet's payload-length field (bytes 0-3, LE) equals total_size - 16.
void assertPayloadLen(const std::vector<uint8_t> &pkt) {
  assert(pkt.size() >= 4);
  uint32_t reported = static_cast<uint32_t>(pkt[0]) | (static_cast<uint32_t>(pkt[1]) << 8) |
                      (static_cast<uint32_t>(pkt[2]) << 16) | (static_cast<uint32_t>(pkt[3]) << 24);
  assert(reported + 16 == pkt.size());
  (void)reported; // suppress unused-variable warning in release builds
}

} // namespace

// ---------------------------------------------------------------------------
// Packet builders
// ---------------------------------------------------------------------------

namespace Mantid::LiveData::Testing {

// buildHeartbeatPkt — base heartbeatPacketV0[16]
// Header layout (LE):
//   Byte  0–3:  payload length    (0x00000000 for heartbeat)
//   Byte  4–7:  packet type/ver   (0x00400900 for heartbeat v0)
//   Byte  8–11: pulse ID seconds  (upper 32 bits: pulseId >> 32)
//   Byte 12–15: pulse ID nanos    (lower 32 bits: pulseId & 0xFFFFFFFF)
std::vector<uint8_t> buildHeartbeatPkt(uint64_t pulseId) {
  std::vector<uint8_t> pkt(heartbeatPacketV0, heartbeatPacketV0 + sizeof(heartbeatPacketV0));
  writeU32LE(pkt, 8, static_cast<uint32_t>(pulseId >> 32));
  writeU32LE(pkt, 12, static_cast<uint32_t>(pulseId & 0xFFFFFFFF));
  assertPayloadLen(pkt);
  return pkt;
}

// buildRunStatusPkt — select base fixture by status value (ADARA::RunStatus::Enum)
// Run Status v1 byte layout (from ADARAPackets.h comments ~line 201):
//   Byte  0–3:  payload length   = 0x00000014 (20 bytes)
//   Byte  4–7:  type/ver          = 0x00400301 (RUN_STATUS v1)
//   Byte  8–11: timestamp seconds (patch with pulseId >> 32)
//   Byte 12–15: timestamp nanos   (patch with pulseId & 0xFFFFFFFF)
//   Byte 16–19: run number        (patch with runNumber, LE u32)
//   Byte 20–23: run start seconds (leave as-is from fixture)
//   Byte 24–27: status | flags    (patch status nibble; OR in caller value)
//   Byte 28–31: paused            (leave as-is)
//   Byte 32–35: addendum          (leave as-is)
std::vector<uint8_t> buildRunStatusPkt(int status, uint32_t runNumber, uint64_t pulseId) {
  const unsigned char *base = nullptr;
  std::size_t baseSize = 0;

  // ADARA::RunStatus::Enum values: NEW_RUN=1, RUN_EOF=2, RUN_BOF=3, END_RUN=4, STATE=5
  switch (status) {
  case 1: // NEW_RUN
    base = runStatusPacketV1NewRun;
    baseSize = sizeof(runStatusPacketV1NewRun);
    break;
  case 2: // RUN_EOF (EOF_PKT)
    base = runStatusPacketV1RunEOF;
    baseSize = sizeof(runStatusPacketV1RunEOF);
    break;
  case 3: // RUN_BOF (NEW_FILE)
    base = runStatusPacketV1RunBOF;
    baseSize = sizeof(runStatusPacketV1RunBOF);
    break;
  case 4: // END_RUN
    base = runStatusPacketV1EndRun;
    baseSize = sizeof(runStatusPacketV1EndRun);
    break;
  case 5: // STATE (no-run)
  default:
    base = runStatusPacketV1NoRun;
    baseSize = sizeof(runStatusPacketV1NoRun);
    break;
  }

  std::vector<uint8_t> pkt(base, base + baseSize);
  // Patch timestamp (bytes 8–15) with pulseId
  writeU32LE(pkt, 8, static_cast<uint32_t>(pulseId >> 32));
  writeU32LE(pkt, 12, static_cast<uint32_t>(pulseId & 0xFFFFFFFF));
  // Patch run number (bytes 16–19)
  writeU32LE(pkt, 16, runNumber);
  // Patch status nibble (bytes 24–27): keep flags from fixture, OR in caller status
  uint32_t existingFlags = static_cast<uint32_t>(pkt[24]) | (static_cast<uint32_t>(pkt[25]) << 8) |
                           (static_cast<uint32_t>(pkt[26]) << 16) | (static_cast<uint32_t>(pkt[27]) << 24);
  // Clear low byte (status byte) and OR in new status
  existingFlags = (existingFlags & 0xFFFFFF00u) | (static_cast<uint32_t>(status) & 0xFF);
  writeU32LE(pkt, 24, existingFlags);
  assertPayloadLen(pkt);
  return pkt;
}

// buildVariableU32Pkt — base variableU32Packet[32]
//   Byte  0–3:  payload length   = 0x00000010 (16 bytes)
//   Byte  4–7:  type/ver          = 0x00800100
//   Byte  8–11: timestamp seconds (patch with pulseId >> 32)
//   Byte 12–15: timestamp nanos   (patch with pulseId & 0xFFFFFFFF)
//   Byte 16–19: device ID         (patch with devId)
//   Byte 20–23: variable ID       (patch with pvId)
//   Byte 24–27: status/severity   (leave as-is: 0x00000000 = OK/OK)
//   Byte 28–31: value             (patch with value)
std::vector<uint8_t> buildVariableU32Pkt(uint32_t devId, uint32_t pvId, uint32_t value, uint64_t pulseId) {
  std::vector<uint8_t> pkt(variableU32Packet, variableU32Packet + sizeof(variableU32Packet));
  writeU32LE(pkt, 8, static_cast<uint32_t>(pulseId >> 32));
  writeU32LE(pkt, 12, static_cast<uint32_t>(pulseId & 0xFFFFFFFF));
  writeU32LE(pkt, 16, devId);
  writeU32LE(pkt, 20, pvId);
  writeU32LE(pkt, 28, value);
  assertPayloadLen(pkt);
  return pkt;
}

// buildVariableDoublePkt — base variableDoublePacket[36]
//   Byte  0–3:  payload length   = 0x00000014 (20 bytes)
//   Byte  4–7:  type/ver          = 0x00800200
//   Byte  8–11: timestamp seconds (patch with pulseId >> 32)
//   Byte 12–15: timestamp nanos   (patch with pulseId & 0xFFFFFFFF)
//   Byte 16–19: device ID         (patch with devId)
//   Byte 20–23: variable ID       (patch with pvId)
//   Byte 24–27: status/severity   (leave as-is: 0x00000000)
//   Byte 28–35: value             (patch with double via memcpy — IEEE 754 LE)
std::vector<uint8_t> buildVariableDoublePkt(uint32_t devId, uint32_t pvId, double value, uint64_t pulseId) {
  std::vector<uint8_t> pkt(variableDoublePacket, variableDoublePacket + sizeof(variableDoublePacket));
  writeU32LE(pkt, 8, static_cast<uint32_t>(pulseId >> 32));
  writeU32LE(pkt, 12, static_cast<uint32_t>(pulseId & 0xFFFFFFFF));
  writeU32LE(pkt, 16, devId);
  writeU32LE(pkt, 20, pvId);
  std::memcpy(pkt.data() + 28, &value, 8);
  assertPayloadLen(pkt);
  return pkt;
}

// buildPausePkt — verbatim copy of AnnotationPacketType3
std::vector<uint8_t> buildPausePkt() {
  return std::vector<uint8_t>(AnnotationPacketType3, AnnotationPacketType3 + sizeof(AnnotationPacketType3));
}

// buildResumePkt — verbatim copy of AnnotationPacketType4
std::vector<uint8_t> buildResumePkt() {
  return std::vector<uint8_t>(AnnotationPacketType4, AnnotationPacketType4 + sizeof(AnnotationPacketType4));
}

// buildBankedEventPkt — reference bankedEventPacketV1[216]
// (ADARAPackets.h layout diagram at ~line 31)
// Variable-length: events section has variable length depending on events.size().
// Justification: the events section (bank ID + event array) has variable length.
//
// Packet layout:
//   Byte  0–3:  payload length (compute from event count)
//   Byte  4–7:  type/ver = 0x00400001 (BANKED_EVENT v1)
//   Byte  8–11: pulse ID seconds
//   Byte 12–15: pulse ID nanos
//   Byte 16–19: pulse charge (units of 10 pC, u32)
//   Byte 20–23: pulse energy (eV, u32; set 0 for tests)
//   Byte 24–27: accelerator cycle (u32; set 0 for tests)
//   Byte 28–31: veto flags | flags (copy 0x00400003 from fixture)
//   Byte 32+:   one source section (16 bytes header) + one bank section
//               (8 bytes) + events (8 bytes each)
//
// Source section header (4 x uint32_t = 16 bytes):
//   source ID (0), intra-pulse time (0), COR+TOF offset (0), bank count (1)
// Bank section header (2 x uint32_t = 8 bytes):
//   bank ID (0 = first real detector bank), event count.
//   NB: bank IDs 0xFFFFFFFF (-1) and 0xFFFFFFFE (-2) are sentinel
//   "error / unmapped" banks and SNSLiveEventDataListener::rxPacket
//   (BankedEventPkt) deliberately drops their events without ever
//   calling appendEvent() (see SNSLiveEventDataListener.cpp:432).
//   Tests that assert getNumberEvents() > 0 therefore require a
//   real bank ID (< 0xFFFFFFFE) here.
// Events: pairs of (TOF u32, pixel ID u32) = 8 bytes each
std::vector<uint8_t> buildBankedEventPkt(uint64_t pulseId, double pulseChargePc, std::vector<PixelTof> const &events) {
  // Fixed fields + 1 source section + 1 bank section + events
  // payload = 16 (fixed) + 16 (source hdr) + 8 (bank hdr) + 8*N (events)
  const uint32_t nEvents = static_cast<uint32_t>(events.size());
  const uint32_t payloadLen = 16 + 16 + 8 + 8 * nEvents;
  const uint32_t totalLen = 16 + payloadLen; // 16-byte ADARA header

  std::vector<uint8_t> pkt;
  pkt.reserve(totalLen);

  // ADARA header
  appendU32LE(pkt, payloadLen);                                  // payload length
  appendU32LE(pkt, 0x00400001u);                                 // type/ver BANKED_EVENT v1
  appendU32LE(pkt, static_cast<uint32_t>(pulseId >> 32));        // pulse ID seconds
  appendU32LE(pkt, static_cast<uint32_t>(pulseId & 0xFFFFFFFF)); // pulse ID nanos

  // Fixed fields (from bankedEventPacketV1 bytes 16–31)
  auto chargeUnits = static_cast<uint32_t>(pulseChargePc / 10.0); // 10 pC per unit
  appendU32LE(pkt, chargeUnits);                                  // pulse charge (units of 10 pC)
  appendU32LE(pkt, 0u);                                           // pulse energy (eV) — 0 for tests
  appendU32LE(pkt, 0u);                                           // accelerator cycle — 0 for tests
  appendU32LE(pkt, 0x00400003u);                                  // veto flags | flags (from fixture byte 28)

  // Source section header (bytes 32–47 in a typical fixture)
  appendU32LE(pkt, 0u); // source ID
  appendU32LE(pkt, 0u); // intra-pulse time
  appendU32LE(pkt, 0u); // COR+TOF offset
  appendU32LE(pkt, 1u); // bank count = 1

  // Bank section header
  appendU32LE(pkt, 0u);      // bank ID = 0 (first real detector bank)
  appendU32LE(pkt, nEvents); // event count

  // Events
  for (const auto &ev : events) {
    appendU32LE(pkt, ev.tof);
    appendU32LE(pkt, ev.pixel);
  }

  assertPayloadLen(pkt);
  return pkt;
}

// buildBeamMonitorPkt — reference beamMonitorPacketV1[136]
// (test/ADARAPackets.h:92; parser: src/ADARA/ADARAPackets.cpp:321-378)
// Variable-length: TOF count varies per call.
//
// Packet layout — ADARA header (16B, not part of payload) followed by payload:
//   Byte 16–19: pulse charge
//   Byte 20–23: pulse energy
//   Byte 24–27: accelerator cycle
//   Byte 28–31: veto flags | flags
// Per-section (one section per call):
//   Word 0: bits 31:22 = monitorId (10 bits), bits 21:0 = eventCount (22 bits)
//           (Parser: BeamMonitorPkt::getSectionMonitorID() >> 22;
//                    BeamMonitorPkt::getSectionEventCount() & 0x003FFFFF)
//   Word 1: sourceID
//   Word 2: bit 31 = TOFCorrected flag, bits 30:0 = TOFOffset
//   Words 3..: one uint32 per event — bit 31 = risingEdge, bits 30:21 = cycle,
//              bits 20:0 = TOF (100 ns units)
// payloadLen = 16 (fixed) + 12 (section header) + 4 * N (events)
std::vector<uint8_t> buildBeamMonitorPkt(uint64_t pulseId, uint32_t monitorId, std::vector<uint32_t> const &tofs) {
  const uint32_t nTofs = static_cast<uint32_t>(tofs.size());
  const uint32_t payloadLen = 16 + 12 + 4 * nTofs;

  std::vector<uint8_t> pkt;
  pkt.reserve(16 + payloadLen);

  // ADARA header
  appendU32LE(pkt, payloadLen);
  appendU32LE(pkt, 0x00400101u); // type/ver BEAM_MONITOR v1
  appendU32LE(pkt, static_cast<uint32_t>(pulseId >> 32));
  appendU32LE(pkt, static_cast<uint32_t>(pulseId & 0xFFFFFFFF));

  // Fixed fields (zero for tests)
  appendU32LE(pkt, 0u); // pulse charge
  appendU32LE(pkt, 0u); // pulse energy
  appendU32LE(pkt, 0u); // accelerator cycle
  appendU32LE(pkt, 0u); // veto flags | flags

  // Section header: monitorId in upper 10 bits, eventCount in lower 22 bits
  appendU32LE(pkt, ((monitorId & 0x3FFu) << 22) | (nTofs & 0x003FFFFFu));
  appendU32LE(pkt, 0u); // sourceID
  appendU32LE(pkt, 0u); // TOFCorrected (bit 31) | TOFOffset

  // Events: risingEdge(31) | cycle(30:21) | TOF(20:0); tests supply TOF only
  for (uint32_t tof : tofs) {
    appendU32LE(pkt, tof & 0x001FFFFFu);
  }

  assertPayloadLen(pkt);
  return pkt;
}

// buildRunInfoPkt — reference runInfoPacketV0[92]
// Variable-length: XML payload varies per call.
// Justification: XML payload varies per call.
//
// Layout: header (16 bytes) + XML length (4-byte LE u32) + XML bytes + padding
// XML = "<runinfo><proposal_id>proposalId</proposal_id><run_title>title</run_title></runinfo>"
// The <runinfo> root element and the <run_title> child name are required by
// SNSLiveEventDataListener::rxPacket(RunInfoPkt) — it locates the "runinfo"
// root via Poco::XML::Document::firstChild() and then only recognises the
// "proposal_id" and "run_title" child element names.  Producing XML without
// the root would throw SAXParseException; using "<title>" would silently
// leave the run-title property empty.
std::vector<uint8_t> buildRunInfoPkt(const std::string &proposalId, const std::string &title) {
  std::string xml =
      "<runinfo><proposal_id>" + proposalId + "</proposal_id><run_title>" + title + "</run_title></runinfo>";
  // Pad XML to 4-byte boundary
  std::size_t xmlLen = xml.size();
  std::size_t paddedLen = (xmlLen + 3) & ~std::size_t{3};

  // payload = 4 (xml length field) + paddedLen
  const uint32_t payloadLen = static_cast<uint32_t>(4 + paddedLen);
  const uint32_t totalLen = 16 + payloadLen;

  std::vector<uint8_t> pkt;
  pkt.reserve(totalLen);

  // ADARA header
  appendU32LE(pkt, payloadLen);
  appendU32LE(pkt, 0x00400400u); // type/ver RUN_INFO v0
  appendU32LE(pkt, 0u);          // timestamp seconds (zero for tests)
  appendU32LE(pkt, 0u);          // timestamp nanos

  // XML length
  appendU32LE(pkt, static_cast<uint32_t>(xmlLen));

  // XML bytes
  std::transform(xml.begin(), xml.end(), std::back_inserter(pkt), [](char c) { return static_cast<uint8_t>(c); });

  // Padding to 4-byte boundary
  while (pkt.size() < totalLen)
    pkt.push_back(0u);

  assertPayloadLen(pkt);
  return pkt;
}

// buildGeometryPkt — reference geometryPacketV0[92]
// Variable-length: XML payload varies.
// Justification: XML payload varies.
// Note: For tests that only need any valid geometry, use geometryPacketV0 array
// verbatim via PKT() shorthand instead of invoking this builder.
//
// Layout: header (16 bytes) + XML length (4-byte LE u32) + XML bytes + padding
std::vector<uint8_t> buildGeometryPkt(const std::string &xml) {
  std::size_t xmlLen = xml.size();
  std::size_t paddedLen = (xmlLen + 3) & ~std::size_t{3};

  const uint32_t payloadLen = static_cast<uint32_t>(4 + paddedLen);
  const uint32_t totalLen = 16 + payloadLen;

  std::vector<uint8_t> pkt;
  pkt.reserve(totalLen);

  // ADARA header
  appendU32LE(pkt, payloadLen);
  appendU32LE(pkt, 0x00400a00u); // type/ver GEOMETRY v0
  appendU32LE(pkt, 0u);
  appendU32LE(pkt, 0u);

  appendU32LE(pkt, static_cast<uint32_t>(xmlLen));

  std::transform(xml.begin(), xml.end(), std::back_inserter(pkt), [](char c) { return static_cast<uint8_t>(c); });

  while (pkt.size() < totalLen)
    pkt.push_back(0u);

  assertPayloadLen(pkt);
  return pkt;
}

// buildBeamlineInfoPkt — reference beamlineInfoPacketV1[32]
// Variable-length: name + name-length fields vary.
// Justification: longName and length fields vary per call.
// Note: For tests that only need any valid beamline info, use the array verbatim.
//
// Layout (from ADARAPackets.cpp BeamlineInfoPkt::BeamlineInfoPkt):
//   Header (16 bytes): payload_len, type/ver 0x00400b01, ts_sec, ts_nsec
//   Payload:
//     sizes (4 bytes LE u32): longName_len (8b) | shortName_len<<8 (8b) | id_len<<16 (8b) | station<<24 (8b)
//     id bytes (0 — no id for tests)
//     shortName bytes (0 — no shortName for tests)
//     longName bytes
std::vector<uint8_t> buildBeamlineInfoPkt(const std::string &longName) {
  if (longName.size() > 255)
    throw std::invalid_argument("buildBeamlineInfoPkt: longName exceeds 255 bytes (ADARA wire-format limit)");
  uint8_t longNameLen = static_cast<uint8_t>(longName.size());
  // payload = 4 (sizes word) + longNameLen; no id, no shortName
  uint32_t payloadLen = 4 + longNameLen;
  // Pad to 4-byte boundary
  uint32_t paddedPayload = (payloadLen + 3) & ~uint32_t{3};
  uint32_t totalLen = 16 + paddedPayload;

  std::vector<uint8_t> pkt;
  pkt.reserve(totalLen);

  appendU32LE(pkt, paddedPayload);
  appendU32LE(pkt, 0x00400b01u); // type/ver BEAMLINE_INFO v1
  appendU32LE(pkt, 0u);
  appendU32LE(pkt, 0u);

  // sizes: longName_len (bits 7-0) | shortName_len (bits 15-8) | id_len (bits 23-16) | station (bits 31-24)
  uint32_t sizes = static_cast<uint32_t>(longNameLen);
  appendU32LE(pkt, sizes);

  std::transform(longName.begin(), longName.end(), std::back_inserter(pkt),
                 [](char c) { return static_cast<uint8_t>(c); });

  // Padding
  while (pkt.size() < totalLen)
    pkt.push_back(0u);

  assertPayloadLen(pkt);
  return pkt;
}

// buildDeviceDescriptorPkt — reference devDesPacket[2600]
// Variable-length: XML content and device ID vary per call.
// Justification: XML content and device ID vary per call.
//
// Layout:
//   Byte  0–3:  payload length
//   Byte  4–7:  type/ver = 0x00800000 (DEVICE_DESC v0)
//   Byte  8–11: timestamp seconds (zero for tests)
//   Byte 12–15: timestamp nanos (zero for tests)
//   Byte 16–19: device ID (LE u32)
//   Byte 20–23: descriptor length (LE u32, = xmlDescriptor.size())
//   Byte 24+:   XML bytes (padded to 4-byte boundary)
std::vector<uint8_t> buildDeviceDescriptorPkt(uint32_t devId, const std::string &xmlDescriptor) {
  std::size_t xmlLen = xmlDescriptor.size();
  std::size_t paddedXmlLen = (xmlLen + 3) & ~std::size_t{3};

  // payload = 4 (devId) + 4 (descriptor length) + paddedXmlLen
  const uint32_t payloadLen = static_cast<uint32_t>(8 + paddedXmlLen);
  const uint32_t totalLen = 16 + payloadLen;

  std::vector<uint8_t> pkt;
  pkt.reserve(totalLen);

  // ADARA header
  appendU32LE(pkt, payloadLen);
  appendU32LE(pkt, 0x00800000u); // type/ver DEVICE_DESC v0
  appendU32LE(pkt, 0u);
  appendU32LE(pkt, 0u);

  // Device ID
  appendU32LE(pkt, devId);
  // Descriptor length (actual, not padded)
  appendU32LE(pkt, static_cast<uint32_t>(xmlLen));

  // XML bytes
  std::transform(xmlDescriptor.begin(), xmlDescriptor.end(), std::back_inserter(pkt),
                 [](char c) { return static_cast<uint8_t>(c); });

  // Padding to 4-byte boundary
  while (pkt.size() < totalLen)
    pkt.push_back(0u);

  assertPayloadLen(pkt);
  return pkt;
}

} // namespace Mantid::LiveData::Testing
