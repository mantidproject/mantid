#ifndef MANTID_LIVEDATA_KAFKATOPICSUBSCRIBERTEST_H_
#define MANTID_LIVEDATA_KAFKATOPICSUBSCRIBERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"
#include "MantidKernel/make_unique.h"
#include <memory>

class KafkaTopicSubscriberTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static KafkaTopicSubscriberTest *createSuite() {
    return new KafkaTopicSubscriberTest();
  }
  static void destroySuite(KafkaTopicSubscriberTest *suite) { delete suite; }

  // ---------------------------------------------------------------------------
  // Success cases
  // ---------------------------------------------------------------------------
  void test_Connection_Properties_Returned_As_Expected() {
    using Mantid::LiveData::KafkaTopicSubscriber;
    std::string broker("badhost"), topic("SANS2Devent_data");

    auto subscriber =
        Mantid::Kernel::make_unique<KafkaTopicSubscriber>(broker, topic);

    TS_ASSERT_EQUALS(topic, subscriber->topic());
  }

  void test_Real_Connection_To_Test_Server() {
    using Mantid::LiveData::KafkaTopicSubscriber;
    std::string broker("sakura"), topic("SANS2Devent_data");

    auto subscriber =
        Mantid::Kernel::make_unique<KafkaTopicSubscriber>(broker, topic);

    TS_ASSERT_THROWS_NOTHING(subscriber->subscribe());
    std::string data;
    size_t msgCount(0);
    while(msgCount < 100 && subscriber->consumeMessage(&data)) {
      std::cerr << "received  " << data.size() << "bytes\n";
      ++msgCount;
    }
  }
  
  // ---------------------------------------------------------------------------
  // Failure cases
  // ---------------------------------------------------------------------------

};

#endif /* MANTID_LIVEDATA_KAFKATOPICSUBSCRIBERTEST_H_ */
