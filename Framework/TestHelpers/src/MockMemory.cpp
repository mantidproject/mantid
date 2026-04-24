#include "MantidFrameworkTestHelpers/MockMemory.h"
#include "MantidKernel/Memory.h"

#include <atomic>
#include <dlfcn.h>
#include <stdexcept>

namespace {
using AvailMemFunc = std::size_t (*)(Mantid::Kernel::MemoryStats const *);
static std::atomic<bool> g_override_availMem{false}; // flag, whether to use the real or mock availMem
static AvailMemFunc g_real_availMem(nullptr);        // the real function, looked up from symbol list
static std::atomic<std::size_t> g_value{Mantid::TestMemory::g_default_value}; // the return value of mocked availMem

static void init_real_availMem() {
  // if not set, set the real function for availMem by looking up its mangled name in the symbol list
  if (!g_real_availMem) {
    // NOTE: deep magic begins here
    char const *mangled = "_ZNK6Mantid6Kernel11MemoryStats8availMemEv";
    void *sym = dlsym(RTLD_NEXT, mangled);
    if (sym) {
      g_real_availMem = reinterpret_cast<AvailMemFunc>(sym);
    }
    // end deep magic
  }
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
  if (g_override_availMem) {
    return g_value;
  } else {
    init_real_availMem();
    if (g_real_availMem) {
      return g_real_availMem(this);
    } else {
      // if it cannot be thrown, this can cause opaque testing errors; throw an error here instead
      throw std::runtime_error("Failed to reset the MemoryStats patch by name lookup");
    }
  }
}
