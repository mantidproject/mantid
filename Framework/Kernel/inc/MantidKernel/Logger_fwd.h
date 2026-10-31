// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>

namespace Mantid {
namespace Kernel {

class Logger;

/// Shared pointer to Logger
using Logger_sptr = std::shared_ptr<Logger>;
/// Shared pointer to Logger (const version)
using Logger_const_sptr = std::shared_ptr<const Logger>;
/// Unique pointer to Logger
using Logger_uptr = std::unique_ptr<Logger>;
/// Unique pointer to Logger (const version)
using Logger_const_uptr = std::unique_ptr<const Logger>;

} // namespace Kernel
} // namespace Mantid
