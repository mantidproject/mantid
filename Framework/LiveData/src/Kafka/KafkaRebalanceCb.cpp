#include "MantidLiveData/Kafka/KafkaRebalanceCb.h"
#include "MantidKernel/Logger.h"

#include <iostream>

namespace {
/// A reference to the static logger
Mantid::Kernel::Logger &LOGGER() {
  static Mantid::Kernel::Logger logger("KafkaRebalanceCb");
  return logger;
}

bool endsWith(std::string const &fullString, std::string const &ending) {
  if (fullString.length() >= ending.length()) {
    return (0 ==
            fullString.compare(fullString.length() - ending.length(),
                               ending.length(), ending));
  } else {
    return false;
  }
}
}

const std::string KafkaRebalanceCb::EVENT_TOPIC_SUFFIX = "_event_topic";
const std::string KafkaRebalanceCb::RUN_TOPIC_SUFFIX = "_run_topic";
const std::string KafkaRebalanceCb::DET_SPEC_TOPIC_SUFFIX = "_det_spec_topic";

void KafkaRebalanceCb::rebalance_cb(
    RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
    std::vector<RdKafka::TopicPartition *> &partitions) {
  LOGGER().debug() << "RebalanceCb: " << RdKafka::err2str(err) << ": ";

  if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {

    for (auto partition : partitions) {
      if (endsWith(partition->topic(), RUN_TOPIC_SUFFIX) ||
          endsWith(partition->topic(), DET_SPEC_TOPIC_SUFFIX)) {
        int64_t lowOffset = 0;
        int64_t highOffset = 0;
        consumer->query_watermark_offsets(partition->topic(),
                                          partition->partition(), &lowOffset,
                                          &highOffset, -1);
        // Current offset-1 means that we will get the last message in the
        // partition
        // This guarantees that we get the required run info and det spec map
        // message, provided it has not been longer than the Kafka retention
        // time since one of those was sent
        partition->set_offset(highOffset - 1);
        LOGGER().debug() << "Setting topic: " << partition->topic()
                         << " , partition: " << partition->partition()
                         << " to offset " << partition->offset() << std::endl;
      }
    }

    consumer->assign(partitions);

  } else {
    consumer->unassign();
  }
}
