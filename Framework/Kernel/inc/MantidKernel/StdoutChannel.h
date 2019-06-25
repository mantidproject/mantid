// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//
// StdoutChannel.h
//
// Similar to console channel for logging. The output is on std::cout instead of
// std::clog (which is the same as std::cerr)
// Usage: use in it Mantid.properties or mantid.user.properties instead of
// ConsoleChannel class
//
//
//

#ifndef STDOUTCHANNEL_H
#define STDOUTCHANNEL_H

#include "MantidKernel/DllConfig.h"
#include <Poco/ConsoleChannel.h>
namespace Poco {
class MANTID_KERNEL_DLL StdoutChannel : public ConsoleChannel {
public:
  /// Constructor for StdChannel
  StdoutChannel();
};
} // namespace Poco
#endif // STDOUTCHANNEL_H
