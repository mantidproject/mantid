// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>

namespace Mantid {
namespace Kernel {
/**
  This file provides forward declarations for Mantid::Kernel::PropertyManager
*/

/// forward declare of Mantid::Kernel::PropertyManager
class PropertyManager;
/// shared pointer to Mantid::Kernel::PropertyManager
using PropertyManager_sptr = std::shared_ptr<PropertyManager>;
/// shared pointer to Mantid::Kernel::PropertyManager(const version)
using PropertyManager_const_sptr = std::shared_ptr<const PropertyManager>;
/// unique pointer to Mantid::Kernel::PropertyManager
using PropertyManager_uptr = std::unique_ptr<PropertyManager>;
/// unique pointer to Mantid::Kernel::PropertyManager (const version)
using PropertyManager_const_uptr = std::unique_ptr<const PropertyManager>;

} // namespace Kernel
} // namespace Mantid
