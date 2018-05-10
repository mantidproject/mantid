#ifndef MANTID_LIVEDATA_FAKEEVENTDATALISTENERTEST_H_
#define MANTID_LIVEDATA_FAKEEVENTDATALISTENERTEST_H_

#include "MantidAPI/LiveListenerFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include <Poco/Thread.h>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using Mantid::Kernel::CPUTimer;

class FakeEventDataListenerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FakeEventDataListenerTest *createSuite() {
    return new FakeEventDataListenerTest();
  }
  static void destroySuite(FakeEventDataListenerTest *suite) { delete suite; }

  FakeEventDataListenerTest() {
    // Create the listener. Remember: this will call connect()
    fakel =
        LiveListenerFactory::Instance().create("FakeEventDataListener", true);
  }

  void testProperties() {
    TS_ASSERT(fakel)
    TS_ASSERT_EQUALS(fakel->name(), "FakeEventDataListener")
    TS_ASSERT(!fakel->supportsHistory())
    TS_ASSERT(fakel->buffersEvents())
    TS_ASSERT(fakel->isConnected())
  }

  void testStart() {
    // Nothing much to test just yet
    TS_ASSERT_THROWS_NOTHING(fakel->start(0))
  }

  void testRunStatus() {
    TS_ASSERT_EQUALS(fakel->runStatus(), ILiveListener::Running)
  }

  void testExtractData() {
    using namespace Mantid::DataObjects;
    Workspace_const_sptr buffer;
    Poco::Thread::sleep(100);
    TS_ASSERT_THROWS_NOTHING(buffer = fakel->extractData())
    // Check this is the only surviving reference to it
    TS_ASSERT_EQUALS(buffer.use_count(), 1)
    // Check it's an event workspace
    EventWorkspace_const_sptr evbuf =
        boost::dynamic_pointer_cast<const EventWorkspace>(buffer);
    TS_ASSERT(evbuf)
    // Check the workspace has the correct dimension
    TS_ASSERT_EQUALS(evbuf->getNumberHistograms(), 2)
    // Should be around 20 events, but this can vary a lot on some platforms so
    // just check there's something
    TS_ASSERT_LESS_THAN(1, evbuf->getNumberEvents())

    Poco::Thread::sleep(100);
    // Call it again, and check things again
    TS_ASSERT_THROWS_NOTHING(buffer = fakel->extractData())
    // Check this is the only surviving reference to it
    TS_ASSERT_EQUALS(buffer.use_count(), 1)
    // Check it's a different workspace to last time
    TS_ASSERT_DIFFERS(buffer.get(), evbuf.get())
    // Check it's an event workspace
    evbuf = boost::dynamic_pointer_cast<const EventWorkspace>(buffer);
    TS_ASSERT(evbuf)
    // Check the workspace has the correct dimension
    TS_ASSERT_EQUALS(evbuf->getNumberHistograms(), 2)
    // Should be around 20 events, but this can vary a lot on some platforms so
    // just check there's something
    TS_ASSERT_LESS_THAN(1, evbuf->getNumberEvents())
  }

  /** Call the extractData very quickly to try to trip up
   * the thread.
   */
  void testThreadSafety() {
    using namespace Mantid::DataObjects;
    Workspace_const_sptr buffer;
    Poco::Thread::sleep(100);

    CPUTimer tim;
    size_t num = 10000;
    for (size_t i = 0; i < num; i++) {
      TS_ASSERT_THROWS_NOTHING(buffer = fakel->extractData(););
      // Check it's a valid workspace
      TS_ASSERT(buffer)
    }
    std::cout << tim << " to call extactData() " << num << " times\n";
  }

private:
  boost::shared_ptr<ILiveListener> fakel;
};

#endif /* MANTID_LIVEDATA_FAKEEVENTDATALISTENERTEST_H_ */
