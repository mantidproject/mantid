/**
 *
 * Uses a similar method to the plugin loading to ensure that the
 *FrameworkManager is
 * called when an executable loads.
 *
 * To use this simply compile this file along with the other source files that
 * generate the executable/library. When the library is loaded/executable
 *started
 * the "static" object will ensure that the FrameworkManager is started
 */
#include "MantidKernel/RegistrationHelper.h"
#include "MantidAPI/FrameworkManager.h"

namespace {
Mantid::Kernel::RegistrationHelper
    start_framework(((Mantid::API::FrameworkManager::Instance()), 0));
}
