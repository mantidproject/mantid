#ifndef MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGYTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGYTEST_H_

#include <cxxtest/TestSuite.h>
#include "ReflMainViewMockObjects.h"
#include "MantidQtCustomInterfaces/ReflMeasureTransferStrategy.h"
#include "MantidQtCustomInterfaces/ReflMeasurementSource.h"
#include "MantidQtCustomInterfaces/ReflTableSchema.h"
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

    // Search result inforation not used in the following since we mock the
    // return from the measurementSource
    SearchResultMap data;
    data.insert(
        std::make_pair<std::string, SearchResult>("111", SearchResult()));

    auto mockMeasurementSource = new MockReflMeasurementSource;
    // We expect that we are going to fetch the  measurement data for every
    // search result.
    EXPECT_CALL(*mockMeasurementSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillRepeatedly(Return(Measurement("a", "s_a", "l", "t", 0, "111")));

    auto mockCatInfo = new MockICatalogInfo;
    // We expect that every location will be translated/transformed to make it
    // os specific
    EXPECT_CALL(*mockCatInfo, transformArchivePath(_))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillRepeatedly(Return(std::string()));

    MockProgressBase progress;
    // We expect a progress update on each transfer
    EXPECT_CALL(progress, doReport(_))
        .Times(Exactly(static_cast<int>(data.size())));

    ReflMeasureTransferStrategy strategy(
        std::move(std::unique_ptr<MockICatalogInfo>(mockCatInfo)),
        std::move(
            std::unique_ptr<MockReflMeasurementSource>(mockMeasurementSource)));

    strategy.transferRuns(data, progress);

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementSource));
  }

  void test_when_two_measurement_ids_match_group_them_but_not_others() {

    // Search result inforation not used in the following since we mock the
    // return from the measurementSource
    SearchResultMap data;
    data.insert(
        std::make_pair<std::string, SearchResult>("111", SearchResult()));
    data.insert(
        std::make_pair<std::string, SearchResult>("112", SearchResult()));
    data.insert(
        std::make_pair<std::string, SearchResult>("113", SearchResult()));

    auto mockMeasurementSource = new MockReflMeasurementSource;
    // We are going to return three SearchResults two have the same measurement
    // id
    EXPECT_CALL(*mockMeasurementSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillOnce(Return(Measurement("m1", "s1", "l1", "t1", 0.1, "111")))
        .WillOnce(Return(Measurement("m1", "s2", "l1", "t1", 0.2, "122")))
        .WillOnce(Return(Measurement("m2", "s2", "l1", "t1", 0.2, "123")));

    auto mockCatInfo = new MockICatalogInfo;
    // We expect that every location will be translated/transformed to make it
    // os specific
    EXPECT_CALL(*mockCatInfo, transformArchivePath(_))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillRepeatedly(Return(std::string()));

    MockProgressBase progress;
    // Expect a progress update
    EXPECT_CALL(progress, doReport(_))
        .Times(Exactly(static_cast<int>(data.size())));

    // Make the transfer stragegy
    ReflMeasureTransferStrategy strategy(
        std::move(std::unique_ptr<MockICatalogInfo>(mockCatInfo)),
        std::move(
            std::unique_ptr<MockReflMeasurementSource>(mockMeasurementSource)));

    // Do the transfer
    auto transferResult = strategy.transferRuns(data, progress);

    // Check the transfer entries
    TSM_ASSERT_EQUALS("Wrong number of rows", 3, transferResult.size());

    for (size_t i = 1; i < transferResult.size(); ++i) {
      TSM_ASSERT_DIFFERS("Runs should be the different for all rows",
                         transferResult[0][ReflTableSchema::RUNS],
                         transferResult[i][ReflTableSchema::RUNS]);
    }

    TSM_ASSERT_EQUALS("Group should be the same for first two rows",
                      transferResult[0][ReflTableSchema::GROUP],
                      transferResult[1][ReflTableSchema::GROUP]);

    TSM_ASSERT_DIFFERS("Group should be different for last rows",
                       transferResult[0][ReflTableSchema::GROUP],
                       transferResult[2][ReflTableSchema::GROUP]);

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementSource));
  }

  void test_when_two_measurement_sub_ids_match_combine_rows() {

    // Search result inforation not used in the following since we mock the
    // return from the measurementSource
    SearchResultMap data;
    data.insert(
        std::make_pair<std::string, SearchResult>("111", SearchResult()));
    data.insert(
        std::make_pair<std::string, SearchResult>("112", SearchResult()));
    data.insert(
        std::make_pair<std::string, SearchResult>("113", SearchResult()));

    auto mockMeasurementSource = new MockReflMeasurementSource;
    // All 3 have same measurment id, but we also have 2 with same sub id.
    EXPECT_CALL(*mockMeasurementSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillOnce(Return(Measurement("m1", "s1", "l1", "t1", 0.1, "111")))
        .WillOnce(Return(Measurement("m1", "s1", "l1", "t1", 0.2, "122")))
        .WillOnce(Return(Measurement("m1", "s2", "l1", "t1", 0.2, "123")));

    auto mockCatInfo = new MockICatalogInfo;
    // We expect that every location will be translated/transformed to make it
    // os specific
    EXPECT_CALL(*mockCatInfo, transformArchivePath(_))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillRepeatedly(Return(std::string()));

    MockProgressBase progress;
    // Expect a progress update
    EXPECT_CALL(progress, doReport(_))
        .Times(Exactly(static_cast<int>(data.size())));

    // Make the transfer stragegy
    ReflMeasureTransferStrategy strategy(
        std::move(std::unique_ptr<MockICatalogInfo>(mockCatInfo)),
        std::move(
            std::unique_ptr<MockReflMeasurementSource>(mockMeasurementSource)));

    // Do the transfer
    auto transferResult = strategy.transferRuns(data, progress);

    // Check the transfer entries
    TSM_ASSERT_EQUALS("Should have two rows", 2, transferResult.size());
    TSM_ASSERT_DIFFERS("Runs should be the different for both columns",
                       transferResult[0][ReflTableSchema::RUNS],
                       transferResult[1][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("Runs should be summed. Sub ids are the same.", "111+122",
                      transferResult[0][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("Other run should be singular.", "123",
                      transferResult[1][ReflTableSchema::RUNS]);

    for (size_t i = 1; i < transferResult.size(); ++i) {
      TSM_ASSERT_EQUALS("All should have the same group",
                        transferResult[0][ReflTableSchema::GROUP],
                        transferResult[i][ReflTableSchema::GROUP]);
    }

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementSource));
  }

  void test_do_not_include_invalid_measurements() {
    // Search result inforation not used in the following since we mock the
    // return from the measurementSource
    SearchResultMap data;
    data.insert(
        std::make_pair<std::string, SearchResult>("111", SearchResult()));

    auto mockMeasurementSource = new MockReflMeasurementSource;
    // We expect that we are going to fetch the  measurement data for every
    // search result.
    EXPECT_CALL(*mockMeasurementSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillRepeatedly(Return(Measurement::InvalidMeasurement("Abort!")));

    auto mockCatInfo = new MockICatalogInfo;
    // We expect that every location will be translated/transformed to make it
    // os specific
    EXPECT_CALL(*mockCatInfo, transformArchivePath(_))
        .Times(Exactly(static_cast<int>(data.size())));

    MockProgressBase progress;
    // Nothing obtained. No progress to report.
    EXPECT_CALL(progress, doReport(_)).Times(Exactly(1));

    ReflMeasureTransferStrategy strategy(
        std::move(std::unique_ptr<MockICatalogInfo>(mockCatInfo)),
        std::move(
            std::unique_ptr<MockReflMeasurementSource>(mockMeasurementSource)));

    auto result = strategy.transferRuns(data, progress);

    TSM_ASSERT_EQUALS("Measurements where invalid. Results should be empty.", 0,
                      result.size());
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
