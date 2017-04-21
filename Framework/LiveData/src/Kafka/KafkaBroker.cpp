#include "MantidLiveData/Kafka/KafkaBroker.h"
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"

#include "MantidKernel/make_unique.h"

namespace Mantid {
namespace LiveData {

/**
 * Constructor accepting the address of a broker.
 * @param address The address of a broker in the form host:port
 */
KafkaBroker::KafkaBroker(std::string address)
    : IKafkaBroker(), m_address(address) {}

/**
 * Create an object to provide access to a topic stream from this broker
 * @param topic The name of a topic
 * @return A new IKafkaStreamSubscriber object
 */
std::unique_ptr<IKafkaStreamSubscriber>
KafkaBroker::subscribe(const std::string &topic) const {
  auto subscriber = Kernel::make_unique<KafkaTopicSubscriber>(m_address, topic);
  subscriber->subscribe();
  return std::move(subscriber);
}

std::unique_ptr<IKafkaStreamSubscriber>
KafkaBroker::subscribe(const std::string &topic, int64_t offset) const {
  auto subscriber = Kernel::make_unique<KafkaTopicSubscriber>(m_address, topic);
  subscriber->subscribe(offset);
  return std::move(subscriber);
}

} // namespace LiveData
} // namespace Mantid
