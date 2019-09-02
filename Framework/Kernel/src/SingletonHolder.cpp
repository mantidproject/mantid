// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/SingletonHolder.h"
#include <list>

namespace Mantid {
namespace Kernel {

namespace {
/// List of functions to call on program exit
using CleanupList = std::list<SingletonDeleterFn>;

CleanupList &cleanupList() {
  static CleanupList cleanup;
  return cleanup;
}

/// Intended to be registered to atexit() that will clean up
/// all registered singletons
/// This function may be registed with atexit() more than once, so it needs to
/// clear the list once it has called all the functions
void cleanupSingletons() {
  auto &deleters = cleanupList();
  for (auto &deleter : deleters) {
    deleter();
  }
  deleters.clear();
}
} // namespace

/// Adds singleton cleanup function to our atexit list
/// functions are added to the start of the list so on deletion it is last in,
/// first out
/// @param func :: Exit function to call - the singleton destructor function
void deleteOnExit(SingletonDeleterFn func) {
  auto &deleters = cleanupList();
  if (deleters.empty()) {
    atexit(&cleanupSingletons);
  }
  deleters.push_front(func);
}
} // namespace Kernel
} // namespace Mantid
