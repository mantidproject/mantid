#include "MantidAlgorithms/EventWorkspaceAccess.h"

namespace Mantid {
namespace Algorithms {

/// Returns std::mem_fn object refering to EventWorkspace::getEventList().
decltype(std::mem_fn((DataObjects::EventList &
                      (DataObjects::EventWorkspace::*)(const std::size_t)) &
                     DataObjects::EventWorkspace::getEventList))
    EventWorkspaceAccess::eventList =
        std::mem_fn((DataObjects::EventList &
                     (DataObjects::EventWorkspace::*)(const std::size_t)) &
                    DataObjects::EventWorkspace::getEventList);
}
}
