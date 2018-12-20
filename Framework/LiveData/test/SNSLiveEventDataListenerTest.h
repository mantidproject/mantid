#ifndef MANTID_LIVEDATA_SNSLIVEEVENTDATALISTENERTEST_H_
#define MANTID_LIVEDATA_SNSLIVEEVENTDATALISTENERTEST_H_

/* This code is largely based on Russell Taylor's test for the
 * FakeEventDataLister class. */

#include "MantidAPI/LiveListenerFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include <Poco/Thread.h>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using Mantid::Kernel::CPUTimer;

class SNSLiveEventDataListenerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SNSLiveEventDataListenerTest *createSuite() {
    return new SNSLiveEventDataListenerTest();
  }
  static void destroySuite(SNSLiveEventDataListenerTest *suite) {
    delete suite;
  }

  SNSLiveEventDataListenerTest() {
    // Create the listener. Remember: this will call connect()
    sns_l = LiveListenerFactory::Instance().create("SNSLiveEventDataListener");
    // sns_l = LiveListenerFactory::Instance().create("SEQUOIA");
  }

  void testProperties() {
    TS_ASSERT(sns_l)
    TS_ASSERT_EQUALS(sns_l->name(), "SNSLiveEventDataListener")
    TS_ASSERT(!sns_l->supportsHistory())
    TS_ASSERT(sns_l->buffersEvents())
    TS_ASSERT(sns_l->isConnected())
  }

  void testStart() {
    // Nothing much to test just yet
    TS_ASSERT_THROWS_NOTHING(sns_l->start(0))
  }

  void testExtractData() {
    using namespace Mantid::DataObjects;
    Workspace_const_sptr buffer;
    Poco::Thread::sleep(100);
    TS_ASSERT_THROWS_NOTHING(buffer = sns_l->extractData())
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
    TS_ASSERT_THROWS_NOTHING(buffer = sns_l->extractData())
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
      TS_ASSERT_THROWS_NOTHING(buffer = sns_l->extractData(););
      // Check it's a valid workspace
      TS_ASSERT(buffer)
    }
    std::cout << tim << " to call extactData() " << num << " times"
              << std::endl;
  }

private:
  boost::shared_ptr<ILiveListener> sns_l;
};

#endif /* MANTID_LIVEDATA_SNSLIVEEVENTDATALISTENERTEST_H_ */
