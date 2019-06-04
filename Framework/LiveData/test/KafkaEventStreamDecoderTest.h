// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_KAFKAEVENTSTREAMDECODERTEST_H_
#define MANTID_LIVEDATA_KAFKAEVENTSTREAMDECODERTEST_H_

#include <cxxtest/TestSuite.h>

#include "KafkaTesting.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidLiveData/Kafka/KafkaEventStreamDecoder.h"

#include <Poco/Path.h>
#include <condition_variable>
#include <thread>

class KafkaEventStreamDecoderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static KafkaEventStreamDecoderTest *createSuite() {
    return new KafkaEventStreamDecoderTest();
  }
  static void destroySuite(KafkaEventStreamDecoderTest *suite) { delete suite; }

  void setUp() override {
    // Temporarily change the instrument directory to the testing one
    using Mantid::Kernel::ConfigService;
    auto &config = ConfigService::Instance();
    auto baseInstDir = config.getInstrumentDirectory();
    Poco::Path testFile =
        Poco::Path(baseInstDir).resolve("unit_testing/UnitTestFacilities.xml");
    // Load the test facilities file
    config.updateFacilities(testFile.toString());
    config.setFacility("TEST");
    // Update instrument search directory
    config.setString("instrumentDefinition.directory",
                     baseInstDir + "/unit_testing");
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
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISEventSubscriber(1)))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TSM_ASSERT("Decoder should not have create data buffers yet",
               !decoder->hasData());
    startCapturing(*decoder, 1);

    // Checks
    Workspace_sptr workspace;
    TSM_ASSERT("Decoder's data buffers should be created now",
               decoder->hasData());
    TS_ASSERT_THROWS_NOTHING(workspace = decoder->extractData());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());

    // -- Workspace checks --
    TSM_ASSERT("Expected non-null workspace pointer from extractData()",
               workspace);
    auto eventWksp = boost::dynamic_pointer_cast<EventWorkspace>(workspace);
    TSM_ASSERT(
        "Expected an EventWorkspace from extractData(). Found something else",
        eventWksp);
    checkWorkspaceMetadata(*eventWksp);
    checkWorkspaceEventData(*eventWksp);
  }

  void test_Multiple_Period_Event_Stream() {
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::WorkspaceGroup;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISEventSubscriber(2)))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(2)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    // Need 2 full loops to get both periods
    // Note: Only 2 iterations required as FakeISISEventSubscriber does not send
    // start/stop messages
    startCapturing(*decoder, 2);

    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = decoder->extractData());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());

    // --- Workspace checks ---
    TSM_ASSERT("Expected non-null workspace pointer from extractData()",
               workspace);
    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    TSM_ASSERT(
        "Expected a WorkspaceGroup from extractData(). Found something else.",
        group);

    TS_ASSERT_EQUALS(2, group->size());
    for (size_t i = 0; i < 2; ++i) {
      auto eventWksp =
          boost::dynamic_pointer_cast<EventWorkspace>(group->getItem(i));
      TSM_ASSERT("Expected an EventWorkspace for each member of the group",
                 eventWksp);
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
    using Mantid::API::WorkspaceGroup;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeVariablePeriodSubscriber(0))) // 1st run
        .WillOnce(Return(new FakeRunInfoStreamSubscriberVaryingNPeriods))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    EXPECT_CALL(*mockBroker, subscribe_(_, _, _))
        .Times(Exactly(2))
        .WillOnce(Return(new FakeVariablePeriodSubscriber(4))) // 2nd run
        .WillOnce(
            Return(new FakeISISSpDetStreamSubscriber)); // det-spec for 2nd run

    auto decoder = createTestDecoder(mockBroker);
    TSM_ASSERT("Decoder should not have create data buffers yet",
               !decoder->hasData());
    // Run start, Event, Run stop, Run start (2 period)
    startCapturing(*decoder, 4);
    Workspace_sptr workspace;
    // Extract the data from single period and inform the decoder
    TS_ASSERT_THROWS_NOTHING(workspace = decoder->extractData());
    TS_ASSERT(decoder->hasReachedEndOfRun());
    // Continue to capture multi period data
    // (one extra iteration to ensure stop signal is acted on before data
    // extraction)
    continueCapturing(*decoder, 7);
    TS_ASSERT_THROWS_NOTHING(workspace = decoder->extractData());
    TS_ASSERT(decoder->hasReachedEndOfRun());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());

    // --- Workspace checks ---
    TSM_ASSERT("Expected non-null workspace pointer from extractData()",
               workspace);
    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    TSM_ASSERT(
        "Expected a WorkspaceGroup from extractData(). Found something else.",
        group);

    TS_ASSERT_EQUALS(2, group->size());
    for (size_t i = 0; i < 2; ++i) {
      auto eventWksp =
          boost::dynamic_pointer_cast<EventWorkspace>(group->getItem(i));
      TSM_ASSERT("Expected an EventWorkspace for each member of the group",
                 eventWksp);
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
        .Times(Exactly(3))
        .WillOnce(Return(new FakeDataStreamSubscriber(1)))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TSM_ASSERT("Decoder should not have create data buffers yet",
               !decoder->hasData());
    // 3 iterations to get first run, consisting of a run start message, an
    // event message and a run stop message
    startCapturing(*decoder, 3);
    Workspace_sptr workspace;
    // Extract data should only get data from the first run
    TS_ASSERT_THROWS_NOTHING(workspace = decoder->extractData());
    TS_ASSERT(decoder->hasReachedEndOfRun());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());

    // -- Workspace checks --
    TSM_ASSERT("Expected non-null workspace pointer from extractData()",
               workspace);
    auto eventWksp = boost::dynamic_pointer_cast<EventWorkspace>(workspace);
    TSM_ASSERT(
        "Expected an EventWorkspace from extractData(). Found something else",
        eventWksp);

    TSM_ASSERT_EQUALS("Expected exactly 6 events from message in first run", 6,
                      eventWksp->getNumberEvents());
  }

  void
  test_Get_All_Run_Events_When_Run_Stop_Message_Received_Before_Last_Event_Message() {
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeDataStreamSubscriber(3)))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TSM_ASSERT("Decoder should not have create data buffers yet",
               !decoder->hasData());
    // 4 iterations to get first run, consisting of a run start message, an
    // event message, a run stop message, lastly another event message
    startCapturing(*decoder, 4);
    Workspace_sptr workspace;
    // Extract data should only get data from the first run
    TS_ASSERT_THROWS_NOTHING(workspace = decoder->extractData());
    TS_ASSERT(decoder->hasReachedEndOfRun());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());

    // -- Workspace checks --
    TSM_ASSERT("Expected non-null workspace pointer from extractData()",
               workspace);
    auto eventWksp = boost::dynamic_pointer_cast<EventWorkspace>(workspace);
    TSM_ASSERT(
        "Expected an EventWorkspace from extractData(). Found something else",
        eventWksp);

    TSM_ASSERT_EQUALS("Expected exactly 12 events from messages in first run",
                      12, eventWksp->getNumberEvents());
  }

  void test_Sample_Log_From_Event_Stream() {
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeSampleEnvironmentSubscriber))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TSM_ASSERT("Decoder should not have create data buffers yet",
               !decoder->hasData());
    startCapturing(*decoder, 1);
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = decoder->extractData());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());

    // -- Workspace checks --
    TSM_ASSERT("Expected non-null workspace pointer from extractData()",
               workspace);
    auto eventWksp = boost::dynamic_pointer_cast<EventWorkspace>(workspace);
    TSM_ASSERT(
        "Expected an EventWorkspace from extractData(). Found something else",
        eventWksp);

    checkWorkspaceLogData(*eventWksp);
  }

  void test_Empty_Event_Stream_Waits() {
    using namespace ::testing;
    using namespace KafkaTesting;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeEmptyStreamSubscriber))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    startCapturing(*decoder, 1);

    TS_ASSERT_THROWS_NOTHING(decoder->extractData());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());
  }

  void
  test_No_Exception_When_Event_Message_Without_Facility_Data_Is_Processed() {
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::EventWorkspace;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeEventSubscriber))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    startCapturing(*decoder, 2);
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = decoder->extractData());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());

    // Check we did process the event message and extract the events
    TSM_ASSERT("Expected non-null workspace pointer from extractData()",
               workspace);
    auto eventWksp = boost::dynamic_pointer_cast<EventWorkspace>(workspace);
    TSM_ASSERT(
        "Expected an EventWorkspace from extractData(). Found something else",
        eventWksp);

    TSM_ASSERT_EQUALS("Expected 3 events from the event message", 3,
                      eventWksp->getNumberEvents());
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Error_In_Stream_Extraction_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace KafkaTesting;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeExceptionThrowingStreamSubscriber))
        .WillOnce(Return(new FakeExceptionThrowingStreamSubscriber))
        .WillOnce(Return(new FakeExceptionThrowingStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    startCapturing(*decoder, 1);

    TS_ASSERT_THROWS(decoder->extractData(), const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());
  }

  void test_Empty_SpDet_Stream_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace KafkaTesting;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISEventSubscriber(1)))
        .WillOnce(Return(new FakeRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeEmptyStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    startCapturing(*decoder, 1);

    TS_ASSERT_THROWS(decoder->extractData(), const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());
  }

  void test_Empty_RunInfo_Stream_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace KafkaTesting;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISEventSubscriber(1)))
        .WillOnce(Return(new FakeEmptyStreamSubscriber))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    startCapturing(*decoder, 1);
    TS_ASSERT_THROWS(decoder->extractData(), const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());
  }

private:
  // Start decoding and wait until we have gathered enough data to test
  void startCapturing(Mantid::LiveData::KafkaEventStreamDecoder &decoder,
                      uint8_t maxIterations) {
    // Register callback to know when a whole loop as been iterated through
    m_niterations = 0;
    auto callback = [this, maxIterations]() {
      this->iterationCallback(maxIterations);
    };
    decoder.registerIterationEndCb(callback);
    decoder.registerErrorCb(callback);
    TS_ASSERT_THROWS_NOTHING(decoder.startCapture());
    continueCapturing(decoder, maxIterations);
  }

  void iterationCallback(uint8_t maxIterations) {
    std::unique_lock<std::mutex> lock(this->m_callbackMutex);
    this->m_niterations++;
    if (this->m_niterations == maxIterations) {
      lock.unlock();
      this->m_callbackCondition.notify_one();
    }
  }

  void continueCapturing(Mantid::LiveData::KafkaEventStreamDecoder &decoder,
                         uint8_t maxIterations) {
    // Re-register callback with the (potentially) new value of maxIterations
    auto callback = [this, maxIterations]() {
      this->iterationCallback(maxIterations);
    };
    decoder.registerIterationEndCb(callback);
    decoder.registerErrorCb(callback);
    {
      std::unique_lock<std::mutex> lk(m_callbackMutex);
      this->m_callbackCondition.wait(lk, [this, maxIterations]() {
        return this->m_niterations == maxIterations;
      });
    }
  }

  std::unique_ptr<Mantid::LiveData::KafkaEventStreamDecoder>
  createTestDecoder(std::shared_ptr<Mantid::LiveData::IKafkaBroker> broker) {
    using namespace Mantid::LiveData;
    return std::make_unique<KafkaEventStreamDecoder>(broker, "", "",
                                                                "", "");
  }

  void
  checkWorkspaceMetadata(const Mantid::DataObjects::EventWorkspace &eventWksp) {
    TS_ASSERT(eventWksp.getInstrument());
    TS_ASSERT_EQUALS("HRPDTEST", eventWksp.getInstrument()->getName());
    TS_ASSERT_EQUALS(
        "2016-08-31T12:07:42",
        eventWksp.run().getPropertyValueAsType<std::string>("run_start"));
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

  void checkWorkspaceEventData(
      const Mantid::DataObjects::EventWorkspace &eventWksp) {
    // A timer-based test and each message contains 6 events so the total should
    // be divisible by 6, but not be 0
    TS_ASSERT(eventWksp.getNumberEvents() % 6 == 0);
    TS_ASSERT(eventWksp.getNumberEvents() != 0);
  }

  void checkWorkspaceLogData(Mantid::DataObjects::EventWorkspace &eventWksp) {
    Mantid::Kernel::TimeSeriesProperty<int32_t> *log = nullptr;
    auto run = eventWksp.mutableRun();
    // We should find a sample log with this name
    TS_ASSERT_THROWS_NOTHING(
        log = run.getTimeSeriesProperty<int32_t>("fake source"));
    if (log) {
      TS_ASSERT_EQUALS(log->firstTime().toISO8601String(),
                       "2017-05-24T09:29:48")
      TS_ASSERT_EQUALS(log->firstValue(), 42)
    }
  }

private:
  std::mutex m_callbackMutex;
  std::condition_variable m_callbackCondition;
  uint8_t m_niterations = 0;
};

#endif /* MANTID_LIVEDATA_KAFKAEVENTSTREAMDECODERTEST_H_ */
