#ifndef MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGYTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGYTEST_H_

#include <cxxtest/TestSuite.h>
#include "ReflMainViewMockObjects.h"
#include "MantidQtCustomInterfaces/ReflMeasureTransferStrategy.h"
#include "MantidQtCustomInterfaces/ReflMeasurementSource.h"
#include <memory>
#include <gmock/gmock.h>

using namespace testing;

class MockReflMeasurementSource
    : public MantidQt::CustomInterfaces::ReflMeasurementSource {
public:
  MOCK_CONST_METHOD1(
      obtain, MantidQt::CustomInterfaces::Measurement(const std::string &));
  MOCK_CONST_METHOD0(clone, MockReflMeasurementSource *());
  ~MockReflMeasurementSource() {}
};

using MantidQt::CustomInterfaces::ReflMeasureTransferStrategy;

class ReflMeasureTransferStrategyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflMeasureTransferStrategyTest *createSuite() {
    return new ReflMeasureTransferStrategyTest();
  }
  static void destroySuite(ReflMeasureTransferStrategyTest *suite) {
    delete suite;
  }

  void test_clone() {

    // Sub component ICatalogInfo will be cloned
    auto pCatInfo = new MockICatalogInfo;
    EXPECT_CALL(*pCatInfo, clone()).WillOnce(Return(new MockICatalogInfo));

    // Sub component Measurment source will be cloned
    auto pMeasurementSource = new MockReflMeasurementSource;
    EXPECT_CALL(*pMeasurementSource, clone())
        .WillOnce(Return(new MockReflMeasurementSource));

    // Create it
    ReflMeasureTransferStrategy strategy(
        std::move(std::unique_ptr<MockICatalogInfo>(pCatInfo)),
        std::move(
            std::unique_ptr<MockReflMeasurementSource>(pMeasurementSource)));
    // Clone it
    auto *clone = strategy.clone();
    TS_ASSERT(dynamic_cast<ReflMeasureTransferStrategy *>(clone));
    delete clone;
  }

  void test_filtering() {

    ReflMeasureTransferStrategy strategy(
        std::unique_ptr<MockICatalogInfo>(new MockICatalogInfo),
        std::unique_ptr<MockReflMeasurementSource>(
            new MockReflMeasurementSource));

    // ISIS nexus format files can have the right logs.
    TSM_ASSERT("Yes this transfer mechanism should know about nexus formats",
               strategy.knownFileType("madeup.nxs"));

    // Raw files do not have the necessary logs
    TSM_ASSERT("No this transfer mechanism should know about anything but "
               "nexus formats",
               !strategy.knownFileType("madeup.raw"));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGYTEST_H_ */
