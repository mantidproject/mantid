// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Logger.h"
#include "MantidKernel/PythonStdoutChannel.h"
#include <Poco/AutoPtr.h>
#include <Poco/Logger.h>
#include <Poco/NullChannel.h>

#include <boost/iostreams/categories.hpp> // sink_tag
#include <iosfwd>                         // streamsize

using namespace Mantid::Kernel;

#include <ostream>
#include <iostream>
#include <boost/iostreams/stream.hpp>
#include <Poco/ConsoleChannel.h>
#include <boost/iostreams/device/file.hpp>

// FIXME #include <Python.h>

namespace io = boost::iostreams;

class my_sink {
public:
    typedef char      char_type;
    typedef io::sink_tag  category;

    std::streamsize write(const char* s, std::streamsize n)
    {
        // Write up to n characters to the underlying
        // data sink into the buffer s, returning the
        // number of characters written
        std::cout << s;
    }

    /* Other members */
};

class PythonStdoutChannelTest : public CxxTest::TestSuite {
public:
  void testContructor() {
    TS_ASSERT_THROWS_NOTHING(Poco::PythonStdoutChannel a;)
  }

  void testRadomlyBoostStream() {
    // Start an instance with boost stream
    boost::iostreams::stream<pysys_stdout_sink> pysys_stdout;

    Poco::AutoPtr<Poco::PythonStdoutChannel> stdoutChannel2(
        new Poco::PythonStdoutChannel);
    stdoutChannel2->nice();
    Poco::ConsoleChannel *testconsole2 = new Poco::ConsoleChannel(pysys_stdout);

    my_sink test_sink = my_sink();
    // io::stream<my_sink> mystream{test_sink};  NO GOOD

    typedef io::stream<io::file_sink> ofstream;

   ofstream out("HeavyArtillery.txt"); // Wilfred Owen
//    out << "Reach at that Arrogance which needs thy harm,\n"
//           "And beat it down before its sins grow worse.\n";
//    out.close();

    std::ofstream fb;
    fb.open ("test.txt",std::ios::out);

    fb << "Test again" << "\n";

//    std::ostream os(&fb);
//     os << "Test sentence\n";
    fb.close();

//    std::ostream stdstream;
//    stdstream << "abc edf" << "\n";
//    std::cout << stdstream.tostring();

    Poco::AutoPtr<Poco::PythonStdoutChannel> stdoutChannel(
        new Poco::PythonStdoutChannel);
    stdoutChannel->nice();
    Poco::ConsoleChannel *testconsole = new Poco::ConsoleChannel(out);

    // TS_ASSERT_EQUALS(1, 3);


  }

  void testLogMessage() {
    // Save root channel
    Poco::Channel *rootChannel = Poco::Logger::root().getChannel();

    std::stringstream obuffer, lbuffer;
    // Save cout's and clog's buffers here
    std::streambuf *obuf = std::cout.rdbuf();
    std::streambuf *lbuf = std::clog.rdbuf();
    // Redirect cout to buffer or any other ostream
    std::cout.rdbuf(obuffer.rdbuf());
    std::clog.rdbuf(lbuffer.rdbuf());
    // root logger has empty name
    Logger log("");

    // Test null channel first
    Poco::AutoPtr<Poco::NullChannel> nullChannel(new Poco::NullChannel);
    Poco::Logger::root().setChannel(nullChannel);

    log.error() << "Error Message 1\n";
    // cout and clog should be empty

    TS_ASSERT_EQUALS(obuffer.str(), "");
    TS_ASSERT_EQUALS(lbuffer.str(), "");

    // reset
    obuffer.str("");
    lbuffer.str("");

    // Test console channel: all go to the log channel but not std::cout channel
    Poco::AutoPtr<Poco::ConsoleChannel> consoleChannel(
        new Poco::ConsoleChannel);
    Poco::Logger::root().setChannel(consoleChannel);
    log.notice() << "[Notice]\n" << "[Notice] 2\n";
    log.error() << "Error Message 2\n";
    // the error should be in std::clog (or std:err)
    std::string finalout = lbuffer.str();

    TS_ASSERT_EQUALS(obuffer.str(), "");
    TS_ASSERT_EQUALS(lbuffer.str(), "[Notice]\n[Notice] 2\nError Message 2\n");

    // reset
    obuffer.str("");
    lbuffer.str("");

    // Test customized channel
    Poco::AutoPtr<Poco::PythonStdoutChannel> stdoutChannel(
        new Poco::PythonStdoutChannel);
    Poco::Logger::root().setChannel(stdoutChannel);
    log.error() << "Error Message 3\n";
    log.information() << "[Notice]\n" << "[Notice] 3\n";
    // the error should be in std::cout
    std::string notice3 = obuffer.str();
    // FIXME TS_ASSERT_EQUALS(obuffer.str(), "Error Message 3\n");
    // FIXME TS_ASSERT_EQUALS(lbuffer.str(), "[Notice]\n[Notice] 2\n");

    // When done redirect cout to its old self
    // Otherwise, it can cause segmentation fault
    std::cout.rdbuf(obuf);
    std::clog.rdbuf(lbuf);
    // set back the channel on root
    Poco::Logger::root().setChannel(rootChannel);

    std::cout << "\n[notice 2]: " << finalout << "\n";
    std::cout << "\n[notice 3]: " << notice3 << "\n";
  }
};
