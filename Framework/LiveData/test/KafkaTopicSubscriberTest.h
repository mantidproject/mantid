// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_KAFKATOPICSUBSCRIBERTEST_H_
#define MANTID_LIVEDATA_KAFKATOPICSUBSCRIBERTEST_H_


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
    using Mantid::LiveData::SubscribeAtOption;
    std::string broker("badhost"), topic("topicstring");

    // This won't connect yet
    std::vector<std::string> topics = {topic};
    auto subscriber = std::make_unique<KafkaTopicSubscriber>(
        broker, topics, SubscribeAtOption::LATEST);

    TS_ASSERT_EQUALS(topic, subscriber->topics()[0]);
  }
};

#endif /* MANTID_LIVEDATA_KAFKATOPICSUBSCRIBERTEST_H_ */
