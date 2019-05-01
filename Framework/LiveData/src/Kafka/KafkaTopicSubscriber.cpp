// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"
#include "MantidKernel/Logger.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <sstream>
#include <thread>

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
  conf->set("message.max.bytes", "25000000", errorMsg);
  conf->set("fetch.message.max.bytes", "25000000", errorMsg);
  conf->set("replica.fetch.max.bytes", "25000000", errorMsg);
  conf->set("enable.auto.commit", "false", errorMsg);
  conf->set("enable.auto.offset.store", "false", errorMsg);
  conf->set("offset.store.method", "none", errorMsg);
  conf->set("api.version.request", "true", errorMsg);
  return conf;
}
} // namespace

namespace Mantid {
namespace LiveData {

// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------

const std::string KafkaTopicSubscriber::EVENT_TOPIC_SUFFIX = "_events";
const std::string KafkaTopicSubscriber::HISTO_TOPIC_SUFFIX = "_eventSum";
const std::string KafkaTopicSubscriber::RUN_TOPIC_SUFFIX = "_runInfo";
const std::string KafkaTopicSubscriber::DET_SPEC_TOPIC_SUFFIX = "_detSpecMap";
const std::string KafkaTopicSubscriber::SAMPLE_ENV_TOPIC_SUFFIX = "_sampleEnv";

/**
 * Construct a topic subscriber
 * @param broker The host:port address of the broker
 * @param topics Name of the topics
 */
KafkaTopicSubscriber::KafkaTopicSubscriber(std::string broker,
                                           std::vector<std::string> topics,
                                           SubscribeAtOption subscribeOption)
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
std::vector<std::string> KafkaTopicSubscriber::topics() const {
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
 * Construct list of partitions on configured topics
 * @return list of TopicPartitions
 */
std::vector<RdKafka::TopicPartition *>
KafkaTopicSubscriber::getTopicPartitions() {
  std::vector<RdKafka::TopicPartition *> partitions;
  auto metadata = queryMetadata();
  auto topics = metadata->topics();
  // Search through all topics for the ones we are interested in
  for (const auto &topicName : m_topicNames) {
    auto iter = std::find_if(topics->cbegin(), topics->cend(),
                             [topicName](const TopicMetadata *tpc) {
                               return tpc->topic() == topicName;
                             });
    auto matchedTopic = *iter;
    auto partitionMetadata = matchedTopic->partitions();
    auto numberOfPartitions = partitionMetadata->size();
    // Create a TopicPartition for each partition in the topic
    for (size_t partitionNumber = 0; partitionNumber < numberOfPartitions;
         ++partitionNumber) {
      auto topicPartition = RdKafka::TopicPartition::create(
          topicName, static_cast<int>(partitionNumber));
      partitions.push_back(topicPartition);
    }
  }
  return partitions;
}

/**
 * Get the current offsets the consumer has reached in each topic
 * @return map with key of topic name and value of vector of offsets for its
 * partitions
 */
std::unordered_map<std::string, std::vector<int64_t>>
KafkaTopicSubscriber::getCurrentOffsets() {
  std::unordered_map<std::string, std::vector<int64_t>> currentOffsets;
  std::vector<RdKafka::TopicPartition *> partitions;
  auto error = m_consumer->assignment(partitions);
  if (error != RdKafka::ERR_NO_ERROR) {
    throw std::runtime_error("In KafkaTopicSubscriber failed to lookup "
                             "current partition assignment.");
  }
  error = m_consumer->position(partitions);
  if (error != RdKafka::ERR_NO_ERROR) {
    throw std::runtime_error("In KafkaTopicSubscriber failed to lookup "
                             "current partition positions.");
  }
  for (auto topicPartition : partitions) {
    std::vector<int64_t> offsetList = {topicPartition->offset()};
    auto result = currentOffsets.emplace(
        std::make_pair(topicPartition->topic(), offsetList));
    if (!result.second) {
      // If we could not emplace a new pair then the key already exists, so
      // append the offset to the vector belonging to the existing topic key
      currentOffsets[topicPartition->topic()].push_back(
          topicPartition->offset());
    }
  }
  return currentOffsets;
}

/**
 * Get metadata from the Kafka brokers
 * @return metadata
 */
std::unique_ptr<Metadata> KafkaTopicSubscriber::queryMetadata() const {
  Metadata *metadataRawPtr(nullptr);
  // API requires address of a pointer to the struct but compiler won't allow
  // &metadata.get() as it is an rvalue
  m_consumer->metadata(true, nullptr, &metadataRawPtr, CONSUME_TIMEOUT_MS);
  // Capture the pointer in an owning struct to take care of deletion
  std::unique_ptr<Metadata> metadata(std::move(metadataRawPtr));
  if (!metadata) {
    throw std::runtime_error("Failed to query metadata from broker");
  }
  return metadata;
}

/**
 * Setup the connection to the broker for the configured topics
 * at a specified time
 * @param time (milliseconds since 1 Jan 1970) at which to start listening on
 * topic
 */
void KafkaTopicSubscriber::subscribeAtTime(int64_t time) {
  auto partitions = getTopicPartitions();
  std::for_each(partitions.cbegin(), partitions.cend(),
                [time](RdKafka::TopicPartition *partition) {
                  partition->set_offset(time);
                });

  // Convert the timestamps to partition offsets
  auto error = m_consumer->offsetsForTimes(partitions, 10000);
  if (error != RdKafka::ERR_NO_ERROR) {
    throw std::runtime_error("In KafkaTopicSubscriber failed to lookup "
                             "partition offsets for specified start time.");
  }
  LOGGER().debug("Called offsetsForTimes");

  if (LOGGER().debug()) {
    for (auto partition : partitions) {
      LOGGER().debug() << "Topic: " << partition->topic()
                       << ", partition: " << partition->partition()
                       << ", time (milliseconds past epoch): " << time
                       << ", looked up offset as: " << partition->offset()
                       << ", current high watermark is: "
                       << getCurrentOffset(partition->topic(),
                                           partition->partition())
                       << std::endl;
    }
  }

  error = m_consumer->assign(partitions);

  // Clean up topicPartition pointers
  std::for_each(partitions.cbegin(), partitions.cend(),
                [](RdKafka::TopicPartition *partition) { delete partition; });
  reportSuccessOrFailure(error, 0);
}

/**
 * Query the broker for the current high watermark offset for a particular topic
 * and partition, useful for debugging
 * @param topic : topic name
 * @param partition : partition number
 * @return high watermark offset
 */
int64_t KafkaTopicSubscriber::getCurrentOffset(const std::string &topic,
                                               int partition) {
  int64_t lowOffset = 0;
  int64_t highOffset = 0;
  auto err = m_consumer->query_watermark_offsets(topic, partition, &lowOffset,
                                                 &highOffset, -1);
  if (err != RdKafka::ERR_NO_ERROR) {
    LOGGER().debug()
        << "Failed to query current high watermark offset, returning as -1 "
        << RdKafka::err2str(err) << std::endl;
    return -1;
  }
  return highOffset;
}

/**
 * Seek to given offset on specified topic and partition
 *
 * @param topic : topic name
 * @param partition : partition number
 * @param offset : offset to seek to
 */
void KafkaTopicSubscriber::seek(const std::string &topic, uint32_t partition,
                                int64_t offset) {
  auto topicPartition = RdKafka::TopicPartition::create(topic, partition);
  topicPartition->set_offset(offset);
  auto error = m_consumer->seek(*topicPartition, 2000);
  if (error) {
    std::ostringstream os;
    os << "Offset seek failed with error: '" << err2str(error) << "'";
    throw std::runtime_error(os.str());
  }
  LOGGER().debug() << "Successful seek of topic: " << topic
                   << ", partition: " << partition << " to offset: " << offset
                   << std::endl;
}

/**
 * Create the KafkaConsumer for required configuration
 */
void KafkaTopicSubscriber::createConsumer() {
  // Create configurations
  auto globalConf = createGlobalConfiguration(m_brokerAddr);

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
  auto metadata = queryMetadata();
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

  if (m_subscribeOption == SubscribeAtOption::TIME) {
    subscribeAtTime(offset);
    return;
  }

  for (const auto &topicName : m_topicNames) {
    const int partitionId = 0;
    auto topicPartition =
        RdKafka::TopicPartition::create(topicName, partitionId);
    int64_t lowOffset, highOffset = 0;
    // This gets the lowest and highest offsets available on the brokers
    m_consumer->query_watermark_offsets(topicName, partitionId, &lowOffset,
                                        &highOffset, -1);

    switch (m_subscribeOption) {
    case SubscribeAtOption::LATEST:
      confOffset = highOffset;
      break;
    case SubscribeAtOption::LASTONE:
      confOffset = highOffset - 1;
      break;
    case SubscribeAtOption::LASTTWO:
      confOffset = highOffset - 2;
      // unless there is only one message on the topic
      if (confOffset == -1)
        confOffset = 0;
      break;
    case SubscribeAtOption::OFFSET:
      confOffset = offset;
      break;
    default:
      throw std::runtime_error("Unexpected subscribe option in "
                               "KafkaTopicSubscriber::subscribeAtOffset");
    }

    topicPartition->set_offset(confOffset);
    topicPartitions.push_back(topicPartition);
  }
  LOGGER().debug() << "Attempting to subscribe to " << topicPartitions.size()
                   << " partitions in KafkaTopicSubscriber::subscribeAtOffset()"
                   << std::endl;
  error = m_consumer->assign(topicPartitions);

  // Clean up topicPartition pointers
  std::for_each(topicPartitions.cbegin(), topicPartitions.cend(),
                [](RdKafka::TopicPartition *partition) { delete partition; });

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
void KafkaTopicSubscriber::consumeMessage(std::string *payload, int64_t &offset,
                                          int32_t &partition,
                                          std::string &topic) {
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
      offset = kfMsg->offset();
      partition = kfMsg->partition();
      topic = kfMsg->topic_name();
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

/**
 * Offsets of the messages whose timestamps are greater than or equal to the
 * given target time of each partition.
 * @param timestamp : timestamp at which to get corresponding offsets
 * @return : map with topic names as key with a vector of offsets for the
 * partitions
 */
std::unordered_map<std::string, std::vector<int64_t>>
KafkaTopicSubscriber::getOffsetsForTimestamp(int64_t timestamp) {
  auto partitions = getTopicPartitions();
  for (auto partition : partitions) {
    partition->set_offset(timestamp);
  }
  // Convert the timestamps to partition offsets
  auto error = m_consumer->offsetsForTimes(partitions, 2000);
  if (error != RdKafka::ERR_NO_ERROR) {
    throw std::runtime_error("In KafkaTopicSubscriber failed to lookup "
                             "partition offsets for specified time.");
  }

  // Preallocate map
  auto metadata = queryMetadata();
  auto topics = metadata->topics();
  std::unordered_map<std::string, std::vector<int64_t>> partitionOffsetMap;
  std::for_each(
      topics->cbegin(), topics->cend(),
      [&partitionOffsetMap, this](const TopicMetadata *tpc) {
        if (std::find(m_topicNames.cbegin(), m_topicNames.cend(),
                      tpc->topic()) != m_topicNames.cend()) {
          partitionOffsetMap.insert(
              {tpc->topic(), std::vector<int64_t>(tpc->partitions()->size())});
        }
      });

  // Get the offsets from the topic partitions and add them to map
  for (auto partition : partitions) {
    auto offset = partition->offset();
    if (offset < 0)
      offset = getCurrentOffset(partition->topic(), partition->partition()) - 1;
    partitionOffsetMap[partition->topic()][partition->partition()] = offset;
  }

  // Clean up topicPartition pointers
  std::for_each(partitions.cbegin(), partitions.cend(),
                [](RdKafka::TopicPartition *partition) { delete partition; });

  return partitionOffsetMap;
}
} // namespace LiveData
} // namespace Mantid
