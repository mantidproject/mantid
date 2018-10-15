// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Logger.h"

class LoggingCleaner {
public:
  /**
   * Cleanly shutdown the logging system on test shutdown.
   */
  ~LoggingCleaner() { Mantid::Kernel::Logger::shutdown(); }
};

LoggingCleaner theCleaner;
