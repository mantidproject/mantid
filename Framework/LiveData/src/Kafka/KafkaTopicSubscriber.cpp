#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"
#include "MantidKernel/Logger.h"

#include <algorithm>
#include <cassert>
#include <sstream>

using RdKafka::Conf;
using RdKafka::KafkaConsumer;
using RdKafka::Metadata;
using RdKafka::TopicMetadata;

namespace {
/// Timeout for message consume
const int CONSUME_TIMEOUT_MS = 30000;
/// A reference to the static logger
Mantid::Kernel::Logger &LOGGER() {
  static Mantid::Kernel::Logger logger("KafkaTopicSubscriber");
  return logger;
}

/// Create and return the global configuration object
std::unique_ptr<Conf> createGlobalConfiguration(const std::string &brokerAddr) {
  auto conf = std::unique_ptr<Conf>(Conf::create(Conf::CONF_GLOBAL));
  std::string errorMsg;
  conf->set("metadata.broker.list", brokerAddr, errorMsg);
  conf->set("session.timeout.ms", "10000", errorMsg);
  conf->set("group.id", "mantid", errorMsg);
  conf->set("message.max.bytes", "10000000", errorMsg);
  conf->set("fetch.message.max.bytes", "10000000", errorMsg);
  conf->set("replica.fetch.max.bytes", "10000000", errorMsg);
  conf->set("enable.auto.commit", "false", errorMsg);
  conf->set("enable.auto.offset.store", "false", errorMsg);
  conf->set("offset.store.method", "none", errorMsg);
  return conf;
}

/// Create and return a topic configuration object for a given global
/// configuration
std::unique_ptr<Conf> createTopicConfiguration(Conf *globalConf) {
  auto conf = std::unique_ptr<Conf>(Conf::create(Conf::CONF_TOPIC));
  std::string errorMsg;
  // default to start consumption from the end of the topic
  // NB, can be circumvented by calling assign rather than subscribe
  conf->set("auto.offset.reset", "largest", errorMsg);

  // tie the global config to this topic configuration
  globalConf->set("default_topic_conf", conf.get(), errorMsg);
  return conf;
}
}

namespace Mantid {
namespace LiveData {

// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------

const std::string KafkaTopicSubscriber::EVENT_TOPIC_SUFFIX = "_events";
const std::string KafkaTopicSubscriber::RUN_TOPIC_SUFFIX = "_runInfo";
const std::string KafkaTopicSubscriber::DET_SPEC_TOPIC_SUFFIX = "_detSpecMap";

/**
 * Construct a topic subscriber
 * @param broker The host:port address of the broker
 * @param topics Name of the topics
 */
KafkaTopicSubscriber::KafkaTopicSubscriber(std::string broker,
                                           std::vector<std::string> topics,
                                           subscribeAtOption subscribeOption)
    : IKafkaStreamSubscriber(), m_consumer(), m_brokerAddr(broker),
      m_topicNames(topics), m_subscribeOption(subscribeOption) {}

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
 * @return The names of the topics subscription
 */
std::vector<std::string> KafkaTopicSubscriber::topic() const {
  return m_topicNames;
}

/**
 * Setup the connection to the broker for the configured topic
 */
void KafkaTopicSubscriber::subscribe() { subscribe(IGNORE_OFFSET); }

/**
 * Setup the connection to the broker for the configured topic
 * at a specified offset
 * @param offset at which to start listening on topic
 */
void KafkaTopicSubscriber::subscribe(int64_t offset) {
  createConsumer();
  checkTopicsExist();
  subscribeAtOffset(offset);
}

/**
 * Get list of partitions on configured topic
 * @return list of TopicPartitions
 */
std::vector<RdKafka::TopicPartition *>
KafkaTopicSubscriber::getTopicPartitions() {
  std::vector<RdKafka::TopicPartition *> partitions;
  subscribe();
  auto error = m_consumer->assignment(partitions);
  if (error != RdKafka::ERR_NO_ERROR) {
    throw std::runtime_error("In KafkaTopicSubscriber failed to get "
                             "TopicPartition information from Kafka brokers.");
  }
  m_consumer->unsubscribe();
  return partitions;
}

/**
 * Setup the connection to the broker for the configured topic
 * at a specified time
 * @param time (milliseconds since 1 Jan 1970) at which to start listening on
 * topic
 */
void KafkaTopicSubscriber::subscribeAtTime(int64_t time) {
  // Get list of TopicPartitions for the topic
  auto partitions = getTopicPartitions();
  for (auto partition : partitions) {
    partition->set_offset(time);
  }
  // Convert the timestamps to partition offsets
  auto error = m_consumer->offsetsForTimes(partitions, 2000);
  if (error != RdKafka::ERR_NO_ERROR) {
    throw std::runtime_error("In KafkaTopicSubscriber failed to lookup "
                             "partition offsets for specified start time.");
  }
  error = m_consumer->assign(partitions);
  reportSuccessOrFailure(error, 0);
}

/**
 * Create the KafkaConsumer for required configuration
 */
void KafkaTopicSubscriber::createConsumer() {
  // Create configurations
  auto globalConf = createGlobalConfiguration(m_brokerAddr);
  auto topicConf = createTopicConfiguration(globalConf.get());

  std::string errorMsg;
  m_consumer = std::unique_ptr<KafkaConsumer>(
      KafkaConsumer::create(globalConf.get(), errorMsg));
  if (!m_consumer) {
    std::ostringstream os;
    os << "Failed to create Kafka consumer: '" << errorMsg << "'";
    throw std::runtime_error(os.str());
  }
  LOGGER().debug() << "% Created consumer " << m_consumer->name() << std::endl;
}

/**
 * Check that the topic we want to subscribe to exists on the Kafka brokers
 */
void KafkaTopicSubscriber::checkTopicsExist() const {
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
  for (const auto &topicName : m_topicNames) {
    auto iter = std::find_if(topics->cbegin(), topics->cend(),
                             [topicName](const TopicMetadata *tpc) {
                               return tpc->topic() == topicName;
                             });
    if (iter == topics->cend()) {
      std::ostringstream os;
      os << "Failed to find topic '" << topicName << "' on broker";
      throw std::runtime_error(os.str());
    }
  }
}

/**
 * Subscribe to a topic at the required offset using rdkafka::assign()
 */
void KafkaTopicSubscriber::subscribeAtOffset(int64_t offset) {

  // Offset of message to start at
  int64_t confOffset = -1;
  RdKafka::ErrorCode error = RdKafka::ERR_NO_ERROR;
  std::vector<RdKafka::TopicPartition *> topicPartitions;

  for (const auto &topicName : m_topicNames) {

    if (m_subscribeOption == subscribeAtOption::TIME) {
      subscribeAtTime(offset);
      return;
    }

    const int partitionId = 0;
    auto topicPartition =
        RdKafka::TopicPartition::create(topicName, partitionId);
    int64_t lowOffset, highOffset = 0;
    // This gets the lowest and highest offsets available on the brokers
    m_consumer->query_watermark_offsets(topicName, partitionId, &lowOffset,
                                        &highOffset, -1);

    switch (m_subscribeOption) {
    case subscribeAtOption::LATEST:
      confOffset = highOffset;
      break;
    case subscribeAtOption::LASTONE:
      confOffset = highOffset - 1;
      break;
    case subscribeAtOption::LASTTWO:
      confOffset = highOffset - 2;
      // unless there is only one message on the topic
      if (confOffset == -1)
        confOffset = 0;
      break;
    case subscribeAtOption::OFFSET:
      confOffset = offset;
      break;
    default:
      throw std::runtime_error("Unexpected subscribe option in "
                               "KafkaTopicSubscriber::subscribeAtOffset");
    }

    topicPartition->set_offset(confOffset);
    topicPartitions.push_back(topicPartition);
  }
  error = m_consumer->assign(topicPartitions);
  reportSuccessOrFailure(error, confOffset);
}

/**
 * Report whether subscribing to topic was successful
 *
 * @param error : rdkafka error code
 * @param confOffset : offset to start receiving messages at
 */
void KafkaTopicSubscriber::reportSuccessOrFailure(
    const RdKafka::ErrorCode &error, int64_t confOffset) const {
  if (confOffset < 0) {
    std::ostringstream os;
    os << "No messages are yet available on the Kafka brokers for one "
          "or more of these topics: '";
    for (const auto &topicName : m_topicNames) {
      os << topicName << ", ";
    }
    os << "'";
    throw std::runtime_error(os.str());
  }
  if (error) {
    std::ostringstream os;
    os << "Failed to subscribe to topic: '" << err2str(error) << "'";
    throw std::runtime_error(os.str());
  }
  LOGGER().debug() << "Successfully subscribed to topics '";
  for (const auto &topicName : m_topicNames) {
    LOGGER().debug() << topicName << ", ";
  }
  LOGGER().debug() << "'\n";
}

/**
 * Consume a message from the stream. A runtime_error is raised if
 *   - kafka indicates no error but the msg is empty
 *   - kafka indicates anything other than a timeout or end of partition error
 * A timeout or EOF are not treated as exceptional so that the client may keep
 * polling without having to catch all errors.
 * @param payload Output parameter filled with message payload. This is
 * cleared
 * on entry into the method
 */
void KafkaTopicSubscriber::consumeMessage(std::string *payload) {
  using RdKafka::Message;
  using RdKafka::err2str;
  assert(m_consumer);
  assert(payload);

  payload->clear();
  auto kfMsg =
      std::unique_ptr<Message>(m_consumer->consume(CONSUME_TIMEOUT_MS));

  switch (kfMsg->err()) {
  case RdKafka::ERR_NO_ERROR:
    // Real message
    if (kfMsg->len() > 0) {
      payload->assign(static_cast<const char *>(kfMsg->payload()),
                      static_cast<int>(kfMsg->len()));
    } else {
      // If RdKafka indicates no error then we should always get a
      // non-zero length message
      throw std::runtime_error("KafkaTopicSubscriber::consumeMessage() - Kafka "
                               "indicated no error but a zero-length payload "
                               "was received");
    }
    break;

  case RdKafka::ERR__TIMED_OUT:
  case RdKafka::ERR__PARTITION_EOF:
    // Not errors as the broker might come back or more data might be pushed
    break;

  default:
    /* All other errors */
    std::ostringstream os;
    os << "KafkaTopicSubscriber::consumeMessage() - "
       << RdKafka::err2str(kfMsg->err());
    throw std::runtime_error(os.str());
  }
}
}
}
