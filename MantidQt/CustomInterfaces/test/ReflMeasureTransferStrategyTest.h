#ifndef MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGYTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGYTEST_H_

#include <cxxtest/TestSuite.h>
#include "ReflMainViewMockObjects.h"
#include "MantidQtCustomInterfaces/ReflMeasureTransferStrategy.h"
#include "MantidQtCustomInterfaces/ReflMeasurementSource.h"
#include <memory>
#include <gmock/gmock.h>
#include <utility>

using namespace testing;
using namespace MantidQt::CustomInterfaces;

class MockReflMeasurementSource
    : public MantidQt::CustomInterfaces::ReflMeasurementSource {
public:
  MOCK_CONST_METHOD2(obtain, MantidQt::CustomInterfaces::Measurement(
                                 const std::string &, const std::string &));
  MOCK_CONST_METHOD0(clone, MockReflMeasurementSource *());
  ~MockReflMeasurementSource() {}
};

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

  void test_obtain_single_measurement() {

    SearchResultMap data;
    data.insert(std::make_pair<std::string, SearchResult>(
        "111", SearchResult("descr", "location")));

    auto mockMeasurementSource = new MockReflMeasurementSource;
    // We expect that we are going to fetch the  measurement data for every
    // search result.
    EXPECT_CALL(*mockMeasurementSource, obtain(_, _))
        .Times(Exactly(data.size()))
        .WillRepeatedly(Return(Measurement("a", "s_a", "l", "t")));

    auto mockCatInfo = new MockICatalogInfo;
    // We expect that every location will be translated/transformed to make it
    // os specific
    EXPECT_CALL(*mockCatInfo, transformArchivePath(_))
        .Times(Exactly(data.size()))
        .WillRepeatedly(Return(std::string()));

    MockProgressBase progress;
    EXPECT_CALL(progress, doReport(_)).Times(Exactly(data.size()));

    ReflMeasureTransferStrategy strategy(
        std::move(std::unique_ptr<MockICatalogInfo>(mockCatInfo)),
        std::move(
            std::unique_ptr<MockReflMeasurementSource>(mockMeasurementSource)));

    strategy.transferRuns(data, progress);

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementSource));
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
