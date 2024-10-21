#pragma once

//
// SNS ADARA SYSTEM - Common Library
//
// This repository contains the software for the next-generation Data
// Acquisition System (DAS) at the Spallation Neutron Source (SNS) at
// Oak Ridge National Laboratory (ORNL) -- "ADARA".
//
// Copyright (c) 2015, UT-Battelle LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <stdexcept>
#include <string>

namespace ADARA {

const std::string VERSION = "1.5.1";
const std::string TAG_NAME = "XXX_TAG_NAME_XXX";

#define ADARA_PKT_TYPE(_base_type, _version) ((((uint32_t)(_base_type)) << 8) | (_version))

#define ADARA_BASE_PKT_TYPE(_type) ((_type) >> 8)

#define ADARA_PKT_VERSION(_type) ((_type) & 0xff)
namespace PacketType {
enum Type {
  RAW_EVENT_TYPE = 0x0000,
  RTDL_TYPE = 0x0001,
  SOURCE_LIST_TYPE = 0x0002,
  MAPPED_EVENT_TYPE = 0x0003,
  BANKED_EVENT_TYPE = 0x4000,
  BEAM_MONITOR_EVENT_TYPE = 0x4001,
  PIXEL_MAPPING_TYPE = 0x4002,
  RUN_STATUS_TYPE = 0x4003,
  RUN_INFO_TYPE = 0x4004,
  TRANS_COMPLETE_TYPE = 0x4005,
  CLIENT_HELLO_TYPE = 0x4006,
  STREAM_ANNOTATION_TYPE = 0x4007,
  SYNC_TYPE = 0x4008,
  HEARTBEAT_TYPE = 0x4009,
  GEOMETRY_TYPE = 0x400A,
  BEAMLINE_INFO_TYPE = 0x400B,
  DATA_DONE_TYPE = 0x400C,
  BEAM_MONITOR_CONFIG_TYPE = 0x400D,
  DETECTOR_BANK_SETS_TYPE = 0x400E,
  DEVICE_DESC_TYPE = 0x8000,
  VAR_VALUE_U32_TYPE = 0x8001,
  VAR_VALUE_DOUBLE_TYPE = 0x8002,
  VAR_VALUE_STRING_TYPE = 0x8003,
};

enum Version {
  RAW_EVENT_VERSION = 0x00,
  RTDL_VERSION = 0x00,
  SOURCE_LIST_VERSION = 0x00,
  MAPPED_EVENT_VERSION = 0x00,
  BANKED_EVENT_VERSION = 0x01,
  BEAM_MONITOR_EVENT_VERSION = 0x01,
  PIXEL_MAPPING_VERSION = 0x00,
  RUN_STATUS_VERSION = 0x00,
  RUN_INFO_VERSION = 0x00,
  TRANS_COMPLETE_VERSION = 0x00,
  CLIENT_HELLO_VERSION = 0x01,
  STREAM_ANNOTATION_VERSION = 0x00,
  SYNC_VERSION = 0x00,
  HEARTBEAT_VERSION = 0x00,
  GEOMETRY_VERSION = 0x00,
  BEAMLINE_INFO_VERSION = 0x01,
  DATA_DONE_VERSION = 0x00,
  BEAM_MONITOR_CONFIG_VERSION = 0x00,
  DETECTOR_BANK_SETS_VERSION = 0x00,
  DEVICE_DESC_VERSION = 0x00,
  VAR_VALUE_U32_VERSION = 0x00,
  VAR_VALUE_DOUBLE_VERSION = 0x00,
  VAR_VALUE_STRING_VERSION = 0x00,
};
} // namespace PacketType

/* These are defined in the SNS Timing Master Functional System Description,
 * section 1.3.4.
 */
namespace PulseFlavor {
enum Enum {
  NO_BEAM = 0,
  NORMAL = 1,
  NORMAL_TGT_1 = 1,
  NORMAL_TGT_2 = 2,
  DIAG_10us = 3,
  DIAG_50us = 4,
  DIAG_100us = 5,
  SPECIAL_PHYSICS_1 = 6,
  SPECIAL_PHYSICS_2 = 7
};
}

namespace RunStatus {
enum Enum {
  NO_RUN = 0,
  NEW_RUN = 1,
  RUN_EOF = 2,
  RUN_BOF = 3,
  END_RUN = 4,
  STATE = 5,
};
}

namespace VariableStatus {
enum Enum {
  OK = 0, // EPICS: NO_ALARM
  READ_ERROR = 1,
  WRITE_ERROR = 2,
  HIHI_LIMIT = 3,
  HIGH_LIMIT = 4,
  LOLO_LIMIT = 5,
  LOW_LIMIT = 6,
  BAD_STATE = 7,
  CHANGED_STATE = 8,
  NO_COMMUNICATION = 9,
  COMMUNICATION_TIMEOUT = 10,
  HARDWARE_LIMIT = 11,
  BAD_CALCULATION = 12,
  INVALID_SCAN = 13,
  LINK_FAILED = 14,
  INVALID_STATE = 15,
  BAD_SUBROUTINE = 16,
  UNDEFINED_ALARM = 17,
  DISABLED = 18,
  SIMULATED = 19,
  READ_PERMISSION = 20,
  WRITE_PERMISSION = 21,
  UPSTREAM_DISCONNECTED = 0xfffe,
  NOT_REPORTED = 0xffff,
};
}

namespace VariableSeverity {
enum Enum {
  OK = 0, // EPICS: NO_ALARM
  MINOR_ALARM = 1,
  MAJOR_ALARM = 2,
  INVALID = 3,
  NOT_REPORTED = 0xffff,
};
}

namespace MarkerType {
enum Enum {
  GENERIC,
  SCAN_START,
  SCAN_STOP,
  PAUSE,
  RESUME,
  OVERALL_RUN_COMMENT,
};
}

struct Event {
  uint32_t tof;
  uint32_t pixel;
};

struct Header {
  uint32_t payload_len;
  uint32_t pkt_format;
  uint32_t ts_sec;
  uint32_t ts_nsec;
};

class invalid_packet : public std::runtime_error {
public:
  explicit invalid_packet(const std::string &msg) : runtime_error(msg) {}
};

enum { EPICS_EPOCH_OFFSET = 631152000 };

} /* namespace ADARA */
