#ifndef LIVELISTENERFACTORYTEST_H_
#define LIVELISTENERFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "LiveListenerTest.h"
#include <Poco/Path.h>

using namespace Mantid;
using namespace Mantid::API;

class LiveListenerFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LiveListenerFactoryTest *createSuite() {
    return new LiveListenerFactoryTest();
  }
  static void destroySuite(LiveListenerFactoryTest *suite) { delete suite; }

  LiveListenerFactoryTest() : factory(LiveListenerFactory::Instance()) {
    // Subscribe the mock implementation created in ILiveListenerTest.h
    factory.subscribe<MockLiveListener>("MockLiveListener");
  }

  void setUp() override {
    auto &config = Kernel::ConfigService::Instance();
    Poco::Path testFile =
        Poco::Path(config.getInstrumentDirectory())
            .resolve("IDFs_for_UNIT_TESTING/UnitTestFacilities.xml");
    // Load the test facilities file
    config.updateFacilities(testFile.toString());
  }

  void tearDown() override {
    // Restore the main facilities file
    Kernel::ConfigService::Instance()
        .updateFacilities(); // no file loads the default
  }

  void test_create() {
    // Check that we can successfully create a registered class
    boost::shared_ptr<ILiveListener> l;
    TS_ASSERT_THROWS_NOTHING(l = factory.create("MockLiveListener", false))
    // Check it's really the right class
    TS_ASSERT(boost::dynamic_pointer_cast<MockLiveListener>(l))

    // Check that unregistered class request throws
    TS_ASSERT_THROWS(factory.create("fdsfds", false),
                     Mantid::Kernel::Exception::NotFoundError)
  }

  void test_create_throws_when_unable_to_connect() {
    Kernel::ConfigService::Instance().setFacility("TEST");
    TS_ASSERT_THROWS(factory.create("MINITOPAZ", true), std::runtime_error);
    // Now test that it doesn't throw if we ask not to connect
    TS_ASSERT_THROWS_NOTHING(factory.create("MINITOPAZ", false));
  }

  void test_createUnwrapped_throws() {
    // Make sure this method just throws.
    // Note that you can't even access the method unless you assign to a
    // DynamicFactory reference.
    // I.e. LiveListenerFactory::Instance().createUnwrapped would not compile
    Mantid::Kernel::DynamicFactory<ILiveListener> &f =
        LiveListenerFactory::Instance();
    TS_ASSERT_THROWS(f.createUnwrapped(""),
                     Mantid::Kernel::Exception::NotImplementedError)
  }

private:
  LiveListenerFactoryImpl &factory;
};

#endif /* LIVELISTENERFACTORYTEST_H_ */
