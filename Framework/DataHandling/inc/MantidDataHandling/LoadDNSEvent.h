#ifndef MANTID_DATAHANDLING_LoadDNSEvent_H_
#define MANTID_DATAHANDLING_LoadDNSEvent_H_

#include "BitStream.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidTypes/Core/DateAndTime.h"

#include <array>
#include <fstream>
#include <limits>
//#include <iterator>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

/**
  LoadDNSEvent

  Algorithm used to generate an EventWorkspace from a DNS PSD listmode (.mdat)
  file.

  @author Joachim Coenen, Jülich Centre for Neutron Science
  @date 2018-08-16

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport LoadDNSEvent : public API::Algorithm {
public:
  ///
  const std::string name() const override { return "LoadDNSEvent"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads data from the new PSD detector to a Mantid EventWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling"; }

private:
  static const std::string INSTRUMENT_NAME;
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  enum class BufferType { DATA = 0, COMMAND = 1 };

  enum class DeviceStatus {
    DAQ_STOPPED_SYNC_ERROR = 0,
    DAQ_RUNNING_SYNC_ERROR = 1,
    DAQ_STOPPED_SYNC_OK = 2,
    DAQ_RUNNING_SYNC_OK = 3
  };

  struct BufferHeader {
    uint16_t bufferLength;
    uint16_t bufferVersion;
    BufferType bufferType;
    uint16_t headerLength; // static const uint16_t headerLength = 21;
    uint16_t bufferNumber;
    uint16_t runId;
    uint8_t mcpdId;
    DeviceStatus deviceStatus;
    uint64_t timestamp;
    // std::array<uint16_t, 3> parameter0;
    // std::array<uint16_t, 3> parameter1;
    // std::array<uint16_t, 3> parameter2;
    // std::array<uint16_t, 3> parameter3;
  };

  struct NeutronEventData {
    uint8_t modId;
    //! number of the slot inside the MPSD
    uint8_t slotId;
    //! amplitude value of the neutron event
    uint16_t amplitude;
    //! position of the neutron event
    uint16_t position;
  };

  struct TriggerEventData {
    uint8_t trigId;
    uint8_t dataId;
    uint32_t data;
  };

public:
  enum event_id_e { NEUTRON = 0, TRIGGER = 1 };

  struct Event {
    event_id_e eventId;

    union {
      NeutronEventData neutron;
      TriggerEventData trigger;
    } data;

    uint8_t mcpdId;
    uint64_t timestamp;
  };

  struct CompactEvent {
    uint64_t timestamp;
  };

private:
  struct EventAccumulator {
    //! Neutron Events for each pixel
    std::vector<std::vector<CompactEvent>> neutronEvents;
    std::vector<CompactEvent> triggerEvents;
  };

  uint32_t chopperChannel;
  uint32_t monitorChannel;

  void runLoadInstrument(std::string instrumentName,
                         DataObjects::EventWorkspace_sptr &eventWS);

  void populate_EventWorkspace(Mantid::DataObjects::EventWorkspace_sptr eventWS,
                               EventAccumulator &finalEventAccumulator);

  EventAccumulator parse_File(FileByteStream &file, const std::string fileName);
  std::vector<uint8_t> parse_Header(FileByteStream &file);

  std::vector<std::vector<uint8_t>> split_File(FileByteStream &file,
                                               const unsigned maxChunckCount);

  void parse_BlockList(VectorByteStream &file,
                       EventAccumulator &eventAccumulator);
  void parse_Block(VectorByteStream &file, EventAccumulator &eventAccumulator);
  void parse_BlockSeparator(VectorByteStream &file);
  void parse_DataBuffer(VectorByteStream &file,
                        EventAccumulator &eventAccumulator);
  BufferHeader parse_DataBufferHeader(VectorByteStream &file);

  inline size_t getWsIndex(const uint16_t &channel, const uint16_t &position) {
    const uint32_t channelIndex = ((channel & 0b1111111111100000u) >> 1u) |
                                  (channel & 0b0000000000001111u);
    return channelIndex * 960 + position;
  }

  inline void parse_andAddEvent(VectorByteStream &file,
                                const BufferHeader &bufferHeader,
                                EventAccumulator &eventAccumulator);

  void parse_EndSignature(FileByteStream &file);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LoadDNSEvent_H_ */
