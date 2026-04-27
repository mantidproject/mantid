// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+

#include "MantidFrameworkTestHelpers/MockMemory.h"
#include "MantidKernel/Memory.h"

#include <atomic>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <stdexcept>

namespace {
using AvailMemFunc = std::size_t (*)(Mantid::Kernel::MemoryStats const *);
static std::atomic<bool> g_override_availMem{false};       // flag, whether to use the real or mock availMem
static std::atomic<AvailMemFunc> g_real_availMem(nullptr); // the real function, looked up from symbol list
static std::atomic<std::size_t> g_value{Mantid::TestMemory::g_default_value}; // the return value of mocked availMem
static std::once_flag g_init_flag;

static void init_real_availMem() {
  // if not set, set the real function for availMem by looking up its mangled name in the symbol list
  std::call_once(g_init_flag, []() {
  // NOTE: The following is Deep Magic.
#if defined(_WIN32) || defined(_WIN64) // windows
    HMODULE hModule = GetModuleHandleA(nullptr);
    if (hModule) {
      // char const *const func_name = "?availMem@MemoryStats@Kernel@Mantid@@QEBA_KXZ";
      char const *const func_name = "Mantid_TestMemory_get_real_availMem";
      FARPROC proc = GetProcAddress(hModule, func_name);
      if (proc) {
        using Getter = void *(*)();
        try {
          Getter getter = reinterpret_cast<Getter>(proc);
          void *raw = getter();
          g_real_availMem.store(reinterpret_cast<AvailMemFunc>(raw));
        } catch (...) {
          throw std::runtime_error("Failed to cast or call the original MemoryStats::availMem");
        }
      } else {
        throw std::runtime_error("Failed to locate the original MemoryStats::availMem");
      }
    } else {
      throw std::runtime_error("Failed to get module handle for MemoryStats::availMem");
    }
#else // unix
    char const *const mangled = "_ZNK6Mantid6Kernel11MemoryStats8availMemEv";
#if defined(__linux__) || defined(__gnu_linux__)
    void *sym = dlsym(RTLD_NEXT, mangled);
#else // on MacOS or other, should use RLTD_DEFAULT
    void *sym = dlsym(RTLD_DEFAULT, mangled);
#endif
    // try fallback to default if not found
    if (!sym) {
      sym = dlsym(RTLD_DEFAULT, mangled);
    }
    if (sym) {
      g_real_availMem.store(reinterpret_cast<AvailMemFunc>(sym));
    }
#endif // win32 or unix
    // end deep magic
  });
}
} // namespace

namespace Mantid::TestMemory {
extern "C" void enable_mem_override(std::size_t value) {
  g_value.store(value);
  g_override_availMem.store(true);
}
extern "C" void disable_mem_override() { g_override_availMem.store(false); }
} // namespace Mantid::TestMemory

// this is a patched availMem which will be used in testing
// when g_override_availMem has been set, it will return a fixed value
// otherwise, it will prepare the real function from symbol lookup, and
// return the actual memory count
std::size_t Mantid::Kernel::MemoryStats::availMem() const {
  if (g_override_availMem.load()) {
    return g_value;
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
