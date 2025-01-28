// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/WarningSuppressions.h"

#include <cstdint>
#include <cstring>
#include <ctime>
#include <vector>

// to ignore warnings when comparing header versions
GNU_DIAG_OFF("type-limits")
#if defined(_WIN32)
#pragma warning(disable : 4296)
#endif

namespace Mantid {
namespace LiveData {

/// @file TCPEventStreamDefs.h Definitions for an ISIS Event Stream
///
/// @author Freddie Akeroyd, STFC ISIS Facility, GB
///
/// The stream is a sequence of #TCPStreamEventHeader followed by the
/// appropriate data for that header type
/// For neutron event data this is all described in #TCPStreamEventDataNeutron
/// etc
///
/// The data is generated in TCPEventStreamConnection::allEventCallback() and
/// then spooled to clients in TCPEventStreamConnection::run()
/// See EventsToolApp::liveData() in events_tool.cpp for a client example

/// this structure is provided at the start of a packet
/// if a stream gets corrupt, you could look for two consecutive 0xffffffff
/// (marker1, marker2) to find a starting point to
/// continue the stream
struct TCPStreamEventHeader {
  uint32_t marker1; ///< always 0xffffffff
  uint32_t marker2; ///< always 0xffffffff
  uint32_t version; ///< should be TCPStreamEventHeader::current_version
  uint32_t length;  ///< this packet size in bytes
  uint32_t type;    ///< #StreamDataType

  enum StreamDataType { InvalidStream = 0, Setup = 1, Neutron = 2, SE = 3 };
  static const uint32_t marker = 0xffffffff; ///< magic value for marker1, marker2
  TCPStreamEventHeader()
      : marker1(marker), marker2(marker), version(current_version), length(sizeof(TCPStreamEventHeader)),
        type(InvalidStream) {}
  TCPStreamEventHeader(uint32_t type_)
      : marker1(marker), marker2(marker), version(current_version), length(sizeof(TCPStreamEventHeader)), type(type_) {}

  GNU_DIAG_OFF("tautological-compare")
  bool isValid() const {
    return marker1 == marker && marker2 == marker && length >= sizeof(TCPStreamEventHeader) &&
           majorVersion() == TCPStreamEventHeader::major_version &&
           // This is already suppressed for gcc on Linux
           // cppcheck-suppress unsignedPositive
           minorVersion() >= TCPStreamEventHeader::minor_version && type != InvalidStream;
  }
  GNU_DIAG_ON("tautological-compare")

  static const uint32_t major_version = 1; ///< starts at 1, then incremented whenever layout of this or further
  /// packets changes in a non backward compatible way
  static const uint32_t minor_version = 0; ///< reset to 0 in major version change, then incremented whenever
  /// layout of this or further packets changes in a backward compatible
  /// way
  static const uint32_t current_version = (major_version << 16); ///< starts at 1, then incremented
  /// whenever layout of this or
  /// further packets changes
  uint32_t majorVersion() const { return version >> 16; }
  uint32_t minorVersion() const { return version & 0xffff; }
};

/// header for initial data packet send on initial connection and on a state
/// change e.g. run number changes
struct TCPStreamEventHeaderSetup {
  enum { StartTime = 0x1, RunNumber = 0x2, RunState = 0x4, InstName = 0x8 } ChangedFields;
  uint32_t length;    ///< packet size in bytes
  time_t start_time;  ///< run start time from #ISISCRPT_STRUCT
  int run_number;     ///< run number from #ISISCRPT_STRUCT
  int run_state;      ///< SETUP etc
  char inst_name[32]; ///< instrument name

  TCPStreamEventHeaderSetup()
      : ChangedFields(), length(sizeof(TCPStreamEventHeaderSetup)), start_time(0), run_number(0), run_state(0) {
    inst_name[0] = '\0';
  }
  bool isValid() const { return length >= sizeof(TCPStreamEventHeaderSetup); }
  uint32_t changedFields(const TCPStreamEventHeaderSetup &ref) const {
    uint32_t changed = 0;
    changed |= (start_time != ref.start_time ? StartTime : 0);
    changed |= (run_number != ref.run_number ? RunNumber : 0);
    changed |= (run_state != ref.run_state ? RunState : 0);
    changed |= (strcmp(inst_name, ref.inst_name) ? InstName : 0);
    return changed;
  }
};

/// placeholder for sample environment data
struct TCPStreamEventHeaderSE {
  float time_offset;
};

/// this structure is part of a sequence of neutron events, which are all from
/// the same ISIS frame
struct TCPStreamEventHeaderNeutron {
  uint32_t length;       ///< packet size in bytes
  uint32_t frame_number; ///< ISIS frame number, 0 being first frame of run
  uint32_t period;       ///< period number
  float protons;         ///< proton charge (uAh) for this frame
  float frame_time_zero; ///< time offset from run_start of this frame, in seconds
  uint32_t nevents;      ///< number of TCPStreamEvent() structures in this packet

  TCPStreamEventHeaderNeutron()
      : length(sizeof(TCPStreamEventHeaderNeutron)), frame_number(0), period(0), protons(0), frame_time_zero(0),
        nevents(0) {}
  bool isValid() const { return length >= sizeof(TCPStreamEventHeaderNeutron); }
};

/// structure describing an individual neutron event following on from a
/// #TCPStreamEventHeaderNeutron
struct TCPStreamEventNeutron {
  float time_of_flight; ///< neutron time of flight within frame (microseconds)
  uint32_t spectrum;    ///< spectrum number neutron count was recorded in
};

/// structure of a packet describing a set of events - all these events
/// correspond to
/// the same ISIS frame (as specified in #TCPStreamEventHeaderNeutron) but there
/// may be several
/// of these structures sent for each frame. There is no guarantee that you will
/// receive all
/// structures for frame n before receiving some structures for frame n+1 as
/// data is spooled
/// immediately it is read from a detector card. In the actual network stream
/// the data will be
/// an array rather than a stl vector as shown in the structure
/// The data is generated in TCPEventStreamConnection::allEventCallback() and
/// then spooled to clients in TCPEventStreamConnection::run()
/// See EventsToolApp::liveData() in events_tool.cpp for a client example
struct TCPStreamEventDataNeutron {
  TCPStreamEventHeader head;
  TCPStreamEventHeaderNeutron head_n; ///< details of ISIS frame data was
  /// collected in and the number of neutron
  /// events in this packet
  std::vector<TCPStreamEventNeutron> data; ///< list of neutron events

  TCPStreamEventDataNeutron() : head(TCPStreamEventHeader::Neutron) {}
  TCPStreamEventDataNeutron(const TCPStreamEventHeader &head_) : head(head_) {}
  bool isValid() const {
    return head.isValid() && head_n.isValid() && (head.type == TCPStreamEventHeader::Neutron) &&
           (data.size() == head_n.nevents);
  }
};

/// layout of initial data packet send on initial connection and on a state
/// change e.g. run number changes
struct TCPStreamEventDataSetup {
  TCPStreamEventHeader head; ///< details of ISIS frame data was collected in
  /// and the number of neutron events in this packet
  TCPStreamEventHeaderSetup head_setup;
  TCPStreamEventDataSetup() : head(TCPStreamEventHeader::Setup) {}
  TCPStreamEventDataSetup(const TCPStreamEventHeader &head_) : head(head_) {}
  bool isValid() const { return head.isValid() && head_setup.isValid() && (head.type == TCPStreamEventHeader::Setup); }
};

// placeholder fro SE data
struct TCPStreamEventDataSE {
  TCPStreamEventHeader head; ///< details of ISIS frame data was collected in
  /// and the number of neutron events in this packet
  TCPStreamEventHeaderSE head_s;
};
} // namespace LiveData
} // namespace Mantid

GNU_DIAG_ON("type-limits")
