#ifndef MANTID_STDOUTCHANNELTEST_H_
#define MANTID_STDOUTCHANNELTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Logger.h"
#include "MantidKernel/StdoutChannel.h"
#include <Poco/AutoPtr.h>
#include <Poco/Logger.h>
#include <Poco/NullChannel.h>

using namespace Mantid::Kernel;

class StdoutChannelTest : public CxxTest::TestSuite {
public:
  void testContructor() { TS_ASSERT_THROWS_NOTHING(Poco::StdoutChannel a;) }

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
    obuffer.str("");
    lbuffer.str("");

    // Test console channel
    Poco::AutoPtr<Poco::ConsoleChannel> consoleChannel(
        new Poco::ConsoleChannel);
    Poco::Logger::root().setChannel(consoleChannel);
    log.error() << "Error Message 2\n";
    // the error should be in std::clog (or std:err)
    TS_ASSERT_EQUALS(obuffer.str(), "");
    TS_ASSERT_EQUALS(lbuffer.str(), "Error Message 2\n");
    obuffer.str("");
    lbuffer.str("");

    // Test std channel
    Poco::AutoPtr<Poco::StdoutChannel> stdoutChannel(new Poco::StdoutChannel);
    Poco::Logger::root().setChannel(stdoutChannel);
    log.error() << "Error Message 3\n";
    // the error should be in std::cout
    TS_ASSERT_EQUALS(obuffer.str(), "Error Message 3\n");
    TS_ASSERT_EQUALS(lbuffer.str(), "");

    // When done redirect cout to its old self
    std::cout.rdbuf(obuf);
    std::clog.rdbuf(lbuf);
    // set back the channel on root
    Poco::Logger::root().setChannel(rootChannel);
  }
};

#endif /*MANTID_STDOUTCHANNELTEST_H_*/
