// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//
PythonStdoutChannel.h
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

#include "MantidKernel/PythonSink.h"

#include <boost/iostreams/stream.hpp>

//#include <iosfwd>                          // streamsize
//#include <boost/iostreams/categories.hpp>  // sink_tag

// FIXME #include <Python.h>

// namespace io = boost::iostreams;

//class pysys_stdout_sink {
//    // from https://marc.info/?l=boost-users&m=124222823630179&w=2
//public:
//   typedef char char_type;
//   typedef boost::iostreams::sink_tag category;

//   std::streamsize write( const char* s, std::streamsize n ) {
//       // PySys_WriteStdout truncates to 1000 chars
//       static const std::streamsize MAXSIZE = 1000;

//       std::streamsize written = std::min( n, MAXSIZE );
//       // FIXME PySys_WriteStdout( (boost::format("%%.%1%s") % written).str().c_str(), s );
//       std::cout << s;

//       return written;
//   }
//};

static boost::iostreams::stream<pysys_stdout_sink> pysys_stdout;

namespace Poco {

static std::ofstream test_ostream("whatever.txt",  std::ofstream::out);


class MANTID_KERNEL_DLL PythonStdoutChannel : public ConsoleChannel {
public:
  /// Constructor for PythonStdoutChannel
  PythonStdoutChannel();

  void nice() {
      std::cout << "\n\n" << "nice " << "\n";
      test_ostream << "nice nice\n";
  }
};
} // namespace Poco
