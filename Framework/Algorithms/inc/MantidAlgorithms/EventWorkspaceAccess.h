// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
