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

#include <fstream>
#include <iostream>

#include "MantidKernel/DllConfig.h"
#include <Poco/ConsoleChannel.h>

#include "MantidPythonInterface/core/PythonSink.h"

#include <boost/iostreams/stream.hpp>

/// static boost stream with python sink
static boost::iostreams::stream<pysys_stdout_sink> pysys_stdout{
    pysys_stdout_sink()};

namespace Poco {

static std::ofstream test_ostream("whatever.txt", std::ofstream::out);

class MANTID_KERNEL_DLL PythonStdoutChannel : public ConsoleChannel {
public:
  /// Constructor for PythonStdoutChannel
  PythonStdoutChannel();

  void nice() {
    std::cout << "\n\n"
              << "nice "
              << "\n";
    test_ostream << "nice nice\n";
  }
};
} // namespace Poco
