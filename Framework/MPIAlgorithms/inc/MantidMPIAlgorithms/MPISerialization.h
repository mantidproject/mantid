#ifndef MPI_SERIALIZE_HPP___
#define MPI_SERIALIZE_HPP___

#include "MantidMPIAlgorithms/GatherWorkspaces.h"
#include "boost/serialization/split_free.hpp"

BOOST_SERIALIZATION_SPLIT_FREE(Mantid::DataObjects::EventList)

namespace boost {
namespace serialization {
/*! Method that does serialization -- splits to load/save
 *  */
/*template<class Archive>
inline void serialize(Archive & ar,
                Mantid::DataObjects::EventList &elist,
                   const unsigned int file_version)
{
  split_free(ar, elist, file_version);
} */
template <class Archive>
void save(Archive &ar, const Mantid::DataObjects::EventList &elist,
          const unsigned int /*version*/) {
  int etype;
  switch (elist.getEventType()) {
  case Mantid::API::TOF: {
    etype = 1;
    ar &etype;
    std::vector<Mantid::DataObjects::TofEvent> events = elist.getEvents();
    int evsize = static_cast<int>(events.size());
    ar &evsize;
    std::vector<Mantid::DataObjects::TofEvent>::iterator itev;
    std::vector<Mantid::DataObjects::TofEvent>::iterator itev_end =
        events.end();
    for (itev = events.begin(); itev != itev_end; ++itev) {
      double tof = itev->tof();
      ar &tof;
      int64_t time = itev->pulseTime().totalNanoseconds();
      ar &time;
    }
    break;
  }
  case Mantid::API::WEIGHTED: {
    etype = 2;
    ar &etype;
    std::vector<Mantid::DataObjects::WeightedEvent> events =
        elist.getWeightedEvents();
    int evsize = static_cast<int>(events.size());
    ar &evsize;
    std::vector<Mantid::DataObjects::WeightedEvent>::iterator itev;
    std::vector<Mantid::DataObjects::WeightedEvent>::iterator itev_end =
        events.end();
    for (itev = events.begin(); itev != itev_end; ++itev) {
      double tof = itev->tof();
      ar &tof;
      int64_t time = itev->pulseTime().totalNanoseconds();
      ar &time;
      double weight = itev->weight();
      ar &weight;
      double errSq = itev->errorSquared();
      ar &errSq;
    }
    break;
  }
  case Mantid::API::WEIGHTED_NOTIME: {
    etype = 3;
    ar &etype;
    std::vector<Mantid::DataObjects::WeightedEventNoTime> events =
        elist.getWeightedEventsNoTime();
    int evsize = static_cast<int>(events.size());
    ar &evsize;
    std::vector<Mantid::DataObjects::WeightedEventNoTime>::iterator itev;
    std::vector<Mantid::DataObjects::WeightedEventNoTime>::iterator itev_end =
        events.end();
    for (itev = events.begin(); itev != itev_end; ++itev) {
      double tof = itev->tof();
      ar &tof;
      double weight = itev->weight();
      ar &weight;
      double errSq = itev->errorSquared();
      ar &errSq;
    }
    break;
  }
  }
}
template <class Archive>
void load(Archive &ar, Mantid::DataObjects::EventList &elist,
          unsigned int /*version*/) {
  int etype = 0;
  ar &etype;
  int evsize;
  ar &evsize;
  switch (etype) {
  case 1: {
    std::vector<Mantid::DataObjects::TofEvent> mylist;
    double tof = 0.0;
    Mantid::Kernel::DateAndTime pulseTime = 0;
    int64_t time = 0;
    for (int ev = 0; ev < evsize; ev++) {
      ar &tof;
      ar &time;
      pulseTime = Mantid::Kernel::DateAndTime(time);
      mylist.push_back(Mantid::DataObjects::TofEvent(tof, pulseTime));
    }
    elist = Mantid::DataObjects::EventList(mylist);
    break;
  }
  case 2: {
    std::vector<Mantid::DataObjects::WeightedEvent> mylist;
    double tof = 0.0;
    Mantid::Kernel::DateAndTime pulseTime = 0;
    int64_t time = 0;
    double weight = 0.0;
    double errSq = 0.0;
    for (int ev = 0; ev < evsize; ev++) {
      ar &tof;
      ar &time;
      ar &weight;
      ar &errSq;
      pulseTime = Mantid::Kernel::DateAndTime(time);
      mylist.push_back(
          Mantid::DataObjects::WeightedEvent(tof, pulseTime, weight, errSq));
    }
    elist = Mantid::DataObjects::EventList(mylist);
    break;
  }
  case 3: {
    std::vector<Mantid::DataObjects::WeightedEventNoTime> mylist;
    double tof = 0.0;
    double weight = 0.0;
    double errSq = 0.0;
    for (int ev = 0; ev < evsize; ev++) {
      ar &tof;
      ar &weight;
      ar &errSq;
      mylist.push_back(
          Mantid::DataObjects::WeightedEventNoTime(tof, weight, errSq));
    }
    elist = Mantid::DataObjects::EventList(mylist);
    break;
  }
  }
}
// load data required for construction and invoke constructor in place
template <class Archive>
inline void load_construct_data(Archive &ar,
                                Mantid::DataObjects::EventList *elist,
                                const unsigned int /*file_version*/
                                ) {
  // default just uses the default constructor to initialize
  // previously allocated memory.
  new (elist) Mantid::DataObjects::EventList();
}
}
}
#endif
