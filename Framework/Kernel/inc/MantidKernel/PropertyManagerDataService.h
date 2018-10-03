// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_PROPERTYMANAGERDATASERVICE_
#define MANTID_KERNEL_PROPERTYMANAGERDATASERVICE_

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
class MANTID_KERNEL_DLL PropertyManagerDataServiceImpl final
    : public DataService<PropertyManager> {
public:
  PropertyManagerDataServiceImpl(const PropertyManagerDataServiceImpl &) =
      delete;
  PropertyManagerDataServiceImpl &
  operator=(const PropertyManagerDataServiceImpl &) = delete;

private:
  friend struct Mantid::Kernel::CreateUsingNew<PropertyManagerDataServiceImpl>;
  /// Constructor
  PropertyManagerDataServiceImpl();
  ~PropertyManagerDataServiceImpl() override = default;
};

EXTERN_MANTID_KERNEL template class MANTID_KERNEL_DLL
    Mantid::Kernel::SingletonHolder<PropertyManagerDataServiceImpl>;
using PropertyManagerDataService =
    Mantid::Kernel::SingletonHolder<PropertyManagerDataServiceImpl>;

} // Namespace Kernel
} // Namespace Mantid
#endif /*MANTID_KERNEL_PROPERTYMANAGERDATASERVICE_*/
