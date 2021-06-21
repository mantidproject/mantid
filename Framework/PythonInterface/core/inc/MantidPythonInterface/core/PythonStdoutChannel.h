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
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/WrapPython.h"
#include <Poco/ConsoleChannel.h>
#include <iostream>

namespace Poco {

class MANTID_PYTHONINTERFACE_CORE_DLL PythonStdoutChannel : public ConsoleChannel {
public:
  /// Constructor for PythonStdoutChannel
  PythonStdoutChannel();
};

/// std::ostream that redirects to PySys_WriteStdout
class MANTID_PYTHONINTERFACE_CORE_DLL PyOstream {
public:
  PyOstream() : m_ostream(new PyStdoutBuf) {}
  std::ostream m_ostream;

private:
  /// https://stackoverflow.com/questions/772355/how-to-inherit-from-stdostream
  /// Also much better implemented in pybind11::iostream::pythonbuff
  class PyStdoutBuf : public std::streambuf {
  protected:
    int overflow(int c) override {
      Mantid::PythonInterface::GlobalInterpreterLock gil;
      PySys_WriteStdout("%c", c);
      return 0;
    }
  };
};

class MANTID_PYTHONINTERFACE_CORE_DLL PyStdoutChannel : public PyOstream, public ConsoleChannel {
public:
  PyStdoutChannel();
};

} // namespace Poco
