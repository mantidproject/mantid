// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DataService.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"

#include <iosfwd>

namespace Mantid {
namespace Kernel {
// Forward declare
class PropertyManager;

/**
PropertyManagerDataService Class. Derived from DataService.
*/
class MANTID_KERNEL_DLL PropertyManagerDataServiceImpl final : public DataService<PropertyManager> {
public:
  PropertyManagerDataServiceImpl(const PropertyManagerDataServiceImpl &) = delete;
  PropertyManagerDataServiceImpl &operator=(const PropertyManagerDataServiceImpl &) = delete;
  ~PropertyManagerDataServiceImpl() override = default;

private:
  friend struct Mantid::Kernel::CreateUsingNew<PropertyManagerDataServiceImpl>;
  /// Constructor
  PropertyManagerDataServiceImpl();
};

EXTERN_MANTID_KERNEL template class MANTID_KERNEL_DLL Mantid::Kernel::SingletonHolder<PropertyManagerDataServiceImpl>;
using PropertyManagerDataService = Mantid::Kernel::SingletonHolder<PropertyManagerDataServiceImpl>;

} // Namespace Kernel
} // Namespace Mantid
