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

class Unit;

/// Shared pointer to the Unit base class
using Unit_sptr = std::shared_ptr<Unit>;
/// Shared pointer to the Unit base class (const version)
using Unit_const_sptr = std::shared_ptr<const Unit>;

} // namespace Kernel
} // namespace Mantid
