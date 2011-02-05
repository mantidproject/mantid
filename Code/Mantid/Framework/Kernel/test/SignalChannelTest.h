#ifndef MANTID_SIGNALCHANNELTEST_H_
#define MANTID_SIGNALCHANNELTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/SignalChannel.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/Channel.h>
#include <Poco/Message.h>
#include <Poco/LoggingFactory.h>
#include <Poco/LoggingRegistry.h>
#include <string>

using namespace Mantid::Kernel;

std::string str = "";
void tst(const Poco::Message& msg)
{
    str = msg.getText();
}

class SignalChannelTest : public CxxTest::TestSuite
{
public: 

  void testContructor()
  {
    TS_ASSERT_THROWS_NOTHING
    (
      Poco::SignalChannel a;
    )
  }

  void testCreateThroughFactory()
  {
    //Ensure that the ConfigService has started as this registers the SignalChannel into the factory
    ConfigService::Instance();

    Poco::SignalChannel* empty=0;
   
    Poco::Channel* createdChannel = Poco::LoggingFactory::defaultFactory().createChannel("SignalChannel");
    Poco::SignalChannel* castedSignalChannel = dynamic_cast<Poco::SignalChannel*>(createdChannel);
	  TS_ASSERT_DIFFERS(castedSignalChannel,empty);
	  createdChannel->release();
  }

  void testConnect()
  {
    Poco::SignalChannel a;
    TS_ASSERT_THROWS_NOTHING
    (
       a.connect(tst);
    )
  }

  void testSendMessage()
  {
    Poco::SignalChannel a;
    a.connect(tst);
    Poco::Message msg;
    msg.setText("TesT");
    a.log(msg);
    TS_ASSERT_EQUALS(str, "TesT")
  }

};

#endif /*MANTID_SIGNALCHANNELTEST_H_*/
