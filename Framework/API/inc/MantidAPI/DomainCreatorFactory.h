// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
//
// Forward declarations
//
namespace Kernel {
class IPropertyManager;
}
namespace API {
//
// Forward declarations
//
class IDomainCreator;

/**

Constructs a DomainCreator object from a string
 */
class MANTID_API_DLL DomainCreatorFactoryImpl : public Kernel::DynamicFactory<IDomainCreator> {
public:
  /// Returns an initialized domain creator
  IDomainCreator *createDomainCreator(const std::string &id, Kernel::IPropertyManager *pm,
                                      const std::string &workspacePropertyName, const unsigned int domainType) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<DomainCreatorFactoryImpl>;

  /// Private Constructor for singleton class
  DomainCreatorFactoryImpl() = default;

  /// Disable copy and assignment operator
  DomainCreatorFactoryImpl(const DomainCreatorFactoryImpl &) = delete;

  /// No copying
  DomainCreatorFactoryImpl &operator=(const DomainCreatorFactoryImpl &) = delete;

  /// Private Destructor for singleton
  ~DomainCreatorFactoryImpl() override = default;

  // Do not use default methods
  using Kernel::DynamicFactory<IDomainCreator>::create;
  using Kernel::DynamicFactory<IDomainCreator>::createUnwrapped;
};

using DomainCreatorFactory = Mantid::Kernel::SingletonHolder<DomainCreatorFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::DomainCreatorFactoryImpl>;
}
} // namespace Mantid
