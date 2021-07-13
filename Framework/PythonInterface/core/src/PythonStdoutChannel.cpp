// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

// local includes
#include "MantidPythonInterface/core/PythonStdoutChannel.h"

// 3rd-party includes
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/WrapPython.h"
#include <boost/format.hpp>
#include <boost/iostreams/categories.hpp> // sink_tag
#include <boost/iostreams/stream.hpp>

#include <iostream> // streamsize

namespace { // anonymous namespace

class PyStdoutSink {
public:
  typedef char char_type;
  typedef boost::iostreams::sink_tag category;

  std::streamsize write(const char *s, std::streamsize n) {
    Mantid::PythonInterface::GlobalInterpreterLock gil; // acquire the GIL
    // PySys_WriteStdout truncates to 1000 chars
    static const std::streamsize MAXSIZE = 1000;

    std::streamsize written = std::min(n, MAXSIZE);
    PySys_WriteStdout((boost::format("%%.%1%s") % written).str().c_str(), s);

    return written;
  } // release the GIL
};

// wrapper of that sink to be a stream
PyStdoutSink pyStdoutSinkInstance = PyStdoutSink(); // needs to be initialized separately
boost::iostreams::stream<PyStdoutSink> PyStdoutStream(pyStdoutSinkInstance);

} // anonymous namespace

namespace Poco {
PythonStdoutChannel::PythonStdoutChannel() : ConsoleChannel(PyStdoutStream) {}
} // namespace Poco
