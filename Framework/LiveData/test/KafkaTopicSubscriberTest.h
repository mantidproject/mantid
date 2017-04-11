#ifndef MANTID_LIVEDATA_KAFKATOPICSUBSCRIBERTEST_H_
#define MANTID_LIVEDATA_KAFKATOPICSUBSCRIBERTEST_H_

#include "MantidKernel/make_unique.h"
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"
#include <cxxtest/TestSuite.h>
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
    std::string broker("badhost"), topic("topicstring");

    // This won't connect yet
    auto subscriber =
        Mantid::Kernel::make_unique<KafkaTopicSubscriber>(broker, topic);

    TS_ASSERT_EQUALS(topic, subscriber->topic());
  }

  void xtest_Unknown_Topic_Throws_Error() {
    using Mantid::LiveData::KafkaTopicSubscriber;
    std::string broker("sakura"), topic("__NOT_A_TOPIC_LETS_NOT_FIND_THIS");

    // This won't connect yet
    auto subscriber =
        Mantid::Kernel::make_unique<KafkaTopicSubscriber>(broker, topic);

    TS_ASSERT_THROWS(subscriber->subscribe(), std::runtime_error);
  }

  // ---------------------------------------------------------------------------
  // Failure cases
  // ---------------------------------------------------------------------------
  void xtest_BadHost_Throws_Error() {
    using Mantid::LiveData::KafkaTopicSubscriber;
    std::string broker("badhost"), topic("topic");

    // This won't connect yet
    auto subscriber =
        Mantid::Kernel::make_unique<KafkaTopicSubscriber>(broker, topic);

    TS_ASSERT_THROWS(subscriber->subscribe(), std::runtime_error);
  }
};

#endif /* MANTID_LIVEDATA_KAFKATOPICSUBSCRIBERTEST_H_ */
