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
struct MockMemory {
  /// @brief Patch the available memory while in scope
  /// @param value The amount of available memory, in kiB, that will be reported.  Default 12 kiB.
  MockMemory(std::size_t value = g_default_value) { enable_mem_override(value); }
  ~MockMemory() { disable_mem_override(); }
  // delete copies and moves to fill out rule-of-five
  MockMemory(MockMemory const &) = delete;
  MockMemory(MockMemory &&) = delete;
  MockMemory &operator=(MockMemory const &) = delete;
  MockMemory &operator=(MockMemory &&) = delete;
};

} // namespace Mantid::TestMemory
