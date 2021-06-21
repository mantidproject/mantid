// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// local
#include "MantidPythonInterface/core/PythonStdoutChannel.h"

// 3rd party
#include "MantidKernel/Logger.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include <Poco/AutoPtr.h>
#include <Poco/Logger.h>
#include <cxxtest/TestSuite.h>

// standard
#include <filesystem>
#include <fstream>

using Mantid::PythonInterface::GlobalInterpreterLock;
using Poco::PyStdoutChannel;
using Poco::PythonStdoutChannel;

class PythonStdoutChannelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PythonStdoutChannelTest *createSuite() { return new PythonStdoutChannelTest(); }
  static void destroySuite(PythonStdoutChannelTest *suite) { delete suite; }

  void testConstructor() { PyStdoutChannel(); }

  void testStream() {
    // set the root logger's channel to a PyStdoutChannel
    Poco::Channel *channelOld = Poco::Logger::root().getChannel();
    Poco::AutoPtr<Poco::PyStdoutChannel> channelNew(new PyStdoutChannel);
    Poco::Logger::root().setChannel(channelNew);

    // substitute channel's stream buffer with the buffer of a string stream
    std::stringstream recorder;
    std::streambuf *bufferNew = recorder.rdbuf();
    channelNew->m_ostream.rdbuf(bufferNew); // now channel will output to recorder's string object

    // log a message with the root logger. It should be stored in recorder's string object
    Mantid::Kernel::Logger log("");
    log.error() << "Error Message\n";
    TS_ASSERT_EQUALS(recorder.str(), "Error Message\n")

    // restore the channel
    Poco::Logger::root().setChannel(channelOld);
  }

  /// write log message to a file via redirection of python sys.stdout
  void testPySysWriteStdout() {
    // set the root logger's channel to a PyStdoutChannel
    Poco::Channel *channelOld = Poco::Logger::root().getChannel();
    Poco::AutoPtr<Poco::PyStdoutChannel> channelNew(new PyStdoutChannel);
    Poco::Logger::root().setChannel(channelNew);

    // redirect python's sys.stdout to a temporary file, using a python script
    std::string script = "import sys\n"
                         "stdout_old = sys.stdout\n"                          // backup the standard file descriptor
                         "sys.stdout = open('TEMPFILE', 'w', buffering=1)\n"; // redirection, small buffe needed
    auto tmpFilePath = std::filesystem::temp_directory_path() / "testPySysWriteStdout.txt";
    replaceSubstring(script, "TEMPFILE", tmpFilePath.string());
    PyRun_SimpleString(script.c_str()); // execute the python script

    // log a message with the root logger that now uses the PyStdoutChannel
    Mantid::Kernel::Logger log("");
    std::string loggedMessage("Error Message");
    log.error() << loggedMessage << "\n"; // log -> PySys_WriteStdout -> sys.stdout -> tmpFile

    // Now reassign the standard file descriptor to sys.stdout
    std::string revert = "sys.stdout.close()\n"       // close tmpFile
                         "sys.stdout = stdout_old\n"; // reassign original file descriptor
    PyRun_SimpleString(revert.c_str());

    // Fetch the log message from tmpFile
    std::ifstream logFile(tmpFilePath.string());
    std::string message;
    std::getline(logFile, message);
    TS_ASSERT_EQUALS(message, loggedMessage)
    logFile.close();
    std::filesystem::remove(tmpFilePath);

    // restore the channel
    Poco::Logger::root().setChannel(channelOld);
    std::cout << "It is finished!\n";
  }

private:
  bool replaceSubstring(std::string &templatedString, const std::string &target, const std::string &replacement) {
    size_t start_pos = templatedString.find(target);
    if (start_pos == std::string::npos)
      return false;
    templatedString.replace(start_pos, target.length(), replacement);
    return true;
  }
};