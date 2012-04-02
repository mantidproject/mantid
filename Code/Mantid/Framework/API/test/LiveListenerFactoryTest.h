#ifndef LIVELISTENERFACTORYTEST_H_
#define LIVELISTENERFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "ILiveListenerTest.h"

using namespace Mantid;
using namespace Mantid::API;

class LiveListenerFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LiveListenerFactoryTest *createSuite() { return new LiveListenerFactoryTest(); }
  static void destroySuite( LiveListenerFactoryTest *suite ) { delete suite; }

  LiveListenerFactoryTest() : factory(LiveListenerFactory::Instance())
  {
    // Subscribe the mock implementation created in ILiveListenerTest.h
    factory.subscribe<MockILiveListener>("MockILiveListener");
  }

  void test_create()
  {
    // Check that we can successfully create a registered class
    boost::shared_ptr<ILiveListener> l;
    TS_ASSERT_THROWS_NOTHING( l = factory.create("MockILiveListener") )
    // Check it's really the right class
    TS_ASSERT( boost::dynamic_pointer_cast<MockILiveListener>(l) )

    // Check that unregistered class request throws
    TS_ASSERT_THROWS( factory.create("fdsfds"), Mantid::Kernel::Exception::NotFoundError )
  }

  void test_create_throws_when_unable_to_connect()
  {
    Kernel::ConfigService::Instance().setFacility("TEST");
    TS_ASSERT_THROWS( factory.create("MINITOPAZ"), std::runtime_error );
  }

  void test_createUnwrapped_throws()
  {
    // Make sure this method just throws.
    // Note that you can't even access the method unless you assign to a DynamicFactory reference.
    // I.e. LiveListenerFactory::Instance().createUnwrapped would not compile
    Mantid::Kernel::DynamicFactory<ILiveListener> & f = LiveListenerFactory::Instance();
    TS_ASSERT_THROWS(f.createUnwrapped(""), Mantid::Kernel::Exception::NotImplementedError )
  }

private:
  LiveListenerFactoryImpl& factory;
};


#endif /* LIVELISTENERFACTORYTEST_H_ */
