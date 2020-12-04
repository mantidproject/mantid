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

//#include <boost/iostreams/categories.hpp> // sink_tag
//#include <iosfwd>                         // streamsize

#include "MantidKernel/PythonSink.h"

using namespace Mantid::Kernel;

#include <Poco/ConsoleChannel.h>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>
#include <ostream>

#include <Python.h>

// namespace io = boost::iostreams;

class PythonStdoutChannelTest : public CxxTest::TestSuite {
public:
  void FtestContructor() {
    TS_ASSERT_THROWS_NOTHING(Poco::PythonStdoutChannel a;)
  }

  /**
   * @brief Test class pysys_stdout_sink and how to use it with stream
   */
  void testPythonSink() {

    // Init
    Py_Initialize();

    // Create sink
    pysys_stdout_sink testsink;

    // Create stream
    boost::iostreams::stream<pysys_stdout_sink> pysys_stdout{testsink};

    pysys_stdout << "Test Test Test";

    // Make it works ConsoleChannel
    Poco::AutoPtr<Poco::PythonStdoutChannel> stdoutChannel(
        new Poco::PythonStdoutChannel);
    stdoutChannel->nice();
    Poco::ConsoleChannel *testconsole = new Poco::ConsoleChannel(pysys_stdout);
    TS_ASSERT(testconsole);

    // Test console channel: all go to the log channel but not std::cout channel
    Poco::AutoPtr<Poco::ConsoleChannel> consoleChannel(
        new Poco::ConsoleChannel{pysys_stdout});

    // Set up the root channel
    Poco::Channel *rootChannel = Poco::Logger::root().getChannel();
    // root logger has empty name
    Logger log("");
    // set console channel
    Poco::Logger::root().setChannel(consoleChannel);
    log.notice() << "[Notice]\n"
                 << "[Notice] 2\n";
    log.error() << "Error Message 2\n";

    // set back the channel on root
    Poco::Logger::root().setChannel(rootChannel);
    // close std out
    pysys_stdout.close();
  }

  /**
   * @brief Test the static Python stdout stream
   */
  void testStaticPythonStream() {
    Py_Initialize();
    pysys_stdout << "Hello, Python world!\n";
    pysys_stdout << "30";
  }

  /**
   * @brief Test Python Std output with logger
   */
  void testPythonStdoutChannelLogMessage() {
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
    log.notice() << "[Notice]\n"
                 << "[Notice] 2\n";
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
    log.information() << "[Notice]\n"
                      << "[Notice] 3\n";
    // the error should be in std::cout
    std::string notice3 = obuffer.str();
    std::string error3 = lbuffer.str();
    TS_ASSERT_EQUALS(obuffer.str(), "");
    TS_ASSERT_EQUALS(lbuffer.str(), "");

    // When done redirect cout to its old self
    // Otherwise, it can cause segmentation fault
    std::cout.rdbuf(obuf);
    std::clog.rdbuf(lbuf);
    // set back the channel on root
    Poco::Logger::root().setChannel(rootChannel);

    std::cout << "\n[notice 2]: " << finalout << "\n";
    std::cout << "\n[notice 3]: " << notice3 << "\n";
    std::cout << "\n[error  3]: " << error3 << "\n";
  }
};
