#ifndef MANTID_ALGORITHMS_EVENTWORKSPACEACCESS_H_
#define MANTID_ALGORITHMS_EVENTWORKSPACEACCESS_H_

#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
struct EventWorkspaceAccess {
  static decltype(
      std::mem_fn((DataObjects::EventList &
                   (DataObjects::EventWorkspace::*)(const std::size_t)) &
                  DataObjects::EventWorkspace::getSpectrum)) eventList;
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_EVENTWORKSPACEACCESS_H_ */
