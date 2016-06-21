#ifndef MANTID_API_REGISTERFILELOADER_H_
#define MANTID_API_REGISTERFILELOADER_H_

#include "MantidAPI/FileLoaderRegistry.h"

/**
 * DECLARE_FILELOADER_ALGORITHM should be used in place of the standard
 * DECLARE_ALGORITHM macro when writing a file loading algorithm that
 * loads data from a type that is not one of the specialized types. See
 * FileLoaderRegistryImpl::LoaderFormat
 * It both registers the algorithm as usual and subscribes it to the
 * registry.
 */
#define DECLARE_FILELOADER_ALGORITHM(classname)                                \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper reg_loader_##classname(                   \
      (Mantid::API::FileLoaderRegistry::Instance().subscribe<classname>(       \
           Mantid::API::FileLoaderRegistryImpl::Generic),                      \
       0));                                                                    \
  }

/**
 * DECLARE_NEXUS_FILELOADER_ALGORITHM should be used in place of the standard
 * DECLARE_ALGORITHM macro when writing a file loading algorithm that
 * loads data using the Nexus API
 * It both registers the algorithm as usual and subscribes it to the
 * registry.
 */
#define DECLARE_NEXUS_FILELOADER_ALGORITHM(classname)                          \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper reg_hdf_loader_##classname(               \
      (Mantid::API::FileLoaderRegistry::Instance().subscribe<classname>(       \
           Mantid::API::FileLoaderRegistryImpl::Nexus),                        \
       0));                                                                    \
  }

#endif /* MANTID_API_REGISTERFILELOADER_H_ */
