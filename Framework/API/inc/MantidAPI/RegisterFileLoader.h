// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FileLoaderRegistry.h"

/**
 * DECLARE_FILELOADER_ALGORITHM should be used in place of the standard
 * DECLARE_ALGORITHM macro when writing a file loading algorithm that
 * loads data from a type that is not one of the specialized types. See
 * FileLoaderRegistryImpl::LoaderFormat
 * It both registers the algorithm as usual and subscribes it to the
 * registry.
 */
#define DECLARE_FILELOADER_ALGORITHM(classname)                                                                        \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper reg_loader_##classname(                                                           \
      (Mantid::API::FileLoaderRegistry::Instance().subscribe<classname>(Mantid::API::FileLoaderRegistryImpl::Generic), \
       0));                                                                                                            \
  }

/**
 * DECLARE_LEGACY_NEXUS_FILELOADER_ALGORITHM should be used in place of the standard
 * DECLARE_ALGORITHM macro when writing a file loading algorithm that
 * loads data using the Nexus API
 * It both registers the algorithm as usual and subscribes it to the
 * registry.
 */
#define DECLARE_LEGACY_NEXUS_FILELOADER_ALGORITHM(classname)                                                           \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper                                                                                   \
      reg_hdf_loader_##classname((Mantid::API::FileLoaderRegistry::Instance().subscribe<classname>(                    \
                                      Mantid::API::FileLoaderRegistryImpl::LegacyNexus),                               \
                                  0));                                                                                 \
  }

/**
 * DECLARE_NEXUS_FILELOADER_ALGORITHM should be used in place of the
 * standard DECLARE_ALGORITHM macro when writing a file loading algorithm that
 * loads data using the Nexus API
 * It both registers the algorithm as usual and subscribes it to the
 * registry.
 */
#define DECLARE_NEXUS_FILELOADER_ALGORITHM(classname)                                                                  \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper reg_hdf_loader_##classname(                                                       \
      (Mantid::API::FileLoaderRegistry::Instance().subscribe<classname>(Mantid::API::FileLoaderRegistryImpl::Nexus),   \
       0));                                                                                                            \
  }
