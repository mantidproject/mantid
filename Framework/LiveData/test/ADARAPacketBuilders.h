// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Mantid::LiveData::Testing {

// ----- Packet-builder helpers -----

std::vector<uint8_t> buildHeartbeatPkt(uint64_t pulseId);

// status: ADARA::RunStatus::Enum value (NEW_RUN, END_RUN, STATE, RUN_BOF, RUN_EOF)
std::vector<uint8_t> buildRunStatusPkt(int status, uint32_t runNumber, uint64_t pulseId);

std::vector<uint8_t> buildVariableU32Pkt(uint32_t devId, uint32_t pvId, uint32_t value, uint64_t pulseId);

std::vector<uint8_t> buildVariableDoublePkt(uint32_t devId, uint32_t pvId, double value, uint64_t pulseId);

std::vector<uint8_t> buildPausePkt();  // verbatim copy of AnnotationPacketType3
std::vector<uint8_t> buildResumePkt(); // verbatim copy of AnnotationPacketType4

struct PixelTof {
  uint32_t tof;
  uint32_t pixel;
};

std::vector<uint8_t> buildBankedEventPkt(uint64_t pulseId, double pulseChargePc, std::vector<PixelTof> const &events);

std::vector<uint8_t> buildBeamMonitorPkt(uint64_t pulseId, uint32_t monitorId, std::vector<uint32_t> const &tofs);

std::vector<uint8_t> buildRunInfoPkt(const std::string &proposalId, const std::string &title);

std::vector<uint8_t> buildGeometryPkt(const std::string &xml);

std::vector<uint8_t> buildBeamlineInfoPkt(const std::string &longName);

std::vector<uint8_t> buildDeviceDescriptorPkt(uint32_t devId, const std::string &xmlDescriptor);

} // namespace Mantid::LiveData::Testing
