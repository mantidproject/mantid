#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"
#include "MantidKernel/Logger.h"

#include <librdkafka/rdkafkacpp.h>

#include <algorithm>
#include <cassert>
#include <sstream>

namespace {
/// Timeout for message consume
const int CONSUME_TIMEOUT_MS = 30000;

Mantid::Kernel::Logger &LOGGER() {
  static Mantid::Kernel::Logger logger("KafkaTopicSubscriber");
  return logger;
}
}

namespace Mantid {
namespace LiveData {

// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------

/**
 * Construct a topic subscriber
 * @param broker The host:port address of the broker
 * @param topic Name of the topic
 */
KafkaTopicSubscriber::KafkaTopicSubscriber(std::string broker,
                                           std::string topic)
    : IKafkaStreamSubscriber(), m_consumer(), m_brokerAddr(broker),
      m_topicName(topic) {}

/// Destructor
KafkaTopicSubscriber::~KafkaTopicSubscriber() {
  if (m_consumer) {
    m_consumer->close();
    /*
     * Wait for RdKafka to decommission.
     * This is not strictly needed (with check outq_len() above), but
     * allows RdKafka to clean up all its resources before the application
     * exits so that memory profilers such as valgrind wont complain about
     * memory leaks.
     */
    RdKafka::wait_destroyed(5000);
  }
}

/**
 * @return The name of the topic subscription
 */
const std::string KafkaTopicSubscriber::topic() const { return m_topicName; }

/**
 * Setup the connection to the broker for the configured topic
 */
void KafkaTopicSubscriber::subscribe() {
  using namespace RdKafka;

  auto globalConf = std::unique_ptr<Conf>(Conf::create(Conf::CONF_GLOBAL));
  std::string errorMsg;
  globalConf->set("metadata.broker.list", m_brokerAddr, errorMsg);
  globalConf->set("session.timeout.ms", "10000", errorMsg);
  globalConf->set("group.id", "mantid", errorMsg);
  globalConf->set("message.max.bytes", "10000000", errorMsg);
  globalConf->set("fetch.message.max.bytes", "10000000", errorMsg);
  globalConf->set("replica.fetch.max.bytes", "10000000", errorMsg);
  auto topicConf = std::unique_ptr<Conf>(Conf::create(Conf::CONF_TOPIC));
  globalConf->set("default_topic_conf", topicConf.get(), errorMsg);

  // Create consumer using accumulated global configuration.
  m_consumer = std::unique_ptr<KafkaConsumer>(
      KafkaConsumer::create(globalConf.get(), errorMsg));
  if (!m_consumer) {
    std::ostringstream os;
    os << "Failed to create Kafka consumer: '" << errorMsg << "'";
    throw std::runtime_error(os.str());
  }
  LOGGER().debug() << "% Created consumer " << m_consumer->name() << std::endl;

  Metadata *metadataRawPtr(nullptr);
  // API requires address of a pointer to the struct but compiler won't allow
  // &metadata.get() as it is an rvalue
  m_consumer->metadata(true, nullptr, &metadataRawPtr, CONSUME_TIMEOUT_MS);
  // Capture the pointer in an owning struct to take care of deletion
  std::unique_ptr<Metadata> metadata(std::move(metadataRawPtr));
  if (!metadata) {
    throw std::runtime_error("Failed to query metadata from broker");
  }
  auto topics = metadata->topics();
  auto iter = std::find_if(topics->cbegin(), topics->cend(),
                           [this](const TopicMetadata *tpc) {
                             return tpc->topic() == this->m_topicName;
                           });
  if(iter == topics->cend()) {
    std::ostringstream os;
    os << "Failed to find topic '" << m_topicName << "' on broker";
    throw std::runtime_error(os.str());
  }

  auto error = m_consumer->subscribe({m_topicName});
  if (error) {
    std::ostringstream os;
    os << "Failed to subscribe to topic: '" << RdKafka::err2str(error) << "'";
    throw std::runtime_error(os.str());
  }
  LOGGER().debug() << "Successfully subscribed to topic '" << m_topicName
                   << "'\n";
}

/**
 * Consume a message from the stream and indicate if there is anything
 * further to read
 * @param payload Output parameter filled with message payload
 * @return True if the read was considered successful
 */
bool KafkaTopicSubscriber::consumeMessage(std::string *payload) {
  assert(m_consumer);
  using RdKafka::Message;
  auto kfMsg =
      std::unique_ptr<Message>(m_consumer->consume(CONSUME_TIMEOUT_MS));

  bool success = false;
  switch (kfMsg->err()) {
  case RdKafka::ERR__TIMED_OUT:
    // Not an error as the broker might come back
    success = true;
    break;

  case RdKafka::ERR_NO_ERROR:
    /* Real message */
    if (kfMsg->len() > 0) {
      success = true;
      payload->assign(static_cast<const char *>(kfMsg->payload()),
                      static_cast<int>(kfMsg->len()));
    }
    break;
  case RdKafka::ERR__PARTITION_EOF:
    // End of stream - not an error as we just keep waiting
    payload->clear();
    success = true;
  default:
    /* Errors */
    success = false;
  }
  return success;
}
}
}
