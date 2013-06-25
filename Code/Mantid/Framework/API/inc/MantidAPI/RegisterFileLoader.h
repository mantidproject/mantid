#ifndef MANTID_API_REGISTERFILELOADER_H_
#define MANTID_API_REGISTERFILELOADER_H_

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/FrameworkManager.h"

/**
 * DECLARE_FILELOADER_ALGORITHM should be used in place of the standard
 * DECLARE_ALGORITHM macro that both registers the algorithm as usual and subscribes it to the
 * registry held in the FrameworkManager
 */
#define DECLARE_FILELOADER_ALGORITHM(classname) \
  DECLARE_ALGORITHM(classname) \
  namespace \
  {\
    Mantid::Kernel::RegistrationHelper \
      reg_loader_##classname((Mantid::API::FrameworkManager::Instance().fileLoaderRegistry().subscribe(#classname), 0));\
  }

#endif /* MANTID_API_REGISTERFILELOADER_H_ */
