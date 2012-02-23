#ifndef MANTID_DATAHANDLING_FAKEEVENTDATALISTENERTEST_H_
#define MANTID_DATAHANDLING_FAKEEVENTDATALISTENERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <Poco/Thread.h>

using namespace Mantid::API;

class FakeEventDataListenerTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FakeEventDataListenerTest *createSuite() { return new FakeEventDataListenerTest(); }
  static void destroySuite( FakeEventDataListenerTest *suite ) { delete suite; }

  FakeEventDataListenerTest()
  {
    // Create the listener. Remember: this will call connect()
    fakel = LiveListenerFactory::Instance().create("FakeEventDataListener");
  }

  void testProperties()
  {
    TS_ASSERT( fakel )
    TS_ASSERT_EQUALS( fakel->name(), "FakeEventDataListener")
    TS_ASSERT( ! fakel->supportsHistory() )
    TS_ASSERT( fakel->buffersEvents() )
    TS_ASSERT( fakel->isConnected() )
  }

  void testStart()
  {
    // Nothing much to test just yet
    TS_ASSERT_THROWS_NOTHING( fakel->start(0) )
  }

  void testExtractData()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspace_const_sptr buffer;
    Poco::Thread::sleep(100);
    TS_ASSERT_THROWS_NOTHING( buffer = fakel->extractData())
    // Check this is the only surviving reference to it
    TS_ASSERT_EQUALS( buffer.use_count(), 1 )
    // Check it's an event workspace
    EventWorkspace_const_sptr evbuf = boost::dynamic_pointer_cast<const EventWorkspace>(buffer);
    TS_ASSERT( evbuf )
    // Check the events are there
    TS_ASSERT_EQUALS( evbuf->getNumberHistograms(), 2 )
    // Should be around 200 events
    TS_ASSERT_LESS_THAN( evbuf->getNumberEvents(), 25 )
    TS_ASSERT_LESS_THAN( 15, evbuf->getNumberEvents() )

    Poco::Thread::sleep(100);
    // Call it again, and check things again
    TS_ASSERT_THROWS_NOTHING( buffer = fakel->extractData())
    // Check this is the only surviving reference to it
    TS_ASSERT_EQUALS( buffer.use_count(), 1 )
    // Check it's a different workspace to last time
    TS_ASSERT_DIFFERS( buffer.get(), evbuf.get() )
    // Check it's an event workspace
    evbuf = boost::dynamic_pointer_cast<const EventWorkspace>(buffer);
    TS_ASSERT( evbuf )
    // Check the events are there
    TS_ASSERT_EQUALS( evbuf->getNumberHistograms(), 2 )
    // Should be around 200 events
    TS_ASSERT_LESS_THAN( evbuf->getNumberEvents(), 25 )
    TS_ASSERT_LESS_THAN( 15, evbuf->getNumberEvents() )
  }

private:
  boost::shared_ptr<ILiveListener> fakel;
};


#endif /* MANTID_DATAHANDLING_FAKEEVENTDATALISTENERTEST_H_ */
