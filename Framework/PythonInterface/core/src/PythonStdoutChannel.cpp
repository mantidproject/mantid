// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/PythonStdoutChannel.h"
#include <Poco/Message.h>
#include <iostream>

#include <boost/iostreams/categories.hpp> // sink_tag
#include <iosfwd>                         // streamsize

#include <fstream>

// test_ostream  std::cout

namespace Poco {

PythonStdoutChannel::PythonStdoutChannel() : ConsoleChannel(pysys_stdout) {}

} // namespace Poco
