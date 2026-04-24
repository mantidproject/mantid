#pragma once

#include <cstddef>

namespace Mantid::TestMemory {

constexpr std::size_t g_default_value{12};

extern "C" void enable_mem_override(std::size_t value = g_default_value);
extern "C" void disable_mem_override();

/// @brief Mock the availMem function with RAII.  Construct an operation within a scope for the
/// availMem function to return a fixed calue within that scope.  Will reset on exit.
struct MockMemory {
  /// @brief Patch the available memory while in scope
  /// @param value The amount of available memory, in kiB, that will be reported.  Default 12 kiB.
  MockMemory(std::size_t value = g_default_value) { enable_mem_override(value); }
  ~MockMemory() { disable_mem_override(); }
};

} // namespace Mantid::TestMemory
