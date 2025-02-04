// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>

#include "ADARA.h"
#include "MantidLiveData/DllConfig.h"

namespace ADARA {

class MANTID_LIVEDATA_DLL PacketHeader {
public:
  PacketHeader(const uint8_t *data) {
    const uint32_t *field = reinterpret_cast<const uint32_t *>(data);

    m_payload_len = field[0];
    m_type = field[1];
    m_base_type = (PacketType::Type)ADARA_BASE_PKT_TYPE(m_type);
    m_version = (PacketType::Version)ADARA_PKT_VERSION(m_type);

#if 0
// NOTE: Windows doesn't have struct timespec and Mantid doesn't really need this,
// so for now we're just going to comment it out.
      /* Convert EPICS epoch to Unix epoch,
        * Jan 1, 1990 ==> Jan 1, 1970
        */
      m_timestamp.tv_sec = field[2] + EPICS_EPOCH_OFFSET;
      m_timestamp.tv_nsec = field[3];
#endif

    m_pulseId = ((uint64_t)field[2]) << 32;
    m_pulseId |= field[3];
  }

  uint32_t type() const { return m_type; }
  PacketType::Type base_type() const { return m_base_type; }
  PacketType::Version version() const { return m_version; }
  uint32_t payload_length() const { return m_payload_len; }
#if 0
	const struct timespec &timestamp(void) const { return m_timestamp; }
#endif
  uint64_t pulseId() const { return m_pulseId; }
  uint32_t packet_length() const { return m_payload_len + 16; }

  static uint32_t header_length() { return 16; }

protected:
  uint32_t m_payload_len;
  uint32_t m_type;
  PacketType::Type m_base_type;
  PacketType::Version m_version;

#if 0
	struct timespec m_timestamp;
#endif
  uint64_t m_pulseId;

  /* Don't allow the default constructor */
  PacketHeader();
};

class MANTID_LIVEDATA_DLL Packet : public PacketHeader {
public:
  Packet(const uint8_t *data, uint32_t len);
  Packet(const Packet &pkt);

  virtual ~Packet();

  const uint8_t *packet() const { return m_data; }
  const uint8_t *payload() const { return m_data + header_length(); }

protected:
  const uint8_t *m_data;
  uint32_t m_len;
  bool m_allocated;

private:
  /* Don't allow the default constructor or assignment operator */
  Packet();
  Packet &operator=(const Packet &pkt);
};

class MANTID_LIVEDATA_DLL RawDataPkt : public Packet {
public:
  RawDataPkt(const RawDataPkt &pkt);

  uint32_t sourceID() const { return m_fields[0]; }
  bool endOfPulse() const { return !!(m_fields[1] & 0x80000000); }
  uint16_t pktSeq() const { return (m_fields[1] >> 16) & 0x7fff; }
  uint16_t dspSeq() const { return m_fields[1] & 0x7fff; }
  PulseFlavor::Enum flavor() const { return static_cast<PulseFlavor::Enum>((m_fields[2] >> 24) & 0x7); }
  uint32_t pulseCharge() const { return m_fields[2] & 0x00ffffff; }
  bool badVeto() const { return !!(m_fields[3] & 0x80000000); }
  bool badCycle() const { return !!(m_fields[3] & 0x40000000); }
  uint8_t timingStatus() const { return (uint8_t)(m_fields[3] >> 22); }
  uint16_t vetoFlags() const { return (m_fields[3] >> 10) & 0xfff; }
  uint16_t cycle() const { return m_fields[3] & 0x3ff; }
  uint32_t intraPulseTime() const { return m_fields[4]; }
  bool tofCorrected() const { return !!(m_fields[5] & 0x80000000); }
  uint32_t tofOffset() const { return m_fields[5] & 0x7fffffff; }
  uint32_t tofField() const { return m_fields[5]; }

  const Event *events() const { return (const Event *)&m_fields[6]; }
  uint32_t num_events() const { return (m_payload_len - 24) / (uint32_t)(2 * sizeof(uint32_t)); }

private:
  const uint32_t *m_fields;

  RawDataPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
  friend class MappedDataPkt;
};

class MANTID_LIVEDATA_DLL MappedDataPkt : public RawDataPkt {
public:
private:
  MappedDataPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL RTDLPkt : public Packet {
public:
  RTDLPkt(const RTDLPkt &pkt);

  PulseFlavor::Enum flavor() const { return static_cast<PulseFlavor::Enum>((m_fields[0] >> 24) & 0x7); }

  uint32_t pulseCharge() const { return m_fields[0] & 0x00ffffff; }

  void setPulseCharge(uint32_t pulseCharge) {
    m_fields[0] &= 0xff000000;
    m_fields[0] |= pulseCharge & 0x00ffffff;
  }

  bool badVeto() const { return !!(m_fields[1] & 0x80000000); }
  bool badCycle() const { return !!(m_fields[1] & 0x40000000); }
  uint8_t timingStatus() const { return (uint8_t)(m_fields[1] >> 22); }
  uint16_t vetoFlags() const { return (m_fields[1] >> 10) & 0xfff; }

  void setVetoFlags(uint16_t vetoFlags) {
    m_fields[1] &= 0xffc003ff;
    m_fields[1] |= (vetoFlags & 0xfff) << 10;
  }

  uint16_t cycle() const { return m_fields[1] & 0x3ff; }
  uint32_t intraPulseTime() const { return m_fields[2]; }
  bool tofCorrected() const { return !!(m_fields[3] & 0x80000000); }
  uint32_t tofOffset() const { return m_fields[3] & 0x7fffffff; }
  uint32_t ringPeriod() const { return m_fields[4] & 0xffffff; }

  // accessor methods for optional FNA/Frame Data fields

  uint32_t FNA(uint32_t index) const {
    // If out of bounds, just return "0" for "Unused Frame"... ;-D
    if (index > 24)
      return 0;
    else
      return (m_fields[5 + index] >> 24) & 0xff;
  }

  uint32_t frameData(uint32_t index) const {
    // Out of bounds, return "-1" (0xffffff) for Bogus "Frame Data" ;-b
    if (index > 24)
      return -1;
    else
      return m_fields[5 + index] & 0xffffff;
  }

private:
  // Note: RTDLPkt m_fields can't be "const", as we Modify Pulse Charge!
  uint32_t *m_fields;

  RTDLPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL SourceListPkt : public Packet {
public:
  const uint32_t *ids() const { return reinterpret_cast<const uint32_t *>(payload()); }
  uint32_t num_ids() const { return (uint32_t)payload_length() / (uint32_t)sizeof(uint32_t); }

private:
  SourceListPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL BankedEventPkt : public Packet {
public:
  BankedEventPkt(const BankedEventPkt &pkt);

  enum Flags {
    ERROR_PIXELS = 0x00001,
    PARTIAL_DATA = 0x00002,
    PULSE_VETO = 0x00004,
    MISSING_RTDL = 0x00008,
    MAPPING_ERROR = 0x00010,
    DUPLICATE_PULSE = 0x00020,
    PCHARGE_UNCORRECTED = 0x00040,
    VETO_UNCORRECTED = 0x00080,
  };

  uint32_t pulseCharge() const { return m_fields[0]; }
  uint32_t pulseEnergy() const { return m_fields[1]; }
  uint32_t cycle() const { return m_fields[2]; }
  uint32_t vetoFlags() const { return (m_fields[3] >> 20) & 0xfff; }
  uint32_t flags() const { return m_fields[3] & 0xfffff; }

  // The source, bank and event accessors all return NULL if we've
  // incremented past the end
  const Event *firstEvent() const;
  const Event *nextEvent() const;

  bool getSourceCORFlag() const { return m_isCorrected; }
  uint32_t getSourceTOFOffset() const { return m_TOFOffset; }
  uint32_t curBankId() const { return m_bankId; }

  //        uint32_t curEventCount() const { return ((uint32_t *)m_curBank)[1];
  //        }

private:
  // Two helper functions for firstEvent() & nextEvent()
  void firstEventInSource() const;
  void firstEventInBank() const;

  const uint32_t *m_fields;

  mutable const Event *m_curEvent;

  mutable unsigned m_lastFieldIndex; // index into m_fields for the last valid
                                     // field in the packet
  mutable unsigned m_curFieldIndex;  // where we currently are in the packet

  // Data about the current source section
  mutable unsigned m_sourceStartIndex; // index into m_fields for the start of
                                       // this source
  mutable uint32_t m_bankCount;
  mutable uint32_t m_TOFOffset;
  mutable bool m_isCorrected;
  mutable unsigned m_bankNum; // which bank are we currently in (relative to the
                              // start of the section)

  // Data about the current bank
  mutable unsigned m_bankStartIndex; // index into m_fields for the start of
                                     // this source
  mutable uint32_t m_bankId;
  mutable uint32_t m_eventCount;

  BankedEventPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL BeamMonitorPkt : public Packet {
public:
  BeamMonitorPkt(const BeamMonitorPkt &pkt);

  uint32_t pulseCharge() const { return m_fields[0]; }
  uint32_t pulseEnergy() const { return m_fields[1]; }
  uint32_t cycle() const { return m_fields[2]; }
  uint32_t vetoFlags() const { return (m_fields[3] >> 20) & 0xfff; }
  uint32_t flags() const { return m_fields[3] & 0xfffff; }

  // bool firstSection() const;
  bool nextSection() const; // iterate over the sections in the packet

  // Section-specific functions (these require either firstSection() or
  // nextSection() to have run successfully before they will return
  // valid data.
  uint32_t getSectionMonitorID() const;
  uint32_t getSectionEventCount() const;
  uint32_t getSectionSourceID() const;
  uint32_t getSectionTOFOffset() const;
  bool sectionTOFCorrected() const;
  bool nextEvent(bool &risingEdge, uint32_t &cycle, uint32_t &tof) const;

private:
  const uint32_t *m_fields;

  // Data about the current monitor section
  mutable uint32_t m_sectionStartIndex; // index into m_fields for the start of
                                        // this section

  // used to keep nextEvent from running past the end of the section
  mutable uint32_t m_eventNum;

  BeamMonitorPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL PixelMappingPkt : public Packet {
public:
  // TODO implement accessors for fields
private:
  PixelMappingPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL RunStatusPkt : public Packet {
public:
  RunStatusPkt(const RunStatusPkt &pkt);

  uint32_t runNumber() const { return m_fields[0]; }
  uint32_t runStart() const { return m_fields[1]; }
  uint32_t fileNumber() const { return m_fields[2] & 0xffffff; }
  RunStatus::Enum status() const { return static_cast<RunStatus::Enum>(m_fields[2] >> 24); }

private:
  const uint32_t *m_fields;

  RunStatusPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL RunInfoPkt : public Packet {
public:
  const std::string &info() const { return m_xml; }

private:
  std::string m_xml;

  RunInfoPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL TransCompletePkt : public Packet {
public:
  TransCompletePkt(const TransCompletePkt &pkt);

  uint16_t status() const { return m_status; }
  const std::string &reason() const { return m_reason; }

private:
  uint16_t m_status;
  std::string m_reason;

  TransCompletePkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL ClientHelloPkt : public Packet {
public:
  enum Flags {
    PAUSE_AGNOSTIC = 0x0000,
    NO_PAUSE_DATA = 0x0001,
    SEND_PAUSE_DATA = 0x0002,
  };

  uint32_t requestedStartTime() const { return m_reqStart; }
  uint32_t clientFlags() const { return m_clientFlags; }

private:
  uint32_t m_reqStart;
  uint32_t m_clientFlags;

  ClientHelloPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL AnnotationPkt : public Packet {
public:
  AnnotationPkt(const AnnotationPkt &pkt);

  bool resetHint() const { return !!(m_fields[0] & 0x80000000); }
  MarkerType::Enum marker_type() const {
    uint16_t type = (m_fields[0] >> 16) & 0x7fff;
    return static_cast<MarkerType::Enum>(type);
  }
  uint32_t scanIndex() const { return m_fields[1]; }
  const std::string &comment() const {
    if (!m_comment.length() && (m_fields[0] & 0xffff)) {
      m_comment.assign(reinterpret_cast<const char *>(&m_fields[2]), m_fields[0] & 0xffff);
    }

    return m_comment;
  }

private:
  const uint32_t *m_fields;
  mutable std::string m_comment;

  AnnotationPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL SyncPkt : public Packet {
public:
  // TODO implement accessors for fields
private:
  SyncPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL HeartbeatPkt : public Packet {
public:
private:
  HeartbeatPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL GeometryPkt : public Packet {
public:
  const std::string &info() const { return m_xml; }

private:
  std::string m_xml;

  GeometryPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL BeamlineInfoPkt : public Packet {
public:
  const uint32_t &targetStationNumber() const { return m_targetStationNumber; }

  const std::string &id() const { return m_id; }
  const std::string &shortName() const { return m_shortName; }
  const std::string &longName() const { return m_longName; }

private:
  uint32_t m_targetStationNumber;

  std::string m_id;
  std::string m_shortName;
  std::string m_longName;

  BeamlineInfoPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL BeamMonitorConfigPkt : public Packet {
public:
  BeamMonitorConfigPkt(const BeamMonitorConfigPkt &pkt);

  uint32_t beamMonCount() const { return m_fields[0]; }

  uint32_t bmonId(uint32_t index) const {
    if (index < beamMonCount())
      return m_fields[(index * 6) + 1];
    else
      return (0);
  }

  uint32_t tofOffset(uint32_t index) const {
    if (index < beamMonCount())
      return m_fields[(index * 6) + 2];
    else
      return (0);
  }

  uint32_t tofMax(uint32_t index) const {
    if (index < beamMonCount())
      return m_fields[(index * 6) + 3];
    else
      return (0);
  }

  uint32_t tofBin(uint32_t index) const {
    if (index < beamMonCount())
      return m_fields[(index * 6) + 4];
    else
      return (0);
  }

private:
  double extractDouble(uint32_t index, uint32_t fieldOffset) const {
    if (index < beamMonCount()) {
      double value;
      std::memcpy(&value, &m_fields[(index * 6) + fieldOffset], sizeof(double));
      return value;
    } else {
      return 0.0;
    }
  }

  double distance(uint32_t index) const { return extractDouble(index, 5); }

private:
  const uint32_t *m_fields;

  BeamMonitorConfigPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL DetectorBankSetsPkt : public Packet {
public:
  DetectorBankSetsPkt(const DetectorBankSetsPkt &pkt) = delete;
  DetectorBankSetsPkt &operator=(DetectorBankSetsPkt) = delete;

  ~DetectorBankSetsPkt() override;

  // Detector Bank Set Name, alphanumeric characters...
  static const size_t SET_NAME_SIZE = 16;

  // Throttle Suffix, alphanumeric, no spaces/punctuation...
  static const size_t THROTTLE_SUFFIX_SIZE = 16;

private:
  std::string extractString(uint32_t index, uint32_t fieldOffset, size_t size) const {
    const char *start = reinterpret_cast<const char *>(&m_fields[fieldOffset]);
    return std::string(start, size);
  }

private:
  double extractDouble(uint32_t index, uint32_t fieldOffset) const {
    if (index < detBankSetCount()) {
      double value;
      std::memcpy(&value, &m_fields[fieldOffset], sizeof(double));
      return value;
    } else {
      return 0.0;
    }
  }

  enum Flags {
    EVENT_FORMAT = 0x0001,
    HISTO_FORMAT = 0x0002,
  };

  uint32_t detBankSetCount() const { return m_fields[0]; }

  uint32_t sectionOffset(uint32_t index) const {
    if (index < detBankSetCount())
      return (m_sectionOffsets[index]);
    else
      return (0); // Minimum Valid offset is always past Header...
  }

  std::string name(uint32_t index) const {
    return (index < detBankSetCount()) ? extractString(index, m_sectionOffsets[index], SET_NAME_SIZE)
                                       : "<Out Of Range!>";
  }

  uint32_t flags(uint32_t index) const {
    if (index < detBankSetCount())
      return m_fields[m_sectionOffsets[index] + m_name_offset];
    else
      return (0);
  }

  uint32_t bankCount(uint32_t index) const {
    if (index < detBankSetCount())
      return m_fields[m_sectionOffsets[index] + m_name_offset + 1];
    else
      return (0);
  }

  const uint32_t *banklist(uint32_t index) const {
    if (index < detBankSetCount()) {
      return (const uint32_t *)&m_fields[m_sectionOffsets[index] + m_name_offset + 2];
    } else {
      // Shouldn't be asking for this if bankCount() returned 0...!
      return ((const uint32_t *)nullptr);
    }
  }

  uint32_t tofOffset(uint32_t index) const {
    if (index < detBankSetCount())
      return m_fields[m_after_banks_offset[index]];
    else
      return (0);
  }

  uint32_t tofMax(uint32_t index) const {
    if (index < detBankSetCount())
      return m_fields[m_after_banks_offset[index] + 1];
    else
      return (0);
  }

  uint32_t tofBin(uint32_t index) const {
    if (index < detBankSetCount())
      return m_fields[m_after_banks_offset[index] + 2];
    else
      return (0);
  }

  double throttle(uint32_t index) const { return extractDouble(index, m_after_banks_offset[index] + 3); }

  std::string suffix(uint32_t index) const {
    return (index < detBankSetCount()) ? extractString(index, m_after_banks_offset[index] + 5, THROTTLE_SUFFIX_SIZE)
                                       : "out-of-range-" + std::to_string(index);
  }

private:
  const uint32_t *m_fields;

  static const uint32_t m_name_offset = SET_NAME_SIZE / sizeof(uint32_t);

  static const uint32_t m_suffix_offset = THROTTLE_SUFFIX_SIZE / sizeof(uint32_t);

  uint32_t *m_sectionOffsets;

  uint32_t *m_after_banks_offset;

  DetectorBankSetsPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL DataDonePkt : public Packet {
public:
private:
  DataDonePkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL DeviceDescriptorPkt : public Packet {
public:
  uint32_t devId() const { return m_devId; }
  const std::string &description() const { return m_desc; }

  void remapDevice(uint32_t dev) {
    uint32_t *fields = reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(payload()));
    fields[0] = dev;
    m_devId = dev;
  };

private:
  uint32_t m_devId;
  std::string m_desc;

  DeviceDescriptorPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL VariableU32Pkt : public Packet {
public:
  VariableU32Pkt(const VariableU32Pkt &pkt);

  uint32_t devId() const { return m_fields[0]; }
  uint32_t varId() const { return m_fields[1]; }
  VariableStatus::Enum status() const { return static_cast<VariableStatus::Enum>(m_fields[2] >> 16); }
  VariableSeverity::Enum severity() const { return static_cast<VariableSeverity::Enum>(m_fields[2] & 0xffff); }
  uint32_t value() const { return m_fields[3]; }

  void remapDeviceId(uint32_t dev) {
    uint32_t *fields = reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(payload()));
    fields[0] = dev;
  };

private:
  const uint32_t *m_fields;

  VariableU32Pkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL VariableDoublePkt : public Packet {
public:
  VariableDoublePkt(const VariableDoublePkt &pkt);

  uint32_t devId() const { return m_fields[0]; }
  uint32_t varId() const { return m_fields[1]; }
  VariableStatus::Enum status() const { return static_cast<VariableStatus::Enum>(m_fields[2] >> 16); }
  VariableSeverity::Enum severity() const { return static_cast<VariableSeverity::Enum>(m_fields[2] & 0xffff); }
  double value() const {
    double result;
    std::memcpy(&result, &m_fields[3], sizeof(double));
    return result;
  }

  void remapDeviceId(uint32_t dev) {
    uint32_t *fields = reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(payload()));
    fields[0] = dev;
  };

private:
  const uint32_t *m_fields;

  VariableDoublePkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

class MANTID_LIVEDATA_DLL VariableStringPkt : public Packet {
public:
  VariableStringPkt(const VariableStringPkt &pkt);

  uint32_t devId() const { return m_fields[0]; }
  uint32_t varId() const { return m_fields[1]; }
  VariableStatus::Enum status() const { return static_cast<VariableStatus::Enum>(m_fields[2] >> 16); }
  VariableSeverity::Enum severity() const { return static_cast<VariableSeverity::Enum>(m_fields[2] & 0xffff); }
  const std::string &value() const { return m_val; }

  void remapDeviceId(uint32_t dev) {
    uint32_t *fields = reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(payload()));
    fields[0] = dev;
  };

private:
  const uint32_t *m_fields;
  std::string m_val;

  VariableStringPkt(const uint8_t *data, uint32_t len);

  friend class Parser;
};

} // namespace ADARA
