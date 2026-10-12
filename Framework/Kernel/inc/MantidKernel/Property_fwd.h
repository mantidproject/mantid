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

class Property;

/// Shared pointer to Property
using Property_sptr = std::shared_ptr<Property>;
/// Shared pointer to Property (const version)
using Property_const_sptr = std::shared_ptr<const Property>;
/// Unique pointer to Property
using Property_uptr = std::unique_ptr<Property>;
/// Unique pointer to Property (const version)
using Property_const_uptr = std::unique_ptr<const Property>;

} // namespace Kernel
} // namespace Mantid
