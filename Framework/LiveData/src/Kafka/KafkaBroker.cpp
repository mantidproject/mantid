// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidLiveData/Kafka/KafkaBroker.h"
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"

namespace Mantid::LiveData {

/**
 * Constructor accepting the address of a broker.
 * @param address The address of a broker in the form host:port
 */
KafkaBroker::KafkaBroker(std::string address) : IKafkaBroker(), m_address(std::move(address)) {}

/**
 * Create an object to provide access to a topic stream from this broker
 * @param topic The name of a topic
 * @return A new IKafkaStreamSubscriber object
 */
std::unique_ptr<IKafkaStreamSubscriber> KafkaBroker::subscribe(std::vector<std::string> topics,
                                                               SubscribeAtOption subscribeOption) const {
  auto subscriber = std::make_unique<KafkaTopicSubscriber>(m_address, topics, subscribeOption);
  subscriber->subscribe();
  return subscriber;
}

std::unique_ptr<IKafkaStreamSubscriber> KafkaBroker::subscribe(std::vector<std::string> topics, int64_t offset,
                                                               SubscribeAtOption subscribeOption) const {
  auto subscriber = std::make_unique<KafkaTopicSubscriber>(m_address, topics, subscribeOption);
  subscriber->subscribe(offset);
  return subscriber;
}

} // namespace Mantid::LiveData
