// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "KafkaTestThreadHelper.h"
#include "KafkaTesting.h"

#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidLiveData/Kafka/KafkaEventStreamDecoder.h"

#include <condition_variable>
#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <iostream>
#include <thread>

using Mantid::LiveData::KafkaEventStreamDecoder;

class KafkaEventStreamDecoderTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    // Temporarily change the instrument directory to the testing one
    using Mantid::Kernel::ConfigService;
    auto &config = ConfigService::Instance();
    auto baseInstDir = config.getInstrumentDirectory();
    std::filesystem::path testFile = std::filesystem::path(baseInstDir) / "unit_testing" / "UnitTestFacilities.xml";
    // Load the test facilities file
    config.updateFacilities(testFile.string());
    config.setFacility("TEST");
    // Update instrument search directory
    config.setString("instrumentDefinition.directory", baseInstDir + "/unit_testing");
  }

  void tearDown() override {
    using Mantid::Kernel::ConfigService;
    auto &config = ConfigService::Instance();
    config.reset();
    // Restore the main facilities file
    config.updateFacilities();
  }

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Single_Period_Event_Stream() {
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(2))
        .WillOnce(Return(new FakeISISEventSubscriber(1)))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)));
    auto testWrapper = createTestInstance(mockBroker);
    TSM_ASSERT("testInstance should not have create data buffers yet", !testWrapper->hasData());

    testWrapper.runKafkaOneStep(); // Start up

    // Checks
    Workspace_sptr workspace;
    TSM_ASSERT("testInstance's data buffers should be created now", testWrapper->hasData());

    TS_ASSERT_THROWS_NOTHING(testWrapper.stopCapture());
    TS_ASSERT(!testWrapper->isCapturing());

    TS_ASSERT_THROWS_NOTHING(workspace = testWrapper->extractData());

    // -- Workspace checks --
    TSM_ASSERT("Expected non-null workspace pointer from extractData()", workspace);
    auto eventWksp = std::dynamic_pointer_cast<EventWorkspace>(workspace);
    TSM_ASSERT("Expected an EventWorkspace from extractData(). Found something else", eventWksp);
    checkWorkspaceMetadata(*eventWksp);
    checkWorkspaceEventData(*eventWksp);

    /* Ensure ToF range is as expected */
    TS_ASSERT_EQUALS(6.0, eventWksp->getTofMin());
    TS_ASSERT_EQUALS(11.0, eventWksp->getTofMax());
  }

  void test_Multiple_Period_Event_Stream() {
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::API::WorkspaceGroup;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(2))
        .WillOnce(Return(new FakeISISEventSubscriber(2)))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(2)));
    auto testInstance = createTestInstance(mockBroker);
    // Need 2 full loops to get both periods
    // Note: Only 2 iterations required as FakeISISEventSubscriber does not send
    // start/stop messages

    testInstance.runKafkaOneStep();
    testInstance.runKafkaOneStep();

    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(testInstance.stopCapture());
    TS_ASSERT(!testInstance->isCapturing());

    TS_ASSERT_THROWS_NOTHING(workspace = testInstance->extractData());

    // --- Workspace checks ---
    TSM_ASSERT("Expected non-null workspace pointer from extractData()", workspace);
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    TSM_ASSERT("Expected a WorkspaceGroup from extractData(). Found something else.", group);

    TS_ASSERT_EQUALS(2, group->size());
    for (size_t i = 0; i < 2; ++i) {
      auto eventWksp = std::dynamic_pointer_cast<EventWorkspace>(group->getItem(i));
      TSM_ASSERT("Expected an EventWorkspace for each member of the group", eventWksp);
      checkWorkspaceMetadata(*eventWksp);
      checkWorkspaceEventData(*eventWksp);
    }
  }

  void test_Varying_Period_Event_Stream() {
    /**
     * Test that period number is correctly updated between runs
     * e.g If the first run has 1 period and the next has 2 periods
     */
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::API::WorkspaceGroup;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();

    constexpr size_t nCallsOne = 2;
    constexpr size_t nCallsTwo = 1;

    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(nCallsOne))
        .WillOnce(Return(new FakeVariablePeriodSubscriber(0))) // 1st run
        .WillOnce(Return(new FakeRunInfoStreamSubscriberVaryingNPeriods));
    EXPECT_CALL(*mockBroker, subscribe_(_, _, _))
        .Times(Exactly(nCallsTwo))
        .WillOnce(Return(new FakeVariablePeriodSubscriber(4))); // 2nd run

    auto testInstance = createTestInstance(mockBroker);
    TSM_ASSERT("testInstance should not have create data buffers yet", !testInstance->hasData());
    // Run start, Event, Run stop, Run start, (2 period)
    for (size_t i = 0; i < 5; i++) {
      testInstance.runKafkaOneStep();
    }

    Workspace_sptr workspace;
    // Extract the data from single period and inform the testWrapper
    TS_ASSERT_THROWS_NOTHING(workspace = testInstance->extractData());
    TS_ASSERT(testInstance->hasReachedEndOfRun());
    // Continue to capture multi period data
    // (one extra iteration to ensure stop signal is acted on before data
    // extraction)

    for (size_t i = 0; i < 4; i++) {
      testInstance.runKafkaOneStep();
    }

    TS_ASSERT_THROWS_NOTHING(workspace = testInstance->extractData());
    TS_ASSERT(testInstance->hasReachedEndOfRun());
    TS_ASSERT_THROWS_NOTHING(testInstance.stopCapture());
    TS_ASSERT(!testInstance->isCapturing());

    // --- Workspace checks ---
    TSM_ASSERT("Expected non-null workspace pointer from extractData()", workspace);
    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    TSM_ASSERT("Expected a WorkspaceGroup from extractData(). Found something else.", group);

    TS_ASSERT_EQUALS(2, group->size());
    for (size_t i = 0; i < 2; ++i) {
      auto eventWksp = std::dynamic_pointer_cast<EventWorkspace>(group->getItem(i));
      TSM_ASSERT("Expected an EventWorkspace for each member of the group", eventWksp);
      checkWorkspaceMetadata(*eventWksp);
      checkWorkspaceEventData(*eventWksp);
    }
  }

  void test_End_Of_Run_Reported_After_Run_Stop_Reached() {
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(2))
        .WillOnce(Return(new FakeDataStreamSubscriber(1)))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)));
    auto testInstance = createTestInstance(mockBroker);
    TSM_ASSERT("testInstance should not have create data buffers yet", !testInstance->hasData());
    // 3 iterations to get first run, consisting of a run start message, an
    // event message and a run stop message
    for (int i = 0; i < 3; i++) {
      testInstance.runKafkaOneStep();
    }

    testInstance.runKafkaOneStep(); // End of run

    Workspace_sptr workspace;
    // Extract data should only get data from the first run
    TS_ASSERT_THROWS_NOTHING(workspace = testInstance->extractData());
    TS_ASSERT(testInstance->hasReachedEndOfRun());
    TS_ASSERT_THROWS_NOTHING(testInstance.stopCapture());
    TS_ASSERT(!testInstance->isCapturing());

    // -- Workspace checks --
    TSM_ASSERT("Expected non-null workspace pointer from extractData()", workspace);
    auto eventWksp = std::dynamic_pointer_cast<EventWorkspace>(workspace);
    TSM_ASSERT("Expected an EventWorkspace from extractData(). Found something else", eventWksp);

    TSM_ASSERT_EQUALS("Expected exactly 6 events from message in first run", 6, eventWksp->getNumberEvents());
  }

  void test_Get_All_Run_Events_When_Run_Stop_Message_Received_Before_Last_Event_Message() {
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(2))
        .WillOnce(Return(new FakeDataStreamSubscriber(3)))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)));
    auto testInstance = createTestInstance(mockBroker);
    TSM_ASSERT("testInstance should not have create data buffers yet", !testInstance->hasData());
    // 4 iterations to get first run, consisting of a run start message, an
    // event message, a run stop message, lastly another event message
    for (int i = 0; i < 5; i++) {
      testInstance.runKafkaOneStep();
    }

    Workspace_sptr workspace;
    // Extract data should only get data from the first run
    TS_ASSERT_THROWS_NOTHING(workspace = testInstance->extractData());
    TS_ASSERT(testInstance->hasReachedEndOfRun());
    TS_ASSERT_THROWS_NOTHING(testInstance.stopCapture());
    TS_ASSERT(!testInstance->isCapturing());

    // -- Workspace checks --
    TSM_ASSERT("Expected non-null workspace pointer from extractData()", workspace);
    auto eventWksp = std::dynamic_pointer_cast<EventWorkspace>(workspace);
    TSM_ASSERT("Expected an EventWorkspace from extractData(). Found something else", eventWksp);

    TSM_ASSERT_EQUALS("Expected exactly 12 events from messages in first run", 12, eventWksp->getNumberEvents());
  }

  void test_Sample_Log_From_Event_Stream() {
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(2))
        .WillOnce(Return(new FakeSampleEnvironmentSubscriber))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)));
    auto testInstance = createTestInstance(mockBroker);
    TSM_ASSERT("testInstance should not have create data buffers yet", !testInstance->hasData());
    testInstance.runKafkaOneStep();
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = testInstance->extractData());
    TS_ASSERT_THROWS_NOTHING(testInstance.stopCapture());
    TS_ASSERT(!testInstance->isCapturing());

    // -- Workspace checks --
    TSM_ASSERT("Expected non-null workspace pointer from extractData()", workspace);
    auto eventWksp = std::dynamic_pointer_cast<EventWorkspace>(workspace);
    TSM_ASSERT("Expected an EventWorkspace from extractData(). Found something else", eventWksp);

    checkWorkspaceLogData(*eventWksp);
  }

  void test_Empty_Event_Stream_Waits() {
    using namespace ::testing;
    using namespace KafkaTesting;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(2))
        .WillOnce(Return(new FakeEmptyStreamSubscriber))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)));
    auto testInstance = createTestInstance(mockBroker);
    testInstance.runKafkaOneStep();

    TS_ASSERT_THROWS_NOTHING(testInstance->extractData());
    TS_ASSERT_THROWS_NOTHING(testInstance.stopCapture());
    TS_ASSERT(!testInstance->isCapturing());
  }

  void test_No_Exception_When_Event_Message_Without_Facility_Data_Is_Processed() {
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::EventWorkspace;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(2))
        .WillOnce(Return(new FakeEventSubscriber))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)));
    auto testInstance = createTestInstance(mockBroker);

    testInstance.runKafkaOneStep(); // Init
    testInstance.runKafkaOneStep(); // Process

    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(testInstance.stopCapture());
    TS_ASSERT(!testInstance->isCapturing());
    TS_ASSERT_THROWS_NOTHING(workspace = testInstance->extractData());

    // Check we did process the event message and extract the events
    TSM_ASSERT("Expected non-null workspace pointer from extractData()", workspace);
    auto eventWksp = std::dynamic_pointer_cast<EventWorkspace>(workspace);
    TSM_ASSERT("Expected an EventWorkspace from extractData(). Found something else", eventWksp);

    TSM_ASSERT_EQUALS("Expected 3 events from the event message", 3, eventWksp->getNumberEvents());
  }

  void test_Compute_Bounds_Multiple_Threads() {
    const std::vector<Mantid::LiveData::KafkaEventStreamDecoder::BufferedEvent> events = {
        {0, 0, 0}, {0, 0, 0}, {1, 0, 0}, {1, 0, 0}, {2, 0, 0}, {2, 0, 0}, {3, 0, 0}, {3, 0, 0},
        {4, 0, 0}, {4, 0, 0}, {5, 0, 0}, {5, 0, 0}, {6, 0, 0}, {6, 0, 0}, {7, 0, 0}, {7, 0, 0},
    };

    const auto groupBounds = computeGroupBoundaries(events, 8);
    TS_ASSERT_EQUALS(9, groupBounds.size());

    TS_ASSERT_EQUALS(0, groupBounds[0]);
    TS_ASSERT_EQUALS(2, groupBounds[1]);
    TS_ASSERT_EQUALS(4, groupBounds[2]);
    TS_ASSERT_EQUALS(6, groupBounds[3]);
    TS_ASSERT_EQUALS(8, groupBounds[4]);
    TS_ASSERT_EQUALS(10, groupBounds[5]);
    TS_ASSERT_EQUALS(12, groupBounds[6]);
    TS_ASSERT_EQUALS(14, groupBounds[7]);
    TS_ASSERT_EQUALS(events.size(), groupBounds[8]);
  }

  void test_Compute_Bounds_Multiple_Threads_Low_Events() {
    const std::vector<Mantid::LiveData::KafkaEventStreamDecoder::BufferedEvent> events = {
        {0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}, {3, 0, 0}, {4, 0, 0},
    };

    const auto groupBounds = computeGroupBoundaries(events, 8);
    TS_ASSERT_EQUALS(9, groupBounds.size());

    const auto upper = events.size();

    TS_ASSERT_EQUALS(0, groupBounds[0]);
    TS_ASSERT_EQUALS(1, groupBounds[1]);
    TS_ASSERT_EQUALS(2, groupBounds[2]);
    TS_ASSERT_EQUALS(3, groupBounds[3]);
    TS_ASSERT_EQUALS(5, groupBounds[4]);
    TS_ASSERT_EQUALS(upper, groupBounds[5]);
    TS_ASSERT_EQUALS(upper, groupBounds[6]);
    TS_ASSERT_EQUALS(upper, groupBounds[7]);
    TS_ASSERT_EQUALS(upper, groupBounds[8]);
  }

  void test_Compute_Bounds_Multiple_Threads_Very_Inbalanced() {
    const std::vector<Mantid::LiveData::KafkaEventStreamDecoder::BufferedEvent> events = {/* 0 */
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},
                                                                                          {0, 0, 0},

                                                                                          /* 14 */
                                                                                          {1, 0, 0},

                                                                                          /* 15 */
                                                                                          {2, 0, 0},

                                                                                          /* 16 */
                                                                                          {3, 0, 0},
                                                                                          {3, 0, 0},

                                                                                          /* 18 */
                                                                                          {4, 0, 0}};

    const auto groupBounds = computeGroupBoundaries(events, 8);
    TS_ASSERT_EQUALS(9, groupBounds.size());

    const auto upper = events.size();

    /* Generated groups contain: 0  1,2  3  4 */
    TS_ASSERT_EQUALS(0, groupBounds[0]);
    TS_ASSERT_EQUALS(14, groupBounds[1]);
    TS_ASSERT_EQUALS(16, groupBounds[2]);
    TS_ASSERT_EQUALS(18, groupBounds[3]);
    TS_ASSERT_EQUALS(upper, groupBounds[4]);
    TS_ASSERT_EQUALS(upper, groupBounds[5]);
    TS_ASSERT_EQUALS(upper, groupBounds[6]);
    TS_ASSERT_EQUALS(upper, groupBounds[7]);
    TS_ASSERT_EQUALS(upper, groupBounds[8]);
  }

  void test_Compute_Bounds_Single_Thread() {
    const std::vector<Mantid::LiveData::KafkaEventStreamDecoder::BufferedEvent> events = {
        {0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}, {3, 0, 0}, {4, 0, 0},
    };

    const auto groupBounds = computeGroupBoundaries(events, 1);
    TS_ASSERT_EQUALS(2, groupBounds.size());

    TS_ASSERT_EQUALS(0, groupBounds[0]);
    TS_ASSERT_EQUALS(events.size(), groupBounds[1]);
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Error_In_Stream_Extraction_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace KafkaTesting;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(2))
        .WillOnce(Return(new FakeExceptionThrowingStreamSubscriber))
        .WillOnce(Return(new FakeExceptionThrowingStreamSubscriber));
    auto testInstance = createTestInstance(mockBroker);
    testInstance.runKafkaOneStep();

    TS_ASSERT_THROWS(testInstance->extractData(), const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(testInstance.stopCapture());
    TS_ASSERT(!testInstance->isCapturing());
  }

  void test_Empty_RunInfo_Stream_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace KafkaTesting;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(2))
        .WillOnce(Return(new FakeISISEventSubscriber(1)))
        .WillOnce(Return(new FakeEmptyStreamSubscriber));
    auto testInstance = createTestInstance(mockBroker);
    testInstance.runKafkaOneStep();
    TS_ASSERT_THROWS(testInstance->extractData(), const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(testInstance.stopCapture());
    TS_ASSERT(!testInstance->isCapturing());
  }

private:
  KafkaTesting::KafkaTestThreadHelper<Mantid::LiveData::KafkaEventStreamDecoder>
  createTestInstance(const std::shared_ptr<Mantid::LiveData::IKafkaBroker> &broker) {
    using namespace Mantid::LiveData;

    KafkaEventStreamDecoder testInstance(broker, "", "", "", "", "", 0);
    return KafkaTesting::KafkaTestThreadHelper<KafkaEventStreamDecoder>(std::move(testInstance));
  }

  void checkWorkspaceMetadata(const Mantid::DataObjects::EventWorkspace &eventWksp) {
    TS_ASSERT(eventWksp.getInstrument());
    TS_ASSERT_EQUALS("HRPDTEST", eventWksp.getInstrument()->getName());
    TS_ASSERT_EQUALS("2016-08-31T12:07:42", eventWksp.run().getPropertyValueAsType<std::string>("run_start"));
    std::array<Mantid::specnum_t, 5> specs = {{1, 2, 3, 4, 5}};
    std::array<Mantid::detid_t, 5> ids = {{1001, 1002, 1100, 901000, 10100}};
    TS_ASSERT_EQUALS(specs.size(), eventWksp.getNumberHistograms());
    for (size_t i = 0; i < eventWksp.getNumberHistograms(); ++i) {
      const auto &spec = eventWksp.getSpectrum(i);
      TS_ASSERT_EQUALS(specs[i], spec.getSpectrumNo());
      const auto &sid = spec.getDetectorIDs();
      TS_ASSERT_EQUALS(ids[i], *(sid.begin()));
    }
  }

  void checkWorkspaceEventData(const Mantid::DataObjects::EventWorkspace &eventWksp) {
    // A timer-based test and each message contains 6 events so the total
    // should be divisible by 6, but not be 0
    TS_ASSERT(eventWksp.getNumberEvents() % 6 == 0);
    TS_ASSERT(eventWksp.getNumberEvents() != 0);
  }

  void checkWorkspaceLogData(Mantid::DataObjects::EventWorkspace &eventWksp) {
    Mantid::Kernel::TimeSeriesProperty<int32_t> *log = nullptr;
    auto run = eventWksp.mutableRun();
    // We should find a sample log with this name
    TS_ASSERT_THROWS_NOTHING(log = run.getTimeSeriesProperty<int32_t>("fake source"));
    if (log) {
      TS_ASSERT_EQUALS(log->firstTime().toISO8601String(), "2017-05-24T09:29:48")
      TS_ASSERT_EQUALS(log->firstValue(), 42)
    }
  }
};
