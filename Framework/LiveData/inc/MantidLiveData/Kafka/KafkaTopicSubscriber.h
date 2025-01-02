// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"
#include <librdkafka/rdkafkacpp.h>
#include <memory>

// -----------------------------------------------------------------------------
// RdKafka forward declarations
// -----------------------------------------------------------------------------
namespace RdKafka {
class KafkaConsumer;
}

namespace Mantid {
namespace LiveData {

/**
  Interface to a named Kafka topic on a broker at a given address.
*/
class MANTID_LIVEDATA_DLL KafkaTopicSubscriber final : public IKafkaStreamSubscriber {
public:
  KafkaTopicSubscriber(std::string broker, std::vector<std::string> topics, SubscribeAtOption subscribeOption);
  ~KafkaTopicSubscriber() override;

  std::vector<std::string> topics() const;

  void subscribe() override;
  void subscribe(int64_t offset) override;
  void consumeMessage(std::string *payload, int64_t &offset, int32_t &partition, std::string &topic) override;
  std::unordered_map<std::string, std::vector<int64_t>> getOffsetsForTimestamp(int64_t timestamp) override;
  void seek(const std::string &topic, uint32_t partition, int64_t offset) override;
  std::unordered_map<std::string, std::vector<int64_t>> getCurrentOffsets() override;

  static const std::string EVENT_TOPIC_SUFFIX;
  static const std::string HISTO_TOPIC_SUFFIX;
  static const std::string RUN_TOPIC_SUFFIX;
  static const std::string SAMPLE_ENV_TOPIC_SUFFIX;
  static const std::string CHOPPER_TOPIC_SUFFIX;
  static const std::string MONITOR_TOPIC_SUFFIX;

  static const int64_t IGNORE_OFFSET = -1;

private:
  std::unique_ptr<RdKafka::KafkaConsumer> m_consumer;
  std::string m_brokerAddr;
  std::vector<std::string> m_topicNames;
  SubscribeAtOption m_subscribeOption = SubscribeAtOption::OFFSET;

  void subscribeAtTime(int64_t time);
  void reportSuccessOrFailure(const RdKafka::ErrorCode &error, int64_t confOffset) const;

  void subscribeAtOffset(int64_t offset);
  void checkTopicsExist() const;
  void createConsumer();
  int64_t getCurrentOffset(const std::string &topic, int partition);
  std::vector<RdKafka::TopicPartition *> getTopicPartitions();
  std::unique_ptr<RdKafka::Metadata> queryMetadata() const;
};

} // namespace LiveData
} // namespace Mantid
