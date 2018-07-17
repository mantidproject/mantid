#include <MantidKernel/SingletonHolder.h>
#include <list>

namespace Mantid {
namespace Kernel {

namespace {
/// List of functions to call on program exit
std::list<deleter_t> DELETERS;
} // namespace

/// Intended to be registered to atexit() that will clean up
/// all registered singletons
/// This function may be registed with atexit() more than once, so it needs to
/// clear the list once it has called all the functions
MANTID_KERNEL_DLL void cleanupSingletons() {
  for (auto &deleter : DELETERS) {
    deleter();
  }
  DELETERS.clear();
}

/// Adds singleton cleanup function to our atexit list
/// functions are added to the start of the list so on deletion it is last in,
/// first out
/// @param func :: Exit function to call - the singleton destructor function
MANTID_KERNEL_DLL void deleteOnExit(deleter_t func) {
  if (DELETERS.empty()) {
    atexit(&cleanupSingletons);
  }
  DELETERS.push_front(func);
}
} // namespace Kernel
} // namespace Mantid
