// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//
// PythonStdoutChannel.h
//
// Similar to console channel for logging. The output is on std::cout instead of
// std::clog (which is the same as std::cerr)
// Usage: use in it Mantid.properties or mantid.user.properties instead of
// ConsoleChannel class
//
//
//

#pragma once

#include "MantidPythonInterface/core/DllConfig.h"
#include <Poco/ConsoleChannel.h>

namespace Poco {

class MANTID_PYTHONINTERFACE_CORE_DLL PythonStdoutChannel : public ConsoleChannel {
public:
  /// Constructor for PythonStdoutChannel
  PythonStdoutChannel();
};
} // namespace Poco
