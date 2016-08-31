#ifndef MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_
#define MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidLiveData/ISIS/ISISKafkaEventStreamDecoder.h"
#include "MantidKernel/make_unique.h"
#include "ISISKafkaTesting.h"

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

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void xtest_Single_Period_Event_Stream() {
    using namespace ::testing;
    using namespace ISISKafkaTesting;
    using namespace Mantid::LiveData;

    MockKafkaBroker mockBroker;
    EXPECT_CALL(mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeISISSinglePeriodStreamSubscriber))
        .WillOnce(Return(new FakeISISRunInfoStreamSubscriber))
        .WillOnce(Return(new FakeISISSpDetStreamSubscriber));
    auto decoder = createTestDecoder(mockBroker);
    TS_ASSERT_THROWS_NOTHING(decoder->startCapture());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    TS_ASSERT_THROWS_NOTHING(decoder->extractData());
    TS_ASSERT_THROWS_NOTHING(decoder->stopCapture());
    TS_ASSERT(!decoder->isRunning());
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Empty_Stream_Throws_Error_On_ExtractData() {
    using namespace ::testing;
    using namespace ISISKafkaTesting;

    MockKafkaBroker mockBroker;
    EXPECT_CALL(mockBroker, subscribe_(_))
        .Times(Exactly(3))
        .WillOnce(Return(new FakeEmptyStreamSubscriber))
        .WillOnce(Return(new FakeEmptyStreamSubscriber))
        .WillOnce(Return(new FakeEmptyStreamSubscriber));
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
};

#endif /* MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_ */
