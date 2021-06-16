// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/PythonStdoutChannel.h"
#include "MantidPythonInterface/core/WrapPython.h"

#include <iostream> // streamsize

#include <boost/format.hpp>
#include <boost/iostreams/categories.hpp> // sink_tag
#include <boost/iostreams/stream.hpp>

namespace { // anonymous namespace

// https://stackoverflow.com/questions/772355/how-to-inherit-from-stdostream
class PyStdoutBuf : public std::streambuf {
protected:
  int overflow(int c) override {
    PySys_WriteStdout("%c", c);
    return 0;
  }
};

auto pyStreambuf = new PyStdoutBuf;
auto pyOstream = std::ostream(pyStreambuf);
} // anonymous namespace

namespace Poco {
PythonStdoutChannel::PythonStdoutChannel() : ConsoleChannel(pyOstream) {}
} // namespace Poco
