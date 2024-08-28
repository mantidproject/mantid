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
#include <Poco/AutoPtr.h>
#include <Poco/Logger.h>
#include <cxxtest/TestSuite.h>
#include <filesystem>

// standard
#include <fstream>

using Poco::PythonStdoutChannel;

class PythonStdoutChannelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PythonStdoutChannelTest *createSuite() { return new PythonStdoutChannelTest(); }
  static void destroySuite(PythonStdoutChannelTest *suite) { delete suite; }

  void testConstructor() { PythonStdoutChannel(); }

  /// write log message to a file via redirection of python sys.stdout
  void testPySysWriteStdout() {
    // set the root logger's channel to a PythonStdoutChannel
    Poco::Channel *channelOld = Poco::Logger::root().getChannel();
    Poco::AutoPtr<Poco::PythonStdoutChannel> channelNew(new PythonStdoutChannel);
    Poco::Logger::root().setChannel(channelNew);

    // redirect python's sys.stdout to a temporary file, using a python script
    std::string script = "import sys\n"
                         "stdout_old = sys.stdout\n"                           // backup the standard file descriptor
                         "sys.stdout = open(r'TEMPFILE', 'w', buffering=1)\n"; // redirection, small buffer needed
    auto tmpFilePath = std::filesystem::temp_directory_path() / "testPySysWriteStdout.txt";
    replaceSubstring(script, "TEMPFILE", tmpFilePath.string());
    PyRun_SimpleString(script.c_str()); // execute the python script

    // log a message with the root logger that now uses the PythonStdoutChannel
    Mantid::Kernel::Logger log("");
    std::string loggedMessage("Error Message");
    log.error() << loggedMessage << "\n";

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

    Poco::Logger::root().setChannel(channelOld); // restore the channel
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
