#include "MantidKernel/Logger.h"
#include <iostream>

class LoggingCleaner {
public:
  /**
   * Cleanly shutdown the logging system on test shutdown.
   */
  ~LoggingCleaner() { Mantid::Kernel::Logger::shutdown(); }
};

LoggingCleaner theCleaner;
