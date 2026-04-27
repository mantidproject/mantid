#pragma once
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+

#include <cstddef>

namespace Mantid::TestMemory {

constexpr std::size_t g_default_value{12};

extern "C" void enable_mem_override(std::size_t value = g_default_value);
extern "C" void disable_mem_override();

/// @brief Mock the availMem function with RAII.  Construct an operation within a scope for the
/// availMem function to return a fixed value within that scope.  Will reset on exit.
/// @note on Windows or other deficient OSes, this will not perform the patch, but will just provide
/// the numberOfFloats method to return a very large number expected to always exceed available memory
struct MockMemory {
  /// @brief Patch the available memory while in scope
  /// @param value The amount of available memory, in kiB, that will be reported.  Default 12 kiB.
  MockMemory(std::size_t value = g_default_value) : m_value(value) { enable_mem_override(m_value); }
  ~MockMemory() { disable_mem_override(); }
  // delete copies and moves to fill out rule-of-five
  MockMemory(MockMemory const &) = delete;
  MockMemory(MockMemory &&) = delete;
  MockMemory &operator=(MockMemory const &) = delete;
  MockMemory &operator=(MockMemory &&) = delete;
#if defined(__linux__) || defined(__gnu_linux__)
  std::size_t numberOfFloats() const {
    return static_cast<std::size_t>(static_cast<double>(m_value * 1024) / sizeof(double) + 1);
  }
#else
  // a very large number, sure to always exhaust memory
  std::size_t numberOfFloats() const { return static_cast<std::size_t>(1e12); }
#endif
private:
  std::size_t m_value{g_default_value};
};

} // namespace Mantid::TestMemory
