#ifndef MANTID_ALGORITHMS_GETTERS_H_
#define MANTID_ALGORITHMS_GETTERS_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
namespace Getters {
static auto x = std::mem_fn(
    (std::vector<double> & (API::MatrixWorkspace::*)(const std::size_t)) &
    API::MatrixWorkspace::dataX);
static auto eventList =
    std::mem_fn((DataObjects::EventList &
                 (DataObjects::EventWorkspace::*)(const std::size_t)) &
                DataObjects::EventWorkspace::getEventList);
}
}
}

#endif /*MANTID_ALGORITHMS_GETTERS_H_*/
