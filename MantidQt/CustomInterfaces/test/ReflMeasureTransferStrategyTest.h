#ifndef MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGYTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGYTEST_H_

#include <cxxtest/TestSuite.h>
#include "ReflMainViewMockObjects.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMeasureTransferStrategy.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMeasurementItemSource.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableSchema.h"
#include <memory>
#include <gmock/gmock.h>
#include <utility>

using namespace testing;
using namespace MantidQt::CustomInterfaces;

class MockReflMeasurementItemSource
    : public MantidQt::CustomInterfaces::ReflMeasurementItemSource {
public:
  MOCK_CONST_METHOD2(obtain, MantidQt::CustomInterfaces::MeasurementItem(
                                 const std::string &, const std::string &));
  MOCK_CONST_METHOD0(clone, MockReflMeasurementItemSource *());
  ~MockReflMeasurementItemSource() {}
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
    // return from the measurementItemSource
    SearchResultMap data;
    data.insert(
        std::make_pair<std::string, SearchResult>("111", SearchResult()));

    auto mockMeasurementItemSource = new MockReflMeasurementItemSource;
    // We expect that we are going to fetch the  measurement data for every
    // search result.
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillRepeatedly(Return(MeasurementItem("a", "s_a", "l", "t", 0, "111")));

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
            std::unique_ptr<MockReflMeasurementItemSource>(mockMeasurementItemSource)));

    strategy.transferRuns(data, progress);

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource));
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

    auto mockMeasurementItemSource = new MockReflMeasurementItemSource;
    // We are going to return three SearchResults two have the same measurement
    // id
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillOnce(Return(MeasurementItem("m1", "s1", "l1", "t1", 0.1, "111")))
        .WillOnce(Return(MeasurementItem("m1", "s2", "l1", "t1", 0.2, "122")))
        .WillOnce(Return(MeasurementItem("m2", "s2", "l1", "t1", 0.2, "123")));

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
            std::unique_ptr<MockReflMeasurementItemSource>(mockMeasurementItemSource)));

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
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource));
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

    auto mockMeasurementItemSource = new MockReflMeasurementItemSource;
    // All 3 have same measurment id, but we also have 2 with same sub id.
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillOnce(Return(MeasurementItem("m1", "s1", "l1", "t1", 0.1, "111")))
        .WillOnce(Return(MeasurementItem("m1", "s1", "l1", "t1", 0.2, "122")))
        .WillOnce(Return(MeasurementItem("m1", "s2", "l1", "t1", 0.2, "123")));

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
            std::unique_ptr<MockReflMeasurementItemSource>(mockMeasurementItemSource)));

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
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource));
  }
  
    void test_complex_example_two_groups_of_two() {

    // Search result inforation not used in the following since we mock the
    // return from the measurementSource
    SearchResultMap data;
    data.insert(
        std::make_pair<std::string, SearchResult>("14913", SearchResult()));
    data.insert(
        std::make_pair<std::string, SearchResult>("14914", SearchResult()));
    data.insert(
        std::make_pair<std::string, SearchResult>("14915", SearchResult()));
    data.insert(
        std::make_pair<std::string, SearchResult>("14916", SearchResult()));

    auto mockMeasurementItemSource = new MockReflMeasurementItemSource;
    // All 3 have same measurment id, but we also have 2 with same sub id.
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillOnce(Return(MeasurementItem("m1", "s1", "l1", "t1", 0.1, "14913")))
        .WillOnce(Return(MeasurementItem("m1", "s1", "l1", "t1", 0.1, "14914")))
        .WillOnce(Return(MeasurementItem("m2", "s1", "l1", "t1", 0.2, "14915")))
        .WillOnce(Return(MeasurementItem("m2", "s1", "l1", "t1", 0.2, "14916")));

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
            std::unique_ptr<MockReflMeasurementItemSource>(mockMeasurementItemSource)));

    // Do the transfer
    auto transferResult = strategy.transferRuns(data, progress);

    // Check the transfer entries
    TSM_ASSERT_EQUALS("Should have two rows", 2, transferResult.size());
    TSM_ASSERT_DIFFERS("Runs should be the different for both columns",
                       transferResult[0][ReflTableSchema::RUNS],
                       transferResult[1][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("Runs should be summed. Sub ids are the same.", "14913+14914",
                      transferResult[0][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("Runs should be summed. Sub ids are the same.", "14915+14916",
                      transferResult[1][ReflTableSchema::RUNS]);

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource));
  }


  void test_do_not_include_invalid_measurements() {
    // Search result inforation not used in the following since we mock the
    // return from the measurementSource
    SearchResultMap data;
    data.insert(
        std::make_pair<std::string, SearchResult>("111", SearchResult()));

    auto mockMeasurementItemSource = new MockReflMeasurementItemSource;
    // We expect that we are going to fetch the  measurement data for every
    // search result.
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillRepeatedly(Return(MeasurementItem::InvalidMeasurementItem("Abort!")));

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
            std::unique_ptr<MockReflMeasurementItemSource>(mockMeasurementItemSource)));

    auto result = strategy.transferRuns(data, progress);

    TSM_ASSERT_EQUALS("Measurements where invalid. Results should be empty.", 0,
                      result.size());
    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource));
  }

  void test_clone() {

    // Sub component ICatalogInfo will be cloned
    auto pCatInfo = new MockICatalogInfo;
    EXPECT_CALL(*pCatInfo, clone()).WillOnce(Return(new MockICatalogInfo));

    // Sub component Measurment source will be cloned
    auto pMeasurementItemSource = new MockReflMeasurementItemSource;
    EXPECT_CALL(*pMeasurementItemSource, clone())
        .WillOnce(Return(new MockReflMeasurementItemSource));

    // Create it
    ReflMeasureTransferStrategy strategy(
        std::move(std::unique_ptr<MockICatalogInfo>(pCatInfo)),
        std::move(
            std::unique_ptr<MockReflMeasurementItemSource>(pMeasurementItemSource)));
    // Clone it
    auto *clone = strategy.clone();
    TS_ASSERT(dynamic_cast<ReflMeasureTransferStrategy *>(clone));
    delete clone;
  }

  void test_filtering() {

    ReflMeasureTransferStrategy strategy(
        std::unique_ptr<MockICatalogInfo>(new MockICatalogInfo),
        std::unique_ptr<MockReflMeasurementItemSource>(
            new MockReflMeasurementItemSource));

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
