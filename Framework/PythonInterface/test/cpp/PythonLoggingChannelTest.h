// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// local
#include "MantidPythonInterface/core/PythonLoggingChannel.h"

// 3rd party
#include "MantidKernel/ConfigService.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include <Poco/AutoPtr.h>
#include <Poco/Message.h>
#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/import.hpp>
#include <boost/python/list.hpp>
#include <cxxtest/TestSuite.h>

// standard
#include <string>
#include <vector>

using Mantid::Kernel::ConfigService;
using Poco::PythonLoggingChannel;

class PythonLoggingChannelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PythonLoggingChannelTest *createSuite() { return new PythonLoggingChannelTest(); }
  static void destroySuite(PythonLoggingChannelTest *suite) { delete suite; }

  void setUp() override {
    m_oldQueueSize = ConfigService::Instance().getString(QUEUE_SIZE_KEY);
    Mantid::PythonInterface::GlobalInterpreterLock gil;
    boost::python::exec("import logging\n"
                        "class _CaptureHandler(logging.Handler):\n"
                        "    def __init__(self):\n"
                        "        super().__init__(level=logging.NOTSET)\n"
                        "        self.messages = []\n"
                        "    def emit(self, record):\n"
                        "        self.messages.append(record.getMessage())\n"
                        "_capture_handler = _CaptureHandler()\n"
                        "logging.getLogger('Mantid').addHandler(_capture_handler)\n"
                        "logging.getLogger('Mantid').setLevel(logging.DEBUG)\n",
                        mainNamespace());
  }

  void tearDown() override {
    ConfigService::Instance().setString(QUEUE_SIZE_KEY, m_oldQueueSize);
    Mantid::PythonInterface::GlobalInterpreterLock gil;
    boost::python::exec("logging.getLogger('Mantid').removeHandler(_capture_handler)\n"
                        "logging.getLogger('Mantid').setLevel(logging.NOTSET)\n"
                        "del _capture_handler\n",
                        mainNamespace());
  }

  void testMessagesAreDeliveredOnFlush() {
    Poco::AutoPtr<PythonLoggingChannel> channel(new PythonLoggingChannel);
    channel->log(Poco::Message("test", "first", Poco::Message::PRIO_NOTICE));
    channel->log(Poco::Message("test", "second", Poco::Message::PRIO_ERROR));
    channel->flush();

    TS_ASSERT_EQUALS(capturedMessages(), std::vector<std::string>({"first", "second"}));
  }

  void testCloseDeliversQueuedMessagesAndDiscardsLaterOnes() {
    Poco::AutoPtr<PythonLoggingChannel> channel(new PythonLoggingChannel);
    channel->log(Poco::Message("test", "before close", Poco::Message::PRIO_NOTICE));
    channel->close();
    channel->log(Poco::Message("test", "after close", Poco::Message::PRIO_NOTICE));
    channel->flush();

    TS_ASSERT_EQUALS(capturedMessages(), std::vector<std::string>({"before close"}));
  }

  void testQueueSizeIsReadFromConfig() {
    ConfigService::Instance().setString(QUEUE_SIZE_KEY, "2");
    Poco::AutoPtr<PythonLoggingChannel> channel(new PythonLoggingChannel);
    // the limit is refreshed when the queue is drained
    channel->flush();

    // nothing runs Python between these calls, so all of them are queued before the next drain
    for (int i = 0; i < 5; ++i)
      channel->log(Poco::Message("test", "message " + std::to_string(i), Poco::Message::PRIO_NOTICE));
    channel->flush();

    TS_ASSERT_EQUALS(capturedMessages(),
                     std::vector<std::string>({"PythonLoggingChannel dropped 3 messages because its queue was full",
                                               "message 0", "message 1"}));
  }

  void testQueueSizeZeroIsUnlimited() {
    ConfigService::Instance().setString(QUEUE_SIZE_KEY, "0");
    Poco::AutoPtr<PythonLoggingChannel> channel(new PythonLoggingChannel);
    channel->flush();

    constexpr int numMessages = 20000; // more than the default limit
    for (int i = 0; i < numMessages; ++i)
      channel->log(Poco::Message("test", "message", Poco::Message::PRIO_NOTICE));
    channel->flush();

    TS_ASSERT_EQUALS(capturedMessages().size(), static_cast<size_t>(numMessages));
  }

private:
  static constexpr auto QUEUE_SIZE_KEY = "logging.python.queueSize";

  static boost::python::object mainNamespace() { return boost::python::import("__main__").attr("__dict__"); }

  static std::vector<std::string> capturedMessages() {
    Mantid::PythonInterface::GlobalInterpreterLock gil;
    const boost::python::list messages(mainNamespace()["_capture_handler"].attr("messages"));
    std::vector<std::string> result;
    for (boost::python::ssize_t i = 0; i < boost::python::len(messages); ++i)
      result.emplace_back(boost::python::extract<std::string>(messages[i]));
    return result;
  }

  std::string m_oldQueueSize;
};
