#ifndef MANTID_FILTERCHANNELTEST_H_
#define MANTID_FILTERCHANNELTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/FilterChannel.h"
#include "MantidKernel/ConfigService.h"
#include "Poco/Channel.h"
#include "Poco/Message.h"
#include "Poco/LoggingFactory.h"
#include "Poco/LoggingRegistry.h"
#include "boost/shared_ptr.hpp"
#include "MantidTestHelpers/TestChannel.h"
#include <map>
#include <string>

using namespace Mantid::Kernel;
using Mantid::TestHelpers::TestChannel;

class FilterChannelTest : public CxxTest::TestSuite
{
public: 

  void testContructor()
  {
    TS_ASSERT_THROWS_NOTHING
    (
      Poco::FilterChannel a;
    )
  }

  void testContructorDefaults()
  {
    Poco::FilterChannel a;
    TestChannel* empty=0;
    TS_ASSERT_EQUALS(a.getPriority(),8);
	  TS_ASSERT_EQUALS(a.getChannel(),empty);
  }

  void testSetPriority()
  {
    Poco::FilterChannel a;
    TS_ASSERT_EQUALS(a.getPriority(),8);
    
    TS_ASSERT_EQUALS(a.setPriority("prio_fatal").getPriority(),1);
    TS_ASSERT_EQUALS(a.setPriority("prio_trace").getPriority(),8);
    TS_ASSERT_EQUALS(a.setPriority("fatal").getPriority(),1);
    TS_ASSERT_EQUALS(a.setPriority("trace").getPriority(),8);
    TS_ASSERT_EQUALS(a.setPriority("FATAL").getPriority(),1);
    TS_ASSERT_EQUALS(a.setPriority("tRaCe").getPriority(),8);

    
    TS_ASSERT_EQUALS(a.setPriority("FATAL").getPriority(),1);
    TS_ASSERT_EQUALS(a.setPriority("CRITICAL").getPriority(),2);
    TS_ASSERT_EQUALS(a.setPriority("ERROR").getPriority(),3);
    TS_ASSERT_EQUALS(a.setPriority("WARNING").getPriority(),4);
    TS_ASSERT_EQUALS(a.setPriority("NOTICE").getPriority(),5);
    TS_ASSERT_EQUALS(a.setPriority("INFORMATION").getPriority(),6);
    TS_ASSERT_EQUALS(a.setPriority("DEBUG").getPriority(),7);
    TS_ASSERT_EQUALS(a.setPriority("TRACE").getPriority(),8);

    //abbreviations
    TS_ASSERT_EQUALS(a.setPriority("WARN").getPriority(),4);
    TS_ASSERT_EQUALS(a.setPriority("INFO").getPriority(),6);
  }
  
  void testSetPriorityThroughProperty()
  {
    Poco::FilterChannel a;
    TS_ASSERT_EQUALS(a.getPriority(),8);
    
    a.setProperty("level","prio_fatal");
    TS_ASSERT_EQUALS(a.getPriority(),1);
    a.setProperty("level","prio_trace");
    TS_ASSERT_EQUALS(a.getPriority(),8);
    a.setProperty("level","fatal");
    TS_ASSERT_EQUALS(a.getPriority(),1);
    a.setProperty("level","trace");
    TS_ASSERT_EQUALS(a.getPriority(),8);
    a.setProperty("level","FATAL");
    TS_ASSERT_EQUALS(a.getPriority(),1);
    a.setProperty("level","tRaCe");
    TS_ASSERT_EQUALS(a.getPriority(),8);

    
    a.setProperty("level","FATAL");
    TS_ASSERT_EQUALS(a.getPriority(),1);
    a.setProperty("level","CRITICAL");
    TS_ASSERT_EQUALS(a.getPriority(),2);
    a.setProperty("level","ERROR");
    TS_ASSERT_EQUALS(a.getPriority(),3);
    a.setProperty("level","WARNING");
    TS_ASSERT_EQUALS(a.getPriority(),4);
    a.setProperty("level","NOTICE");
    TS_ASSERT_EQUALS(a.getPriority(),5);
    a.setProperty("level","INFORMATION");
    TS_ASSERT_EQUALS(a.getPriority(),6);
    a.setProperty("level","DEBUG");
    TS_ASSERT_EQUALS(a.getPriority(),7);
    a.setProperty("level","TRACE");
    TS_ASSERT_EQUALS(a.getPriority(),8);

    //abbreviations
    a.setProperty("level","WARN");
    TS_ASSERT_EQUALS(a.getPriority(),4);
    a.setProperty("level","INFO");
    TS_ASSERT_EQUALS(a.getPriority(),6);
  }

  void testAddChannel()
  {
    boost::shared_ptr<TestChannel> tChannel(new TestChannel);
    Poco::FilterChannel a;
    TestChannel* empty=0;
    
	  TS_ASSERT_EQUALS(a.getChannel(),empty);
	  a.addChannel(tChannel.get());
	  TS_ASSERT_EQUALS(a.getChannel(),tChannel.get());
  }
  
  void testLogMessage()
  {
    boost::shared_ptr<TestChannel> tChannel(new TestChannel);
    Poco::FilterChannel a;
	  a.addChannel(tChannel.get());
	  Poco::Message msg;
	  a.log(msg);
	  TS_ASSERT_EQUALS(tChannel->list().size(),1);
  }

  void testLogMessagesByPriority()
  {
    //initialise the channel
    boost::shared_ptr<TestChannel> tChannel(new TestChannel);
    Poco::FilterChannel a;
	  a.addChannel(tChannel.get());

    //create a priority map
    typedef std::map<unsigned int,std::string> priorityMap;
    priorityMap pMap;
    pMap.insert(priorityMap::value_type(1,"FATAL"));
    pMap.insert(priorityMap::value_type(2,"CRITICAL"));
    pMap.insert(priorityMap::value_type(3,"ERROR"));
    pMap.insert(priorityMap::value_type(4,"WARNING"));
    pMap.insert(priorityMap::value_type(5,"NOTICE"));
    pMap.insert(priorityMap::value_type(6,"INFORMATION"));
    pMap.insert(priorityMap::value_type(7,"DEBUG"));
    pMap.insert(priorityMap::value_type(8,"TRACE"));

    
	  Poco::Message msg;
    int totalCount=0;

    priorityMap::iterator iter;   
		for( iter = pMap.begin(); iter != pMap.end(); ++iter) 
		{
      int channelPriority = (*iter).first;
      std::string priorityString = (*iter).second;
      a.setPriority(priorityString);

      for (unsigned int msgPriority = 0; msgPriority < 8; ++msgPriority)
      {
        msg.setPriority(static_cast<Poco::Message::Priority>(msgPriority));

        int previousMessageCount = tChannel->list().size();
	      a.log(msg);
        int addedMessageCount = tChannel->list().size() - previousMessageCount;

        if ((channelPriority >= msgPriority)&&(addedMessageCount==1))
        {
          //count should have increased
          ++totalCount;
        }
        else if ((channelPriority < msgPriority)&&(addedMessageCount==0))
        {
          //count should not have increased and that is good
        }
        else
        {
          //something else happened and that is bad
          if (addedMessageCount==1)
            TS_FAIL("Message incorrectly passed the filter criteria");
          else
            TS_FAIL("Message incorrectly stopped by the filter criteria");
          std::cerr<<"Message Priority="<<msgPriority<<std::endl;
          std::cerr<<"Channel Priority="<<channelPriority<<std::endl;
          std::cerr<<"addedMessageCount="<<addedMessageCount<<std::endl;
        }
      }
    }

	  TS_ASSERT_EQUALS(tChannel->list().size(),totalCount);
  }

  void testAddChannelThroughProperty()
  {
    Poco::FilterChannel a;
    TestChannel* empty=0;
    //initialise the test channel and put it into the registry
    TestChannel* tChannel = new TestChannel;
    Poco::LoggingRegistry::defaultRegistry().registerChannel("tChannel", tChannel);
    
	  TS_ASSERT_EQUALS(a.getChannel(),empty);
	  a.setProperty("channel","tChannel");

    Poco::Channel* createdChannel = a.getChannel();
    TestChannel* castedTestChannel = dynamic_cast<TestChannel*>(createdChannel);
	  TS_ASSERT_EQUALS(castedTestChannel,tChannel);
    
	  Poco::LoggingRegistry::defaultRegistry().unregisterChannel("tChannel");
	  tChannel->release();
  }

  void testCreateThroughFactory()
  {
    //Ensure that the ConfigService has started as this registers the FilterChannel into the factory
    ConfigService::Instance();

    Poco::FilterChannel* empty=0;
   
    Poco::Channel* createdChannel = Poco::LoggingFactory::defaultFactory().createChannel("FilterChannel");
    Poco::FilterChannel* castedFilterChannel = dynamic_cast<Poco::FilterChannel*>(createdChannel);
	  TS_ASSERT_DIFFERS(castedFilterChannel,empty);
	  createdChannel->release();
  }

};

#endif /*MANTID_FILTERCHANNELTEST_H_*/
