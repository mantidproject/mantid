#ifndef MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_
#define MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_

#include <cxxtest/TestSuite.h>

#include "KafkaTesting.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/make_unique.h"
#include "MantidLiveData/Kafka/KafkaEventStreamDecoder.h"

#include <Poco/Path.h>
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
        Poco::Path(baseInstDir)
            .resolve("IDFs_for_UNIT_TESTING/UnitTestFacilities.xml");
    // Load the test facilities file
    config.updateFacilities(testFile.toString());
    config.setFacility("TEST");
    // Update instrument search directory
    config.setString("instrumentDefinition.directory",
                     baseInstDir + "/IDFs_for_UNIT_TESTING");
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
    using namespace ISISKafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISEventSubscriber(1)))
        .WillOnce(Return(new FakeISISRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TSM_ASSERT("Decoder should not have create data buffers yet",
               !decoder->hasData());
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    using namespace ISISKafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::API::WorkspaceGroup;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISEventSubscriber(2)))
        .WillOnce(Return(new FakeISISRunInfoStreamSubscriber(2)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

  void test_Empty_Event_Stream_Waits() {
    using namespace ::testing;
    using namespace ISISKafkaTesting;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeEmptyStreamSubscriber))
        .WillOnce(Return(new FakeISISRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TS_ASSERT_THROWS_NOTHING(decoder->extractData());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Error_In_Stream_Extraction_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace ISISKafkaTesting;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeExceptionThrowingStreamSubscriber))
        .WillOnce(Return(new FakeExceptionThrowingStreamSubscriber))
        .WillOnce(Return(new FakeExceptionThrowingStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TS_ASSERT_THROWS(decoder->extractData(), std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());
  }

  void test_Empty_SpDet_Stream_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace ISISKafkaTesting;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISEventSubscriber(1)))
        .WillOnce(Return(new FakeISISRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeEmptyStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TS_ASSERT_THROWS(decoder->extractData(), std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());
  }

  void test_Empty_RunInfo_Stream_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace ISISKafkaTesting;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISEventSubscriber(1)))
        .WillOnce(Return(new FakeEmptyStreamSubscriber))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TS_ASSERT_THROWS(decoder->extractData(), std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isCapturing());
  }

private:
  std::unique_ptr<Mantid::LiveData::KafkaEventStreamDecoder>
  createTestDecoder(std::shared_ptr<Mantid::LiveData::IKafkaBroker> broker) {
    using namespace Mantid::LiveData;
    return Mantid::Kernel::make_unique<KafkaEventStreamDecoder>(broker, "", "",
                                                                "");
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
    TS_ASSERT(eventWksp.run().hasProperty("SampleLog1"));
    TS_ASSERT_DELTA(eventWksp.run().getLogAsSingleValue("SampleLog1"), 42.0,
                    0.01);
  }

  void checkWorkspaceEventData(
      const Mantid::DataObjects::EventWorkspace &eventWksp) {
    // A timer-based test and each message contains 6 events so the total should
    // be divisible by 6
    TS_ASSERT(eventWksp.getNumberEvents() % 6 == 0);
  }
};

#endif /* MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_ */
