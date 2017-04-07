#ifndef LIVELISTENERFACTORYTEST_H_
#define LIVELISTENERFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Instantiator.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "LiveListenerTest.h"
#include <Poco/Path.h>

using namespace Mantid;
using namespace Mantid::API;

/**
 * Definition for our own Instantiator type for use with the
 * LiveListenerFactory dynamic factory. The default DynamicFactory subscribe
 * peforms hidden object creation! That makes proper use of mocking impossible.
 */
class DLLExport MockLiveListenerInstantiator
    : public Mantid::Kernel::Instantiator<MockLiveListener,
                                          Mantid::API::ILiveListener> {
public:
  MockLiveListenerInstantiator(
      boost::shared_ptr<Mantid::API::ILiveListener> product)
      : m_product(product) {}

  boost::shared_ptr<Mantid::API::ILiveListener>
  createInstance() const override {
    return m_product;
  }

  Mantid::API::ILiveListener *createUnwrappedInstance() const override {
    return m_product.get();
  }

private:
  boost::shared_ptr<Mantid::API::ILiveListener> m_product;
};

class LiveListenerFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LiveListenerFactoryTest *createSuite() {
    return new LiveListenerFactoryTest();
  }
  static void destroySuite(LiveListenerFactoryTest *suite) { delete suite; }

  LiveListenerFactoryTest()
      : factory(LiveListenerFactory::Instance()),
        m_product(boost::make_shared<MockLiveListener>()) {
    // Subscribe the mock implementation created in ILiveListenerTest.h
    factory.subscribe("MockLiveListener",
                      new MockLiveListenerInstantiator{m_product});
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

    m_product = boost::make_shared<MockLiveListener>();

    EXPECT_CALL(*m_product, connect(testing::_))
        .Times(0); // We do not ask this to connect.
    TS_ASSERT_THROWS_NOTHING(l = factory.create("MockLiveListener", false))
    // Check it's really the right class
    TS_ASSERT(boost::dynamic_pointer_cast<MockLiveListener>(l))

    // Check that unregistered class request throws
    TS_ASSERT_THROWS(factory.create("fdsfds", false),
                     Mantid::Kernel::Exception::NotFoundError)
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_product.get()));
  }

  void xtest_create_throws_when_unable_to_connect() {
    EXPECT_CALL(*m_product, connect(testing::_))
        .WillOnce(testing::Return(false /*cannot connect*/));
    Kernel::ConfigService::Instance().setFacility("TEST");
    TS_ASSERT_THROWS(factory.create("MINITOPAZ", true), std::runtime_error);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_product.get()));
    // Now test that it doesn't throw if we ask not to connect
    EXPECT_CALL(*m_product, connect(testing::_)).Times(0);
    TS_ASSERT_THROWS_NOTHING(factory.create(
        "MINITOPAZ",
        false)); // But we won't ask it to connect so all should be fine.
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_product.get()));
  }

  void xtest_createUnwrapped_throws() {
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
  boost::shared_ptr<MockLiveListener> m_product;
  LiveListenerFactoryImpl &factory;
};

#endif /* LIVELISTENERFACTORYTEST_H_ */
