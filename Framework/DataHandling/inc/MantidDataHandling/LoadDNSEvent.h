#ifndef MANTID_DATAHANDLING_LoadDNSEvent_H_
#define MANTID_DATAHANDLING_LoadDNSEvent_H_

#include "BitStream.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTypes/Core/DateAndTime.h"


#include <array>
#include <fstream>
#include <limits>
#include <iterator>

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


  typedef uint32_t EventCount;
  //typedef uint16_t ChannelIndex;

public:
  /*
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
*/

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

    int64_t timestamp;
    uint8_t mcpdId;
  };

  struct TofEvent {
    int64_t tof;
    uint16_t channel;
    uint16_t position;

  };

  struct TriggerEventChannel {
    uint8_t trigId;
    uint8_t dataId;
    uint8_t mcpdId;

    bool operator < (TriggerEventChannel const &rhs) const {
      bool isSmaller = dataId < rhs.dataId;
      isSmaller = trigId < rhs.trigId || (trigId == rhs.trigId && isSmaller);
      isSmaller = mcpdId < rhs.mcpdId || (mcpdId == rhs.mcpdId && isSmaller);

      return isSmaller;
    }
  };

  static inline TriggerEventChannel triggerEventChannel(const Event &event) {
    return {event.data.trigger.trigId, event.data.trigger.dataId, event.mcpdId};
  }


private:

  struct ChopperFinder {
    struct DataSourceInfo {
      int64_t min = std::numeric_limits<int>::max();
      int64_t max = std::numeric_limits<int>::min();
      std::map<int64_t, uint64_t> deltaTFrequencies;

      int64_t lastEventTimestamp = std::numeric_limits<int64_t>::max();

      void addEvent(const Event &event, const uint binSize) {
        const int64_t deltaT = event.timestamp - lastEventTimestamp;
        lastEventTimestamp = event.timestamp;

        deltaTFrequencies[(deltaT / binSize) * binSize]++;

        if (deltaT < std::numeric_limits<int64_t>::min() / 2) {
          //discard event;
          return;
        }

        min = std::min(min, deltaT);
        max = std::max(max, deltaT);
      }
    };

    static constexpr uint INPUTS_COUNT = 4;
    static constexpr uint TRIGS_COUNT = 8;
    uint8_t chopperCandidates = uint8_t(uint16_t(1u << INPUTS_COUNT) - 1u);

    std::map<TriggerEventChannel, DataSourceInfo> dataSourceInfos; // used to determin which DataSource is the chopper.

    inline void setChopperCandidate(uint8_t dataId) {
      chopperCandidates = static_cast<uint8_t>(1u << dataId);
    }

    inline bool couldBeChopper(const TriggerEventData &data) {
      return (1u << data.dataId) & chopperCandidates;
    }

    inline bool isChopper(const TriggerEventData &data) {
      return ((1u << data.dataId) & chopperCandidates) == chopperCandidates;
    }

    inline bool didFindChopper() {
      // exactly ONE bit is set:
      return chopperCandidates && !(chopperCandidates & (chopperCandidates-1));
    }

    inline void addEvent(const Event &event, const uint binSize) {
      const TriggerEventData &data = event.data.trigger;
      auto &dsi = dataSourceInfos[triggerEventChannel(event)];
      //dsi.addEvent(event, binSize);
      //return;
      static const int64_t THRESHOLD = 0;
      chopperCandidates = chopperCandidates & ( 0
        | ~(1u << data.dataId)
        | (std::abs(dsi.max -  dsi.min) < THRESHOLD ? 0xFF : 0x00)
      );
    }
  };

  struct EventAccumulator {
    EventAccumulator(Kernel::Logger &g_log, Progress &progress, uint64_t chopperPeriod)
      : g_log(g_log), progress(progress), chopperPeriod(chopperPeriod) { }
  private:
    Kernel::Logger &g_log;
    Progress &progress;

  public:
    int64_t chopperPeriod;

    inline void addNeutronEvent(Event &event) {
        event.timestamp += lastHeader.timestamp;
        event.mcpdId = lastHeader.mcpdId;
        events.push_back(event);
    }

    inline void addTriggerEvent(Event &event) {
      const TriggerEventData &data = event.data.trigger;
      if (!chopperFinder.couldBeChopper(data)) {
        return;
      }
      event.timestamp += lastHeader.timestamp;
      event.mcpdId = lastHeader.mcpdId;
      events.push_back(event);
    }

    inline void registerDataBuffer(const BufferHeader &header) {
        lastHeader = header;
      }

    inline void postProcessData(Mantid::DataObjects::EventWorkspace_sptr eventWS) {
      std::sort(events.begin(), events.end(), [](Event l, Event r){ return l.timestamp < r.timestamp; });
/*
      for (const Event &event : events) {
        if (chopperFinder.didFindChopper()) {
          break;
        }
        if (event.eventId == event_id_e::TRIGGER && chopperFinder.couldBeChopper(event.data.trigger)) {
          //g_log.notice() << "event_id_e::TRIGGER\n";
          chopperFinder.addEvent(event, 1);
        }
      }
*/
      int64_t chopperTimestamp = 0;
      for (const auto &event : events) {
        if ((event.timestamp - chopperTimestamp) > chopperPeriod) { //(event.eventId == event_id_e::TRIGGER && chopperFinder.isChopper(event.data.trigger)) {
          chopperTimestamp = event.timestamp;
        }
        if (event.timestamp - chopperTimestamp != 0) {
          const uint16_t channel = uint16_t(event.mcpdId << 8 | event.data.neutron.modId << 5 | event.data.neutron.slotId);
          const uint16_t position = event.data.neutron.position;
          Mantid::DataObjects::EventList &eventList = eventWS->getSpectrum(0*channel * 960 + position);
          //eventList.switchTo(TOF);
          eventList.addEventQuickly(Types::Event::TofEvent(double(event.timestamp - chopperTimestamp)));
        }
      }
      //events = tmpEvents;
    }

    inline void postProcessDataX() {
      std::sort(events.begin(), events.end(), [](Event l, Event r){ return l.timestamp < r.timestamp; });

      //return;
      for (Event event : events) {
        if (event.eventId == event_id_e::TRIGGER) {
          chopperFinder.addEvent(event, 18/*binSize*/);
        }
      }

      uint64_t chopperTimestamp = 0;
      for (uint i = 0; i < events.size(); i++) {
        Event event = events[i];
        if (event.eventId == event_id_e::TRIGGER) {
          const TriggerEventData data = event.data.trigger;
          if (chopperFinder.couldBeChopper(data)) {
            chopperTimestamp = event.timestamp;
          }
        } else {
          // event.timestamp -= chopperTimestamp;
          // events[i] = event;
        }
      }
    }
  //private:
    //std::map<uint32_t, uint64_t> tofs;
    ChopperFinder chopperFinder;
    std::vector<Event> events;
    std::vector<TofEvent> tmpEvents;
    std::vector<std::map<TriggerEventChannel, Event>> eventsOut;
    BufferHeader lastHeader;
  };

  //template<class EventWorkspace_sptr>
  void populate_EventWorkspace(Mantid::DataObjects::EventWorkspace_sptr eventWS, LoadDNSEvent::EventAccumulator eventAccumulator);

  void parse_File(ByteStream &file, EventAccumulator &eventAccumulator);
  std::vector<uint8_t> parse_Header(ByteStream &file);
  void parse_BlockList(ByteStream &file, EventAccumulator &eventAccumulator);
  void parse_Block(ByteStream &file, EventAccumulator &eventAccumulator);
  void parse_BlockSeparator(ByteStream &file);
  void parse_DataBuffer(ByteStream &file, EventAccumulator &eventAccumulator);
  BufferHeader parse_DataBufferHeader(ByteStream &file);
  void parse_EventList(ByteStream &file, EventAccumulator &eventAccumulator, const uint16_t &dataLength);
  void parse_EventListQuickly(ByteStream &file, EventAccumulator &eventAccumulator, const uint16_t &dataLength);
  Event parse_Event(ByteStream &file, const HeaderId &headerId);
  void parse_EndSignature(ByteStream &file);

  void parse_BlockQuickly(ByteStream &file, EventAccumulator &eventAccumulator);
  void parse_DataBufferQuickly(ByteStream &file, EventAccumulator &eventAccumulator);
};

/*
std::ostream& operator<<(std::ostream& os, const LoadDNSEvent::ChannelIndex &obj) {
  return logTuple(os, obj.mcpdId, obj.modId, obj.slotId);
}

std::ostream& operator<<(std::ostream& os, const LoadDNSEvent::EventPosition &obj) {
  return logTuple(os, obj.channel, obj.position);
}
*/

std::ostream& operator<<(std::ostream& os, const LoadDNSEvent::TriggerEventData &obj) {
  return logTuple(os, obj.trigId, obj.dataId, obj.data);//, obj.timestamp);
}

std::ostream& operator<<(std::ostream& os, const LoadDNSEvent::NeutronEventData &obj) {
  return logTuple(os, obj.amplitude, obj.position);//, obj.timestamp);
}

std::ostream& operator<<(std::ostream& os, const LoadDNSEvent::NeutronEvent &obj) {
  return logTuple(os, obj.amplitude, obj.tof);
}

}
}

#endif /* MANTID_DATAHANDLING_LoadDNSEvent_H_ */
