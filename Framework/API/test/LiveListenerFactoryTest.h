// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "LiveListenerTest.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Instantiator.h"
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>

#include <utility>

using namespace Mantid;
using namespace Mantid::API;

namespace {
/**
 * Definition for our own Instantiator type for use with the
 * LiveListenerFactory dynamic factory. The default DynamicFactory subscribe
 * peforms hidden object creation! That makes proper use of mocking impossible.
 */
class MockLiveListenerInstantiator : public Mantid::Kernel::Instantiator<MockLiveListener, Mantid::API::ILiveListener> {
public:
  MockLiveListenerInstantiator(std::shared_ptr<Mantid::API::ILiveListener> product) : product(std::move(product)) {}

  std::shared_ptr<Mantid::API::ILiveListener> createInstance() const override { return product; }

  Mantid::API::ILiveListener *createUnwrappedInstance() const override { return product.get(); }

private:
  std::shared_ptr<Mantid::API::ILiveListener> product;
};

/**
 * Fake Algorithm. Used to check that Algorithm references are tracked and
 * passed on.
 */
class FakeAlgorithm : public Algorithm {
public:
  void exec() override { /*Do nothing*/ }
  void init() override { /*Do nothing*/ }
  const std::string name() const override { return "FakeAlgorithm"; }
  int version() const override { return 1; }
  const std::string summary() const override { return ""; }
};
} // namespace
class LiveListenerFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LiveListenerFactoryTest *createSuite() { return new LiveListenerFactoryTest(); }
  static void destroySuite(LiveListenerFactoryTest *suite) { delete suite; }

  void setUp() override {
    auto &config = Kernel::ConfigService::Instance();
    Poco::Path testFile = Poco::Path(config.getInstrumentDirectory()).resolve("unit_testing/UnitTestFacilities.xml");
    // Load the test facilities file
    config.updateFacilities(testFile.toString());
  }

  void tearDown() override {
    // Restore the main facilities file
    Kernel::ConfigService::Instance().updateFacilities(); // no file loads the default
  }

  void test_create() {
    // Check that we can successfully create a registered class
    LiveListenerFactoryImpl &factory = LiveListenerFactory::Instance();
    // Subscribe the mock implementation created in ILiveListenerTest.h
    std::shared_ptr<ILiveListener> l;
    auto product = std::make_shared<MockLiveListener>();
    factory.subscribe("MockLiveListener", std::make_unique<MockLiveListenerInstantiator>(product));

    EXPECT_CALL(*product, setAlgorithm(testing::_))
        .Times(0);                                       // variant of create below does not take an Algorithm. So we
                                                         // do not expect the call to be made on the Live Listener.
    EXPECT_CALL(*product, connect(testing::_)).Times(0); // We do not ask this to connect see false below.
    TS_ASSERT_THROWS_NOTHING(l = factory.create("MockLiveListener", false))
    // Check it's really the right class
    TS_ASSERT(std::dynamic_pointer_cast<MockLiveListener>(l))

    // Check that unregistered class request throws
    TS_ASSERT_THROWS(factory.create("fdsfds", false), const Mantid::Kernel::Exception::NotFoundError &)
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(product.get()));
    factory.unsubscribe("MockLiveListener");
  }

  void test_create_with_calling_alg() {
    // Check that we can successfully create a registered class
    LiveListenerFactoryImpl &factory = LiveListenerFactory::Instance();
    // Subscribe the mock implementation created in ILiveListenerTest.h
    std::shared_ptr<ILiveListener> l;
    auto product = std::make_shared<MockLiveListener>();
    factory.subscribe("MockLiveListener", std::make_unique<MockLiveListenerInstantiator>(product));

    EXPECT_CALL(*product, setAlgorithm(testing::_)).Times(1);
    EXPECT_CALL(*product, connect(testing::_)).Times(0); // We do not ask this to connect see false below.

    FakeAlgorithm callingAlg;

    TS_ASSERT_THROWS_NOTHING(l = factory.create("MockLiveListener", false, &callingAlg))

    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(product.get()));
    factory.unsubscribe("MockLiveListener");
  }
  void test_create_throws_when_unable_to_connect() {
    LiveListenerFactoryImpl &factory = LiveListenerFactory::Instance();
    // Subscribe the mock implementation created in ILiveListenerTest.h
    std::shared_ptr<ILiveListener> l;
    auto product = std::make_shared<MockLiveListener>();
    factory.subscribe("MockLiveListener", std::make_unique<MockLiveListenerInstantiator>(product));
    EXPECT_CALL(*product, connect(testing::_)).WillOnce(testing::Return(false /*cannot connect*/));
    Kernel::ConfigService::Instance().setFacility("TEST");
    TS_ASSERT_THROWS(factory.create("MINITOPAZ", true), const std::runtime_error &);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(product.get()));
    // Now test that it doesn't throw if we ask not to connect
    EXPECT_CALL(*product, connect(testing::_)).Times(0);
    TS_ASSERT_THROWS_NOTHING(factory.create("MINITOPAZ",
                                            false)); // But we won't ask it to connect so all should be fine.
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(product.get()));
    factory.unsubscribe("MockLiveListener");
  }

  void test_createUnwrapped_throws() {
    LiveListenerFactoryImpl &factory = LiveListenerFactory::Instance();
    // Subscribe the mock implementation created in ILiveListenerTest.h
    std::shared_ptr<ILiveListener> l;
    auto product = std::make_shared<MockLiveListener>();
    factory.subscribe("MockLiveListener", std::make_unique<MockLiveListenerInstantiator>(product));
    // Make sure this method just throws.
    // Note that you can't even access the method unless you assign to a
    // DynamicFactory reference.
    // I.e. LiveListenerFactory::Instance().createUnwrapped would not compile
    Mantid::Kernel::DynamicFactory<ILiveListener> &f = LiveListenerFactory::Instance();
    TS_ASSERT_THROWS(f.createUnwrapped(""), const Mantid::Kernel::Exception::NotImplementedError &)
    factory.unsubscribe("MockLiveListener");
  }
};
