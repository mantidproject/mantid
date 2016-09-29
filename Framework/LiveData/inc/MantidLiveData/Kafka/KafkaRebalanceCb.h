#ifndef MANTID_KAFKAREBALANCECB_H
#define MANTID_KAFKAREBALANCECB_H

#include <librdkafka/rdkafkacpp.h>

class KafkaRebalanceCb : public RdKafka::RebalanceCb {

public:
  void
  rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
               std::vector<RdKafka::TopicPartition *> &partitions) override;
};

#endif // MANTID_KAFKAREBALANCECB_H
