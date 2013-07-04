#ifndef MANTID_API_REGISTERFILELOADER_H_
#define MANTID_API_REGISTERFILELOADER_H_

#include "MantidAPI/FileLoaderRegistry.h"

/**
 * DECLARE_FILELOADER_ALGORITHM should be used in place of the standard
 * DECLARE_ALGORITHM macro when writing a file loading algorithm that
 * loads data from a non-hierarchical format.
 * It both registers the algorithm as usual and subscribes it to the
 * registry held in the FrameworkManager
 */
#define DECLARE_FILELOADER_ALGORITHM(classname) \
  namespace \
  {\
    Mantid::Kernel::RegistrationHelper \
      reg_loader_##classname((Mantid::API::\
        FileLoaderRegistry::Instance().subscribe<classname>(Mantid::API::FileLoaderRegistryImpl::NonHDF), 0));\
  }

/**
 * DECLARE_HDF_FILELOADER_ALGORITHM should be used in place of the standard
 * DECLARE_ALGORITHM macro when writing a file loading algorithm that
 * loads data from a hierarchical format, e.g. NeXus, HDF.
 * It both registers the algorithm as usual and subscribes it to the
 * registry held in the FrameworkManager
 */
#define DECLARE_HDF_FILELOADER_ALGORITHM(classname) \
  namespace \
  {\
    Mantid::Kernel::RegistrationHelper \
      reg_hdf_loader_##classname((Mantid::API::\
        FileLoaderRegistry::Instance().subscribe<classname>(Mantid::API::FileLoaderRegistryImpl::HDF), 0)); \
  }


#endif /* MANTID_API_REGISTERFILELOADER_H_ */
