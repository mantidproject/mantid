#include "MantidKernel/Logger.h"

class LoggingCleaner {
public:
  /**
   * Cleanly shutdown the logging system on test shutdown.
   */
  ~LoggingCleaner() { Mantid::Kernel::Logger::shutdown(); }
};

LoggingCleaner theCleaner;
