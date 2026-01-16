// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/EventList.h"
#include "MantidTypes/Event/TofEvent.h"
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/vector.hpp>

BOOST_SERIALIZATION_SPLIT_FREE(Mantid::DataObjects::EventList)

namespace boost {
namespace serialization {

template <class Archive>
void save(Archive &ar, const Mantid::DataObjects::EventList &elist, const unsigned int /*version*/) {
  int etype;
  switch (elist.getEventType()) {
  case Mantid::API::TOF: {
    etype = 1;
    ar & etype;
    const auto &events = elist.getEvents();
    auto evsize = static_cast<int>(events.size());
    ar & evsize;
    for (const auto &event : events) {
      double tof = event.tof();
      ar & tof;
      int64_t time = event.pulseTime().totalNanoseconds();
      ar & time;
    }
    break;
  }
  case Mantid::API::WEIGHTED: {
    etype = 2;
    ar & etype;
    const auto &events = elist.getWeightedEvents();
    auto evsize = static_cast<int>(events.size());
    ar & evsize;
    for (const auto &event : events) {
      double tof = event.tof();
      ar & tof;
      int64_t time = event.pulseTime().totalNanoseconds();
      ar & time;
      double weight = event.weight();
      ar & weight;
      double errSq = event.errorSquared();
      ar & errSq;
    }
    break;
  }
  case Mantid::API::WEIGHTED_NOTIME: {
    etype = 3;
    ar & etype;
    const auto &events = elist.getWeightedEventsNoTime();
    auto evsize = static_cast<int>(events.size());
    ar & evsize;
    for (const auto &event : events) {
      double tof = event.tof();
      ar & tof;
      double weight = event.weight();
      ar & weight;
      double errSq = event.errorSquared();
      ar & errSq;
    }
    break;
  }
  }
}

template <class Archive> void load(Archive &ar, Mantid::DataObjects::EventList &elist, unsigned int /*version*/) {
  int etype = 0;
  ar & etype;
  int evsize;
  ar & evsize;
  switch (etype) {
  case 1: {
    std::vector<Mantid::Types::Event::TofEvent> mylist;
    mylist.reserve(evsize);
    double tof = 0.0;
    int64_t time = 0;
    for (int ev = 0; ev < evsize; ev++) {
      ar & tof;
      ar & time;
      Mantid::Types::Core::DateAndTime pulseTime(time);
      mylist.emplace_back(tof, pulseTime);
    }
    elist = Mantid::DataObjects::EventList(mylist);
    break;
  }
  case 2: {
    std::vector<Mantid::DataObjects::WeightedEvent> mylist;
    mylist.reserve(evsize);
    double tof = 0.0;
    int64_t time = 0;
    double weight = 0.0;
    double errSq = 0.0;
    for (int ev = 0; ev < evsize; ev++) {
      ar & tof;
      ar & time;
      ar & weight;
      ar & errSq;
      Mantid::Types::Core::DateAndTime pulseTime(time);
      mylist.emplace_back(tof, pulseTime, weight, errSq);
    }
    elist = Mantid::DataObjects::EventList(mylist);
    break;
  }
  case 3: {
    std::vector<Mantid::DataObjects::WeightedEventNoTime> mylist;
    mylist.reserve(evsize);
    double tof = 0.0;
    double weight = 0.0;
    double errSq = 0.0;
    for (int ev = 0; ev < evsize; ev++) {
      ar & tof;
      ar & weight;
      ar & errSq;
      mylist.emplace_back(tof, weight, errSq);
    }
    elist = Mantid::DataObjects::EventList(mylist);
    break;
  }
  }
}

// load data required for construction and invoke constructor in place
template <class Archive>
inline void load_construct_data(Archive & /*ar*/, Mantid::DataObjects::EventList *elist,
                                const unsigned int /*file_version*/) {
  // default just uses the default constructor to initialize
  // previously allocated memory.
  new (elist) Mantid::DataObjects::EventList();
}

} // namespace serialization
} // namespace boost
