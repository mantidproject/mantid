// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Logger.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/PythonStdoutChannel.h"
#include <Poco/AutoPtr.h>
#include <Poco/Logger.h>
#include <cxxtest/TestSuite.h>
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
    GlobalInterpreterLock gil; // acquire the GIL, then release it upon calling destructor ~GlobalInterpreterLock

    // set the root logger's channel to a PyStdoutChannel
    Poco::Channel *channelOld = Poco::Logger::root().getChannel();
    Poco::AutoPtr<Poco::PyStdoutChannel> channelNew(new PyStdoutChannel);
    Poco::Logger::root().setChannel(channelNew);

    // redirect python's sys.stdout to a temporary file, using a python script
    std::string script = "import sys\n"
                         "stdout_old = sys.stdout\n"                          // backup the standard file descriptor
                         "sys.stdout = open('TEMPFILE', 'w')\n"               // redirection statement
                         "python_state = open('/tmp/python_state.txt','w')\n" // helper file to check some variables
                         "python_state.write('1 stdout_old = ' + str(stdout_old) + ' ; ')\n"
                         "python_state.write('2 sys.stdout = ' + str(sys.stdout) + ' ; ')\n";
    const std::string tmpFile("/tmp/testPySysWriteStdout.txt"); // TODO: create a temporary file instead
    replaceSubstring(script, "TEMPFILE", tmpFile);
    PyRun_SimpleString(script.c_str()); // execute the python script

    // log a message with the root logger that now uses the PyStdoutChannel
    Mantid::Kernel::Logger log("");
    std::string loggedMessage("Error Message\n");
    log.error() << loggedMessage; // log -> PySys_WriteStdout -> sys.stdout -> tmpFile

    // flush as many buffers I can think of!
    log.flush();
    channelNew->m_ostream.flush();
    std::streambuf *pyBuf = channelNew->m_ostream.rdbuf();
    pyBuf->pubsync(); // https://stackoverflow.com/questions/25833193/c-how-to-flush-stdstringbuf

    // Now reassign the standard file descriptor to sys.stdout
    std::string revert = "python_state.write('3 sys.stdout = ' + str(sys.stdout) + ' ; ')\n"
                         "sys.stdout.flush()\n" // flushing as much as I can think of
                         "sys.stdout.close()\n" // close tmpFile
                         "python_state.write('4 sys.stdout closed? = ' + str(sys.stdout.closed) + ' ; ')\n"
                         "sys.stdout = stdout_old\n" // reassign original file descriptor
                         "python_state.write('5 sys.stdout = ' + str(sys.stdout) + ' ; ')\n"
                         "python_state.close()";
    PyRun_SimpleString(revert.c_str());

    // Fetch the log message from tmpFile
    std::ifstream logFile(tmpFile);
    logFile.open(tmpFile);
    logFile.seekg(0, std::ios::beg);
    std::string message;
    // PROBLEM: at this point, /tmp/testPySysWriteStdout.txt exists but nothing has been yet written to it
    std::getline(logFile, message);
    logFile.close();
    TS_ASSERT_EQUALS(message, loggedMessage)

    // restore the channel
    Poco::Logger::root().setChannel(channelOld);
    std::cout << "It is finished!\n";
  } // PROBLEM: at this point, "Error Message\n" is written to /tmp/testPySysWriteStdout.txt !

  void testPysys() {
    // set the root logger's channel to a PyStdoutChannel
    Poco::Channel *channelOld = Poco::Logger::root().getChannel();
    Poco::AutoPtr<Poco::PyStdoutChannel> channelNew(new PyStdoutChannel);
    Poco::Logger::root().setChannel(channelNew);

    // https://stackoverflow.com/questions/4307187/how-to-catch-python-stdout-in-c-code
    // redirect python's sys.stdout to the buffer of a string stream
    std::string script = "import sys\n\
     class CatchOut:\n\
        def __init__(self):\n\
           self.value = ''\n\
        def write(self, txt):\n\
           self.value += txt\n\
     fisher = CatchOut()\n\
     sys.stdout = fisher\n\
     open('/tmp/junk.txt', 'w').write('hello')\n\
    "; // python code that will record whatever is sent to sys.stdout into catchOut.value

    Py_Initialize();
    PyObject *pythonModule = PyImport_AddModule("__main__"); // create main module
    PyRun_SimpleString(script.c_str());                      // invoke code to redirect
    std::string scriptOther = "open('/tmp/junkOther.txt', 'w').write('hello')\n";
    PyRun_SimpleString(scriptOther.c_str());
    // log a message with the root logger. It should be stored in fisher.value
    Mantid::Kernel::Logger log("");
    log.error() << "Error Message\n";

    // Retrieve the contents of fisher.value
    PyObject *fisher = PyObject_GetAttrString(pythonModule, "fisher"); // get our fisher python object
    PyErr_Print();                                                     // make python print any errors
    PyObject *value = PyObject_GetAttrString(fisher, "value");
    const char *cError = PyUnicode_AsUTF8(value);
    Py_Finalize();

    TS_ASSERT_EQUALS(cError, "Error Message\n")

    // restore the channel
    Poco::Logger::root().setChannel(channelOld);
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