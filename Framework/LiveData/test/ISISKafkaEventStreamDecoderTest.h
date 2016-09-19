#ifndef MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_
#define MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidLiveData/ISIS/ISISKafkaEventStreamDecoder.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/make_unique.h"
#include "ISISKafkaTesting.h"

#include <Poco/Path.h>
#include <thread>

class ISISKafkaEventStreamDecoderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ISISKafkaEventStreamDecoderTest *createSuite() {
    return new ISISKafkaEventStreamDecoderTest();
  }
  static void destroySuite(ISISKafkaEventStreamDecoderTest *suite) {
    delete suite;
  }

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

    MockKafkaBroker mockBroker;
    EXPECT_CALL(mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISSinglePeriodEventSubscriber))
        .WillOnce(Return(new FakeISISRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TSM_ASSERT(!decoder->hasData(),
               "Decoder should not have create data buffers yet");
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    Workspace_sptr workspace;
    TSM_ASSERT(decoder->hasData(),
               "Decoder's data buffers should be created now");
    TS_ASSERT_THROWS_NOTHING(workspace = decoder->extractData());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isRunning());

    // -- Workspace checks --
    TS_ASSERT(workspace);
    auto eventWksp = boost::dynamic_pointer_cast<EventWorkspace>(workspace);
    TS_ASSERT(eventWksp);

    checkWorkspaceMetadata(*eventWksp);
    // -- Data --
    // A timer-based test and each message contains 6 events so the total should
    // be divisible by 6
    TS_ASSERT(eventWksp->getNumberEvents() % 6 == 0);
  }

  void test_Multiple_Period_Event_Stream_Throws_RuntimeErrorOnExtract() {
    using namespace ::testing;
    using namespace ISISKafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::API::WorkspaceGroup;
    using Mantid::DataObjects::EventWorkspace;
    using namespace Mantid::LiveData;

    MockKafkaBroker mockBroker;
    EXPECT_CALL(mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISMultiplePeriodEventSubscriber))
        .WillOnce(Return(new FakeISISRunInfoStreamSubscriber(2)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    Workspace_sptr workspace;
    TS_ASSERT_THROWS(workspace = decoder->extractData(), std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isRunning());

    //    // -- Workspace checks --
    //    TS_ASSERT(workspace);
    //    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    //    TS_ASSERT(group);
  }

  void test_Empty_Event_Stream_Waits() {
    using namespace ::testing;
    using namespace ISISKafkaTesting;

    MockKafkaBroker mockBroker;
    EXPECT_CALL(mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeEmptyStreamSubscriber))
        .WillOnce(Return(new FakeISISRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    TS_ASSERT_THROWS_NOTHING(decoder->extractData());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isRunning());
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Error_In_Stream_Extraction_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace ISISKafkaTesting;

    MockKafkaBroker mockBroker;
    EXPECT_CALL(mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeExceptionThrowingStreamSubscriber))
        .WillOnce(Return(new FakeExceptionThrowingStreamSubscriber))
        .WillOnce(Return(new FakeExceptionThrowingStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    TS_ASSERT_THROWS(decoder->extractData(), std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isRunning());
  }

  void test_Empty_SpDet_Stream_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace ISISKafkaTesting;

    MockKafkaBroker mockBroker;
    EXPECT_CALL(mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISSinglePeriodEventSubscriber))
        .WillOnce(Return(new FakeISISRunInfoStreamSubscriber(1)))
        .WillOnce(Return(new FakeEmptyStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    TS_ASSERT_THROWS(decoder->extractData(), std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isRunning());
  }

  void test_Empty_RunInfo_Stream_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace ISISKafkaTesting;

    MockKafkaBroker mockBroker;
    EXPECT_CALL(mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISSinglePeriodEventSubscriber))
        .WillOnce(Return(new FakeEmptyStreamSubscriber))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    TS_ASSERT_THROWS(decoder->extractData(), std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isRunning());
  }

private:
  std::unique_ptr<Mantid::LiveData::ISISKafkaEventStreamDecoder>
  createTestDecoder(const Mantid::LiveData::IKafkaBroker &broker) {
    using namespace Mantid::LiveData;
    return Mantid::Kernel::make_unique<ISISKafkaEventStreamDecoder>(broker, "",
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
};

#endif /* MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_ */
