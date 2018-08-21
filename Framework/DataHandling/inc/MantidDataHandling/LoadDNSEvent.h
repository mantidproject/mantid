#ifndef MANTID_DATAHANDLING_LoadDNSEvent_H_
#define MANTID_DATAHANDLING_LoadDNSEvent_H_

#include "BitStream.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTypes/Core/DateAndTime.h"


#include <array>
#include <set>
#include <fstream>
#include <limits>
#include <iterator>

using namespace Mantid::Kernel;
using namespace Mantid::API;


namespace Mantid {
namespace DataHandling {

/**
  LoadDNSEvent

  Algorithm used to generate an EventWorkspace from a DNS listmode (.mdat) file.

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
  const std::string name() const override {
    return "LoadDNSEvent";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load data from the new PSD detector to a Mantid EventWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return { };
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling";
  }

private:
  static const std::string INSTRUMENT_NAME;
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  typedef uint64_t separator_t;

  enum class BufferType {
    DATA = 0,
    COMMAND = 1
  };

  enum class DeviceStatus {
    DAQ_STOPPED_SYNC_ERROR = 0,
    DAQ_RUNNING_SYNC_ERROR = 1,
    DAQ_STOPPED_SYNC_OK = 2,
    DAQ_RUNNING_SYNC_OK = 3,
    DAQ_RUNNING = 1,
    SYNC_OK = 2
  };

  struct BufferHeader {
    uint16_t bufferLength;
    uint16_t bufferVersion;
    BufferType bufferType;
    uint16_t headerLength; //static const uint16_t headerLength = 21;
    uint16_t bufferNumber;
    uint16_t runId;
    uint8_t  mcpdId;
    DeviceStatus deviceStatus;
    uint64_t timestamp;
    // std::array<uint16_t, 3> parameter0;
    // std::array<uint16_t, 3> parameter1;
    // std::array<uint16_t, 3> parameter2;
    // std::array<uint16_t, 3> parameter3;
  };

  struct NeutronEventData {
    uint8_t	modId;
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
  enum event_id_e {
    NEUTRON = 0,
    TRIGGER = 1
  };

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
    uint16_t channel;
    uint16_t position;
    event_id_e eventId;
  };

private:

  struct EventAccumulator {
    std::vector<std::vector<CompactEvent>> neutronEvents;
    std::vector<CompactEvent> triggerEvents;
  };
  EventAccumulator _eventAccumulator;

  uint chopperChannel;
  uint monitorChannel;

  uint chopperPeriod;

  void runLoadInstrument(std::string instrumentName, DataObjects::EventWorkspace_sptr &eventWS);

  long populate_EventWorkspace(Mantid::DataObjects::EventWorkspace_sptr eventWS);

  void parse_File(FileByteStream &file);
  std::vector<uint8_t> parse_Header(FileByteStream &file);
  void parse_BlockList(FileByteStream &file, EventAccumulator &eventAccumulator);
  void parse_Block(FileByteStream &file, EventAccumulator &eventAccumulator);
  void parse_BlockSeparator(FileByteStream &file);
  void parse_DataBuffer(FileByteStream &file, EventAccumulator &eventAccumulator);
  BufferHeader parse_DataBufferHeader(FileByteStream &file);

  inline size_t getWsIndex(const CompactEvent &event) {
    const uint16_t channelIndex = ((event.channel & 0b1111111111100000) >> 1) | (event.channel & 0b0000000000001111);
    const uint16_t position = std::min(uint16_t(959u), event.position);
    return channelIndex * 960 + position + 24;
  }

  inline void parse_andAddEvent(FileByteStream &file, const BufferHeader &bufferHeader, EventAccumulator &eventAccumulator) {
    CompactEvent event = {};
    const auto dataChunk = file.extractDataChunk<6>().readBits<1>(event.eventId);

    switch (event.eventId) {
      case event_id_e::TRIGGER: {
        uint8_t trigId;
        dataChunk
            .readBits<3>(trigId)
            .skipBits<25>()
            .readBits<19>(event.timestamp);
        if (!(trigId == chopperChannel)) {
          return;
        }
        event.timestamp += bufferHeader.timestamp;
        //event.channel |= bufferHeader.mcpdId << 8;
        eventAccumulator.triggerEvents.push_back(event);
      } break;
      case event_id_e::NEUTRON: {
        dataChunk
            .readBits<8>(event.channel)
            .skipBits<10>()
            .readBits<10>(event.position)
            .readBits<19>(event.timestamp);
        event.timestamp += bufferHeader.timestamp;
        event.channel |= bufferHeader.mcpdId << 8;

        const size_t wsIndex = getWsIndex(event);
        eventAccumulator.neutronEvents[wsIndex].push_back(event);
      } break;
    default:
      // Panic!!!!
      g_log.error() << "unknow event id 0x" << n2hexstr(event.eventId) << "\n";
      break;
    }
  }


  void parse_EndSignature(FileByteStream &file);


};

}
}

#endif /* MANTID_DATAHANDLING_LoadDNSEvent_H_ */
