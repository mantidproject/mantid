// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGYTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGYTEST_H_

#include "../ISISReflectometry/ReflMeasureTransferStrategy.h"
#include "../ISISReflectometry/ReflMeasurementItemSource.h"
#include "../ISISReflectometry/ReflTableSchema.h"
#include "MantidKernel/WarningSuppressions.h"

#include "ReflMockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <memory>
#include <utility>

using namespace testing;
using namespace MantidQt::CustomInterfaces;

class MockReflMeasurementItemSource
    : public MantidQt::CustomInterfaces::ReflMeasurementItemSource {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_CONST_METHOD2(obtain, MantidQt::CustomInterfaces::MeasurementItem(
                                 const std::string &, const std::string &));
  MOCK_CONST_METHOD0(clone, MockReflMeasurementItemSource *());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
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

    auto mockMeasurementItemSource =
        std::make_unique<MockReflMeasurementItemSource>();
    auto mockMeasurementItemSource_ptr = mockMeasurementItemSource.get();
    // We expect that we are going to fetch the  measurement data for every
    // search result.
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillRepeatedly(
            Return(MeasurementItem("a", "s_a", "l", "t", 0, "111", "title")));

    auto mockCatInfo = std::make_unique<MockICatalogInfo>();
    auto mockCatInfo_ptr = mockCatInfo.get();
    // We expect that every location will be translated/transformed to make it
    // os specific
    EXPECT_CALL(*mockCatInfo, transformArchivePath(_))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillRepeatedly(Return(std::string()));

    MockProgressBase progress;
    // We expect a progress update on each transfer
    EXPECT_CALL(progress, doReport(_))
        .Times(Exactly(static_cast<int>(data.size())));

    ReflMeasureTransferStrategy strategy(std::move(mockCatInfo),
                                         std::move(mockMeasurementItemSource));

    strategy.transferRuns(data, progress);

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo_ptr));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource_ptr));
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

    auto mockMeasurementItemSource =
        std::make_unique<MockReflMeasurementItemSource>();
    auto mockMeasurementItemSource_ptr = mockMeasurementItemSource.get();
    // We are going to return three SearchResults two have the same measurement
    // id
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillOnce(Return(
            MeasurementItem("m1", "s1", "l1", "t1", 0.1, "111", "title")))
        .WillOnce(Return(
            MeasurementItem("m1", "s2", "l1", "t1", 0.2, "122", "title")))
        .WillOnce(Return(
            MeasurementItem("m2", "s2", "l1", "t1", 0.2, "123", "title")));

    auto mockCatInfo = std::make_unique<MockICatalogInfo>();
    auto mockCatInfo_ptr = mockCatInfo.get();
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
    ReflMeasureTransferStrategy strategy(std::move(mockCatInfo),
                                         std::move(mockMeasurementItemSource));

    // Do the transfer
    auto transferResult = strategy.transferRuns(data, progress);

    auto successfulRuns = transferResult.getTransferRuns();
    // Check the transfer entries
    TSM_ASSERT_EQUALS("Wrong number of rows", 3, successfulRuns.size());

    for (size_t i = 1; i < successfulRuns.size(); ++i) {
      TSM_ASSERT_DIFFERS("Runs should be the different for all rows",
                         successfulRuns[0][ReflTableSchema::RUNS],
                         successfulRuns[i][ReflTableSchema::RUNS]);
    }

    TSM_ASSERT_EQUALS("Group should be the same for first two rows",
                      successfulRuns[0][ReflTableSchema::GROUP],
                      successfulRuns[1][ReflTableSchema::GROUP]);
    TSM_ASSERT_EQUALS("Group should be '0 - title' for first two rows",
                      successfulRuns[0][ReflTableSchema::GROUP], "0 - title");

    TSM_ASSERT_DIFFERS("Group should be different for last rows",
                       successfulRuns[0][ReflTableSchema::GROUP],
                       successfulRuns[2][ReflTableSchema::GROUP]);
    TSM_ASSERT_EQUALS("Group should be '1 - title' for third row",
                      successfulRuns[2][ReflTableSchema::GROUP], "1 - title");

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo_ptr));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource_ptr));
  }

  void test_when_there_is_no_valid_measurement_id() {
    // Search result inforation not used in the following since we mock the
    // return from the measurementSource
    SearchResultMap data;
    data.insert(
        std::make_pair<std::string, SearchResult>("111", SearchResult()));
    data.insert(
        std::make_pair<std::string, SearchResult>("112", SearchResult()));
    data.insert(
        std::make_pair<std::string, SearchResult>("113", SearchResult()));

    auto mockMeasurementItemSource =
        std::make_unique<MockReflMeasurementItemSource>();
    auto mockMeasurementItemSource_ptr = mockMeasurementItemSource.get();
    // We have 2 with valid measurement ids and 1 with no measurement id
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillOnce(
            Return(MeasurementItem("", "s1", "l1", "t1", 0.1, "111", "title")))
        .WillOnce(Return(
            MeasurementItem("m1", "s1", "l1", "t1", 0.2, "122", "title")))
        .WillOnce(Return(
            MeasurementItem("m1", "s2", "l1", "t1", 0.2, "123", "title")));

    auto mockCatInfo = std::make_unique<MockICatalogInfo>();
    auto mockCatInfo_ptr = mockCatInfo.get();

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
    ReflMeasureTransferStrategy strategy(std::move(mockCatInfo),
                                         std::move(mockMeasurementItemSource));

    // retreive transfer results
    auto transferResult = strategy.transferRuns(data, progress);
    // get valid runs
    auto successfulRuns = transferResult.getTransferRuns();
    // get invalid runs
    auto invalidRuns = transferResult.getErrorRuns();

    TSM_ASSERT_EQUALS("Should have two rows", 2, successfulRuns.size());
    TSM_ASSERT_EQUALS("Should have one invalid run", 1, invalidRuns.size());
    TSM_ASSERT_DIFFERS("Runs should be the different for both columns",
                       successfulRuns[0][ReflTableSchema::RUNS],
                       successfulRuns[1][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("run should be singular", "122",
                      successfulRuns[0][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("run should be singular.", "123",
                      successfulRuns[1][ReflTableSchema::RUNS]);

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo_ptr));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource_ptr));
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

    auto mockMeasurementItemSource =
        std::make_unique<MockReflMeasurementItemSource>();
    auto mockMeasurementItemSource_ptr = mockMeasurementItemSource.get();
    // All 3 have same measurment id, but we also have 2 with same sub id.
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillOnce(Return(
            MeasurementItem("m1", "s1", "l1", "t1", 0.1, "111", "title")))
        .WillOnce(Return(
            MeasurementItem("m1", "s1", "l1", "t1", 0.2, "122", "title")))
        .WillOnce(Return(
            MeasurementItem("m1", "s2", "l1", "t1", 0.2, "123", "title")));

    auto mockCatInfo = std::make_unique<MockICatalogInfo>();
    auto mockCatInfo_ptr = mockCatInfo.get();
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
    ReflMeasureTransferStrategy strategy(std::move(mockCatInfo),
                                         std::move(mockMeasurementItemSource));

    // Do the transfer
    auto transferResult = strategy.transferRuns(data, progress);
    auto successfulRuns = transferResult.getTransferRuns();
    // Check the transfer entries
    TSM_ASSERT_EQUALS("Should have two rows", 2, successfulRuns.size());
    TSM_ASSERT_DIFFERS("Runs should be the different for both columns",
                       successfulRuns[0][ReflTableSchema::RUNS],
                       successfulRuns[1][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("Runs should be summed. Sub ids are the same.", "111+122",
                      successfulRuns[0][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("Other run should be singular.", "123",
                      successfulRuns[1][ReflTableSchema::RUNS]);

    for (size_t i = 1; i < successfulRuns.size(); ++i) {
      TSM_ASSERT_EQUALS("All should have the same group",
                        successfulRuns[0][ReflTableSchema::GROUP],
                        successfulRuns[i][ReflTableSchema::GROUP]);
      TSM_ASSERT_EQUALS("Group should be '0 - title'",
                        successfulRuns[i][ReflTableSchema::GROUP], "0 - title");
    }

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo_ptr));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource_ptr));
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

    auto mockMeasurementItemSource =
        std::make_unique<MockReflMeasurementItemSource>();
    auto mockMeasurementItemSource_ptr = mockMeasurementItemSource.get();
    // All 3 have same measurment id, but we also have 2 with same sub id.
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillOnce(Return(
            MeasurementItem("m1", "s1", "l1", "t1", 0.1, "14913", "title")))
        .WillOnce(Return(
            MeasurementItem("m1", "s1", "l1", "t1", 0.1, "14914", "title")))
        .WillOnce(Return(
            MeasurementItem("m2", "s1", "l1", "t1", 0.2, "14915", "title")))
        .WillOnce(Return(
            MeasurementItem("m2", "s1", "l1", "t1", 0.2, "14916", "title")));

    auto mockCatInfo = std::make_unique<MockICatalogInfo>();
    auto mockCatInfo_ptr = mockCatInfo.get();
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
    ReflMeasureTransferStrategy strategy(std::move(mockCatInfo),
                                         std::move(mockMeasurementItemSource));

    // Do the transfer
    auto transferResult = strategy.transferRuns(data, progress);
    auto successfulRuns = transferResult.getTransferRuns();

    // Check the transfer entries
    TSM_ASSERT_EQUALS("Should have two rows", 2, successfulRuns.size());
    TSM_ASSERT_DIFFERS("Runs should be the different for both columns",
                       successfulRuns[0][ReflTableSchema::RUNS],
                       successfulRuns[1][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("Runs should be summed. Sub ids are the same.",
                      "14913+14914", successfulRuns[0][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("Runs should be summed. Sub ids are the same.",
                      "14915+14916", successfulRuns[1][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("Group should be '0 - title'",
                      successfulRuns[0][ReflTableSchema::GROUP], "0 - title");
    TSM_ASSERT_EQUALS("Group should be '1 - title'",
                      successfulRuns[1][ReflTableSchema::GROUP], "1 - title");

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo_ptr));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource_ptr));
  }

  void test_same_id_but_different_title() {

    SearchResultMap data;
    data.insert(
        std::make_pair<std::string, SearchResult>("14913", SearchResult()));
    data.insert(
        std::make_pair<std::string, SearchResult>("14914", SearchResult()));

    auto mockMeasurementItemSource =
        std::make_unique<MockReflMeasurementItemSource>();
    auto mockMeasurementItemSource_ptr = mockMeasurementItemSource.get();

    // Same measurment id but different title
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillOnce(Return(MeasurementItem("m1", "s1", "l1", "t1", 0.1, "14913",
                                         "Sample 1 H=0.10")))
        .WillOnce(Return(MeasurementItem("m1", "s2", "l1", "t1", 0.1, "14914",
                                         "Sample 1 H=0.09")));

    auto mockCatInfo = std::make_unique<MockICatalogInfo>();
    auto mockCatInfo_ptr = mockCatInfo.get();
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
    ReflMeasureTransferStrategy strategy(std::move(mockCatInfo),
                                         std::move(mockMeasurementItemSource));

    // Do the transfer
    auto transferResult = strategy.transferRuns(data, progress);
    auto successfulRuns = transferResult.getTransferRuns();

    // Check the transfer entries
    TSM_ASSERT_EQUALS("Should have two rows", 2, successfulRuns.size());
    TSM_ASSERT_DIFFERS("Runs should be the different for both columns",
                       successfulRuns[0][ReflTableSchema::RUNS],
                       successfulRuns[1][ReflTableSchema::RUNS]);
    TSM_ASSERT_EQUALS("Group should be '0 - Sample 1 H=0.10'",
                      successfulRuns[0][ReflTableSchema::GROUP],
                      "0 - Sample 1 H=0.10");
    TSM_ASSERT_EQUALS("Group should be '0 - Sample 1 H=0.10'",
                      successfulRuns[1][ReflTableSchema::GROUP],
                      "0 - Sample 1 H=0.10");

    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo_ptr));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource_ptr));
  }

  void test_do_not_include_invalid_measurements() {
    // Search result inforation not used in the following since we mock the
    // return from the measurementSource
    SearchResultMap data;
    data.insert(
        std::make_pair<std::string, SearchResult>("111", SearchResult()));

    auto mockMeasurementItemSource =
        std::make_unique<MockReflMeasurementItemSource>();
    auto mockMeasurementItemSource_ptr = mockMeasurementItemSource.get();

    // We expect that we are going to fetch the  measurement data for every
    // search result.
    EXPECT_CALL(*mockMeasurementItemSource, obtain(_, _))
        .Times(Exactly(static_cast<int>(data.size())))
        .WillRepeatedly(
            Return(MeasurementItem::InvalidMeasurementItem("Abort!")));

    auto mockCatInfo = std::make_unique<MockICatalogInfo>();
    auto mockCatInfo_ptr = mockCatInfo.get();
    // We expect that every location will be translated/transformed to make it
    // os specific
    EXPECT_CALL(*mockCatInfo, transformArchivePath(_))
        .Times(Exactly(static_cast<int>(data.size())));

    MockProgressBase progress;
    // Nothing obtained. No progress to report.
    EXPECT_CALL(progress, doReport(_)).Times(Exactly(1));

    ReflMeasureTransferStrategy strategy(std::move(mockCatInfo),
                                         std::move(mockMeasurementItemSource));

    auto transferRuns = strategy.transferRuns(data, progress);
    auto result = transferRuns.getTransferRuns();
    TSM_ASSERT_EQUALS("Measurements where invalid. Results should be empty.", 0,
                      result.size());
    TS_ASSERT(Mock::VerifyAndClear(mockCatInfo_ptr));
    TS_ASSERT(Mock::VerifyAndClear(mockMeasurementItemSource_ptr));
  }

  void test_clone() {

    // Sub component ICatalogInfo will be cloned
    auto pCatInfo = std::make_unique<MockICatalogInfo>();
    EXPECT_CALL(*pCatInfo, clone()).WillOnce(Return(new MockICatalogInfo));

    // Sub component Measurment source will be cloned
    auto pMeasurementItemSource =
        std::make_unique<MockReflMeasurementItemSource>();
    EXPECT_CALL(*pMeasurementItemSource, clone())
        .WillOnce(Return(new MockReflMeasurementItemSource));

    // Create it
    ReflMeasureTransferStrategy strategy(std::move(pCatInfo),
                                         std::move(pMeasurementItemSource));
    // Clone it
    auto clone = strategy.clone();
    TS_ASSERT(dynamic_cast<ReflMeasureTransferStrategy *>(clone.get()));
  }

  void test_filtering() {

    ReflMeasureTransferStrategy strategy(
        std::make_unique<MockICatalogInfo>(),
        std::make_unique<MockReflMeasurementItemSource>());

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
