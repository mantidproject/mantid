// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+

#include "MantidFrameworkTestHelpers/MockMemory.h"

// only define the methods for the patch if using Linux
#if defined(__linux__) || defined(__gnu_linux__)

#include "MantidKernel/Memory.h"

#include <atomic>
#include <dlfcn.h>
#include <stdexcept>

namespace {
using AvailMemFunc = std::size_t (*)(Mantid::Kernel::MemoryStats const *);
static std::atomic<bool> g_override_availMem{false};       // flag, whether to use the real or mock availMem
static std::atomic<AvailMemFunc> g_real_availMem(nullptr); // the real function, looked up from symbol list
static std::atomic<std::size_t> g_value{Mantid::TestMemory::g_default_value}; // the return value of mocked availMem
static std::once_flag g_init_flag;

static void init_real_availMem() {
  // NOTE: begin Deep Magic
  // if not set, set the real function for availMem by looking up its mangled name in the symbol list
  std::call_once(g_init_flag, []() {
    // the mangled name may be found with: nm -D bin/libMantidKernel.so | grep availMem
    char const *const mangled = "_ZNK6Mantid6Kernel11MemoryStats8availMemEv";
    void *sym = dlsym(RTLD_NEXT, mangled);
    // fallback to RTLD_DEFAULT if still not found
    if (!sym) {
      sym = dlsym(RTLD_DEFAULT, mangled);
    }
    if (sym) {
      g_real_availMem.store(reinterpret_cast<AvailMemFunc>(sym));
    }
  });
  // end Deep Magic
}
} // namespace

namespace Mantid::TestMemory {
extern "C" void enable_mem_override(std::size_t value) {
  g_value.store(value);
  g_override_availMem.store(true);
}
extern "C" void disable_mem_override() { g_override_availMem.store(false); }
} // namespace Mantid::TestMemory

// this is a patched availMem which will be used in testing.
// when g_override_availMem has been set, it will return a fixed value
// otherwise, it will prepare the real function from symbol lookup,
// and return the actual memory count
std::size_t Mantid::Kernel::MemoryStats::availMem() const {
  if (g_override_availMem.load()) {
    return g_value.load();
  } else {
    init_real_availMem();
    if (g_real_availMem.load()) {
      return g_real_availMem.load()(this);
    } else {
      // if it cannot be thrown, this can cause opaque testing errors; throw an error here instead
      throw std::runtime_error("Failed to reset the MemoryStats patch by name lookup");
    }
  }
}

std::string Mantid::Kernel::MemoryStats::checkAvailableMemory(std::size_t const requestedMemory,
                                                              bool const compareToTotalMemory,
                                                              double const fraction) const {
  if (g_override_availMem.load()) {
    // g_value is the controllable mocked memory basis in KiB. We need the fraction here because, e.g.
    // with a 1 GiB mock ceiling, Rebin can accept a 0.9 GiB request even though production’s 80% limit
    // rejects it, allowing false-positive integration tests.
    std::size_t ceiling = static_cast<std::size_t>(fraction * static_cast<double>(g_value.load()) * 1024.0);
    if (requestedMemory > ceiling) {
      return "Mock Memory Failure";
    } else {
      return "";
    }
  } else {
    // Mirror the real implementation: pick the memory basis and scale it by the fraction. totalMem() is not
    // patched, so it resolves to the real value; availMem() is patched, so reach the real one via lookup.
    std::size_t basisKiB;
    if (compareToTotalMemory) {
      basisKiB = this->totalMem();
    } else {
      init_real_availMem();
      if (!g_real_availMem.load()) {
        // Throw an error here if it doesn't work, otherwise this can cause opaque testing errors
        throw std::runtime_error("Failed to reset the MemoryStats patch by name lookup");
      }
      basisKiB = g_real_availMem.load()(this);
    }
    // basisKiB is in KiB, requestedMemory is in bytes, so multiply
    std::size_t avail = static_cast<std::size_t>(fraction * static_cast<double>(basisKiB) * 1024.0);
    if (requestedMemory > avail) {
      return "Requested Memory Failure " + std::to_string(requestedMemory) + " > " + std::to_string(avail);
    } else {
      return "";
    }
  }
}

#else
namespace Mantid::TestMemory {
extern "C" void enable_mem_override(std::size_t value) { (void)value; }
extern "C" void disable_mem_override() {}
} // namespace Mantid::TestMemory
#endif
