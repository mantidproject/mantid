// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_KAFKAHISTOSTREAMDECODERTEST_H_
#define MANTID_LIVEDATA_KAFKAHISTOSTREAMDECODERTEST_H_

#include <cxxtest/TestSuite.h>

#include "KafkaTesting.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidLiveData/Kafka/KafkaHistoStreamDecoder.h"
#include <Poco/Path.h>

using Mantid::LiveData::KafkaHistoStreamDecoder;
using namespace KafkaTesting;

class KafkaHistoStreamDecoderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static KafkaHistoStreamDecoderTest *createSuite() {
    return new KafkaHistoStreamDecoderTest();
  }
  static void destroySuite(KafkaHistoStreamDecoderTest *suite) { delete suite; }

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

  void test_Histo_Stream() {
    using namespace ::testing;
    using namespace KafkaTesting;
    using Mantid::API::Workspace_sptr;
    using Mantid::DataObjects::Workspace2D;
    using namespace Mantid::LiveData;

    auto mockBroker = std::make_shared<MockKafkaBroker>();
    EXPECT_CALL(*mockBroker, subscribe_(_, _))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeHistoSubscriber()))
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
    auto histoWksp = boost::dynamic_pointer_cast<Workspace2D>(workspace);
    TSM_ASSERT(
        "Expected a Workspace2D from extractData(). Found something else",
        histoWksp);
    checkWorkspaceMetadata(*histoWksp);
    checkWorkspaceHistoData(*histoWksp);
    TS_ASSERT(Mock::VerifyAndClear(mockBroker.get()));
  }

private:
  std::unique_ptr<Mantid::LiveData::KafkaHistoStreamDecoder>
  createTestDecoder(std::shared_ptr<Mantid::LiveData::IKafkaBroker> broker) {
    using namespace Mantid::LiveData;
    return std::make_unique<KafkaHistoStreamDecoder>(broker, "", "",
                                                                "", "");
  }

  // Start decoding and wait until we have gathered enough data to test
  void startCapturing(Mantid::LiveData::KafkaHistoStreamDecoder &decoder,
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

  void continueCapturing(Mantid::LiveData::KafkaHistoStreamDecoder &decoder,
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

  void
  checkWorkspaceMetadata(const Mantid::DataObjects::Workspace2D &histoWksp) {
    TS_ASSERT(histoWksp.getInstrument());
    TS_ASSERT_EQUALS("HRPDTEST", histoWksp.getInstrument()->getName());
    TS_ASSERT_EQUALS(
        "2016-08-31T12:07:42",
        histoWksp.run().getPropertyValueAsType<std::string>("run_start"));
    std::array<Mantid::specnum_t, 5> specs = {{1, 2, 3, 4, 5}};
    std::array<Mantid::detid_t, 5> ids = {{1001, 1002, 1100, 901000, 10100}};
    TS_ASSERT_EQUALS(specs.size(), histoWksp.getNumberHistograms());
    for (size_t i = 0; i < histoWksp.getNumberHistograms(); ++i) {
      const auto &spec = histoWksp.getSpectrum(i);
      TS_ASSERT_EQUALS(specs[i], spec.getSpectrumNo());
      const auto &sid = spec.getDetectorIDs();
      TS_ASSERT_EQUALS(ids[i], *(sid.begin()));
    }
  }

  void
  checkWorkspaceHistoData(const Mantid::DataObjects::Workspace2D &histoWksp) {
    // Inspect all 5 HRPDTEST Spectra
    auto data = histoWksp.histogram(0);
    // std::vector<double> xbins(data.x().cbegin(), data.x().cend());
    TS_ASSERT_EQUALS(data.x().rawData(), (std::vector<double>{0, 1, 2}));
    TS_ASSERT_EQUALS(data.y().rawData(), (std::vector<double>{100, 140}));

    data = histoWksp.histogram(1);
    TS_ASSERT_EQUALS(data.y().rawData(), (std::vector<double>{210, 100}));

    data = histoWksp.histogram(2);
    TS_ASSERT_EQUALS(data.y().rawData(), (std::vector<double>{110, 70}));

    data = histoWksp.histogram(3);
    TS_ASSERT_EQUALS(data.y().rawData(), (std::vector<double>{5, 3}));

    data = histoWksp.histogram(4);
    TS_ASSERT_EQUALS(data.y().rawData(), (std::vector<double>{20, 4}));
  }

private:
  std::mutex m_callbackMutex;
  std::condition_variable m_callbackCondition;
  uint8_t m_niterations = 0;
};

#endif /* MANTID_LIVEDATA_KAFKAHISTOSTREAMDECODERTEST_H_ */
