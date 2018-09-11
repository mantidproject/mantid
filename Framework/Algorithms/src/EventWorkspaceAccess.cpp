#include "MantidAlgorithms/EventWorkspaceAccess.h"

namespace Mantid {
namespace Algorithms {

///@cond Doxygen has problems for decltype for some reason.
/// Returns std::mem_fn object refering to EventWorkspace::getSpectrum().
decltype(std::mem_fn(
    (DataObjects::EventList &
     (DataObjects::EventWorkspace::*)(const std::size_t)) &
    DataObjects::EventWorkspace::getSpectrum)) EventWorkspaceAccess::eventList =
    std::mem_fn((DataObjects::EventList &
                 (DataObjects::EventWorkspace::*)(const std::size_t)) &
                DataObjects::EventWorkspace::getSpectrum);
///@endcond
} // namespace Algorithms
} // namespace Mantid
