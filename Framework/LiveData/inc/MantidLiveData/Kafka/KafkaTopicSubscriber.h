#ifndef MANTID_LIVEDATA_KAFKAEVENTSUBSCRIBER_H_
#define MANTID_LIVEDATA_KAFKAEVENTSUBSCRIBER_H_

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

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport KafkaTopicSubscriber final : public IKafkaStreamSubscriber {
public:
  KafkaTopicSubscriber(std::string broker, std::vector<std::string> topics,
                       SubscribeAtOption subscribeOption);
  ~KafkaTopicSubscriber() override;

  std::vector<std::string> topics() const;

  void subscribe() override;
  void subscribe(int64_t offset) override;
  void consumeMessage(std::string *payload, int64_t &offset, int32_t &partition,
                      std::string &topic) override;
  std::unordered_map<std::string, std::vector<int64_t>>
  getOffsetsForTimestamp(int64_t timestamp) override;
  void seek(const std::string &topic, uint32_t partition,
            int64_t offset) override;
  std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() override;

  static const std::string EVENT_TOPIC_SUFFIX;
  static const std::string HISTO_TOPIC_SUFFIX;
  static const std::string RUN_TOPIC_SUFFIX;
  static const std::string DET_SPEC_TOPIC_SUFFIX;
  static const std::string SAMPLE_ENV_TOPIC_SUFFIX;

  static const int64_t IGNORE_OFFSET = -1;

private:
  std::unique_ptr<RdKafka::KafkaConsumer> m_consumer;
  std::string m_brokerAddr;
  std::vector<std::string> m_topicNames;
  SubscribeAtOption m_subscribeOption = SubscribeAtOption::OFFSET;

  void subscribeAtTime(int64_t time);
  void reportSuccessOrFailure(const RdKafka::ErrorCode &error,
                              int64_t confOffset) const;

  void subscribeAtOffset(int64_t offset);
  void checkTopicsExist() const;
  void createConsumer();
  int64_t getCurrentOffset(const std::string &topic, int partition);
  std::vector<RdKafka::TopicPartition *> getTopicPartitions();
  std::unique_ptr<RdKafka::Metadata> queryMetadata() const;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_KAFKAEVENTSUBSCRIBER_H_ */
