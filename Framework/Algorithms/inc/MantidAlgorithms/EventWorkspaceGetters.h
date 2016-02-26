#ifndef MANTID_ALGORITHMS_EVENTWORKSPACEGETTERS_H_
#define MANTID_ALGORITHMS_EVENTWORKSPACEGETTERS_H_

#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
namespace Getters {
static auto eventList =
    std::mem_fn((DataObjects::EventList &
                 (DataObjects::EventWorkspace::*)(const std::size_t)) &
                DataObjects::EventWorkspace::getEventList);
}
}
}

#endif /* MANTID_ALGORITHMS_EVENTWORKSPACEGETTERS_H_ */
