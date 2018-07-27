#ifndef MANTID_DATAHANDLING_LoadDNSEvent_H_
#define MANTID_DATAHANDLING_LoadDNSEvent_H_

#include "BitStream.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidTypes/Core/DateAndTime.h"


#include <array>
#include <fstream>

// #include <experimental/optional>
// std::experimental::optional opt;
/*
template<typename T>
struct Optional {
public:
  constexpr Optional() noexcept
  : value{} { }

  // Constructors for engaged optionals.
  constexpr Optional(const T& value)
  : value(value), hasValue(true) { }

  constexpr Optional(T&& value)
  : value(std::move(value)), hasValue(true) { }

  template<typename... Args>
    constexpr explicit Optional(Args&&... args)
    : value(std::forward<Args>(args)...), hasValue(true) { }

  // Copy and move constructors.
  Optional(const Optional& other) {
    if (other.hasValue){
      value = T_Stored(*other);
      hasValue = true;
    } else {
      empty = Empty();
      hasValue = false;
    }
  }

  Optional(Optional&& __other) {
    if (other.hasValue){
      value = T_Stored(std::move(*other));
      hasValue = true;
    } else {
      empty = Empty();
      hasValue = false;
    }
  }

  template<typename R>
  const Optional bind(const function<Optional<R>(T)> func) {
    return hasValue ? func(value) : Optional<R>();
  }

  template<typename R>
  const Optional bind(const function<R(T)> func) {

    return hasValue ? Optional<R>(func(value)) : Optional<R>();
  }

  template<typename R, typename E>
  const Optional bind(const function<R(T)> func, Optional<E> &exception) {
    Optional<R> result;
    try {
      result = bind(func);
      exception = Optional<E>();
    } catch(E e) {
      exception = Optional(e);
      result = Optional<R>();
    }
    return result;
  }


private:
  using T_Stored = remove_const_t<_Tp>;
  struct Empty { };
  union {
      Empty empty;
      T_Stored value;
  };
  bool hasValue = false;
};
*/

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

/**
  LoadDNSEvent

  Algorithm used to generate a GroupingWorkspace from an .xml or .map file
  containing the
  detectors' grouping information.

  @date 2011-11-17

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  typedef uint64_t                 separator_t;
  //typedef std::array<uint16_t, 21> buffer_header_t;

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

  enum event_id_e {
    NEUTRON = 0,
    TRIGGER = 1
  };

  //typedef std::array<uint16_t, 3>  event_t;

//  struct RawNeutronEvent {
//    //! flag to indicate a neutron event
//    event_id_e id;
//    //! number of the MPSD generating this event
//    uint8_t	modId;
//    //! number of the slot inside the MPSD
//    uint8_t	slotId;
//    //! amplitude value of the neutron event
//    uint16_t	amplitude;
//    //! position of the neutron event
//    uint16_t position;
//    //! timestamp of the neutron event
//    uint32_t	timestamp;
//  };

  typedef uint32_t EventCount;
  //typedef uint16_t ChannelIndex;

public:
  struct ChannelIndex {
    uint8_t mcpdId;
    uint8_t	modId;
    uint8_t	slotId;
    bool operator < (ChannelIndex const& rhs) const {
      return (mcpdId < rhs.mcpdId)
          || ((mcpdId == rhs.mcpdId)  && (
             (modId < rhs.modId)
          || ((modId == rhs.modId) &&
             (slotId < rhs.slotId))));
    }
    bool operator == (ChannelIndex const& rhs) const {
      return (mcpdId == rhs.mcpdId) && (modId == rhs.modId) && (slotId == rhs.slotId);
    }
  };

  struct EventPosition {
    ChannelIndex channel;
    uint16_t position;

    bool operator < (EventPosition const& rhs) const {
      return (channel < rhs.channel)
          || ((channel == rhs.channel)  && (position < rhs.position));
    }

  };

  struct NeutronEventData {
    uint8_t	modId;
    //! number of the slot inside the MPSD
    uint8_t slotId;
    //! amplitude value of the neutron event
    uint16_t amplitude;
    //! position of the neutron event
    uint16_t position;

    uint32_t timestamp;
  };

  struct TriggerEventData {
    uint8_t trigId;
    uint8_t dataId;
    uint32_t data;
    uint32_t timestamp;
  };

  struct NeutronEvent {
    event_id_e eventId;
    uint16_t amplitude;
    double tof;
    uint32_t timestamp;
    uint64_t headerTim;

  };


  typedef size_t HeaderId;
  struct Event {
    event_id_e eventId;

    union {
      NeutronEventData neutron;
      TriggerEventData trigger;
    } data;

    uint64_t timestamp;
    HeaderId headerId;
  };

private:
  struct EventAccumulator {

    EventAccumulator(Kernel::Logger &g_log, Progress &progress)
      : g_log(g_log), progress(progress) { }
  private:
    Kernel::Logger &g_log;
    Progress &progress;
  public:

    inline void addEvent(const Event &event) {
      switch (event.eventId) {
        case event_id_e::TRIGGER: {
            chopperTimestamp = event.timestamp + headerTimestamp;
        } break;
        case event_id_e::NEUTRON: {
          auto ev = event;
          ev.timestamp += headerTimestamp;
          ev.timestamp -= chopperTimestamp;
          events.push_back(ev);
        } break;
      default:
        // Panic!!!!
        logTuple(g_log.error() << "unknow event id ", std::string("0x") + n2hexstr(event.eventId), std::to_string(event.eventId), sizeof(event.eventId)) << "\n";
        break;
      }

     }

//    inline void addTriggerEvent(const TriggerEventData &event) {
//      static const uint8_t DATA_ID_MONITOR_CHOPPER_MIN = 0;
//      static const uint8_t DATA_ID_MONITOR_CHOPPER_Max = 3;
//      events.push_back( { event_id_e::TRIGGER, event, event.timestamp, lastHeader } );
//    }

    inline size_t registerDataBuffer(const BufferHeader &header) {
        //headers.push_back(header);
        headerTimestamp = header.timestamp;
        return 7;//headers.size() - 1;
      }

    inline void postProcessData() {

    }
  //private:
    std::list<Event> events;
    std::vector<BufferHeader> headers;
    //std::map<uint16_t, std::vector<NeutronEvent>> tubes;
    uint64_t chopperTimestamp;
    uint64_t headerTimestamp;

  };

  //void populate_EventWorkspace(EventWorkspace_sptr eventWS, eventAccumulator_t eventList);

  void parse_File(BitStream &file, EventAccumulator &eventAccumulator);
  std::vector<uint8_t> parse_Header(BitStream &file);
  void parse_BlockList(BitStream &file, EventAccumulator &eventAccumulator);
  void parse_Block(BitStream &file, EventAccumulator &eventAccumulator);
  void parse_BlockSeparator(BitStream &file);
  void parse_DataBuffer(BitStream &file, EventAccumulator &eventAccumulator);
  BufferHeader parse_DataBufferHeader(BitStream &file);
  void parse_EventList(BitStream &file, EventAccumulator &eventAccumulator, const HeaderId &headerId, const uint16_t &dataLength);
  Event parse_Event(BitStream &file, const HeaderId &headerId);
  void parse_NeutronEvent(BitStream &file, const BufferHeader &header, EventAccumulator &accumulator);
  void parse_TriggerEvent(BitStream &file, const BufferHeader &header, EventAccumulator &accumulator);
  void parse_EndSignature(BitStream &file);

};

std::ostream& operator<<(std::ostream& os, const LoadDNSEvent::ChannelIndex &obj) {
  return logTuple(os, obj.mcpdId, obj.modId, obj.slotId);
}

std::ostream& operator<<(std::ostream& os, const LoadDNSEvent::EventPosition &obj) {
  return logTuple(os, obj.channel, obj.position);
}

std::ostream& operator<<(std::ostream& os, const LoadDNSEvent::TriggerEventData &obj) {
  return logTuple(os, obj.trigId, obj.dataId, obj.data, obj.timestamp);
}

std::ostream& operator<<(std::ostream& os, const LoadDNSEvent::NeutronEventData &obj) {
  return logTuple(os, obj.amplitude, obj.position, obj.timestamp);
}

std::ostream& operator<<(std::ostream& os, const LoadDNSEvent::NeutronEvent &obj) {
  return logTuple(os, obj.amplitude, obj.tof);
}

}
}

#endif /* MANTID_DATAHANDLING_LoadDNSEvent_H_ */
