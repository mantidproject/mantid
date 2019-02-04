// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_PROPERTYMANAGER_FWD_H
#define MANTID_KERNEL_PROPERTYMANAGER_FWD_H

#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace Kernel {
/**
  This file provides forward declarations for Mantid::Kernel::PropertyManager
*/

/// forward declare of Mantid::Kernel::PropertyManager
class PropertyManager;
/// shared pointer to Mantid::Kernel::PropertyManager
using PropertyManager_sptr = boost::shared_ptr<PropertyManager>;
/// shared pointer to Mantid::Kernel::PropertyManager(const version)
using PropertyManager_const_sptr = boost::shared_ptr<const PropertyManager>;
/// unique pointer to Mantid::Kernel::PropertyManager
using PropertyManager_uptr = std::unique_ptr<PropertyManager>;
/// unique pointer to Mantid::Kernel::PropertyManager (const version)
using PropertyManager_const_uptr = std::unique_ptr<const PropertyManager>;

} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_PROPERTYMANAGER_FWD_H
