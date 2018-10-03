// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_KAFKABROKER_H_
#define MANTID_LIVEDATA_KAFKABROKER_H_

#include "MantidLiveData/Kafka/IKafkaBroker.h"

namespace Mantid {
namespace LiveData {

/**
  Wraps communication with a Kafka broker at a given address.
*/
class DLLExport KafkaBroker : public IKafkaBroker {
public:
  explicit KafkaBroker(std::string address);

  std::unique_ptr<IKafkaStreamSubscriber>
  subscribe(std::vector<std::string> topics,
            SubscribeAtOption subscribeOption) const override;
  std::unique_ptr<IKafkaStreamSubscriber>
  subscribe(std::vector<std::string> topics, int64_t offset,
            SubscribeAtOption subscribeOption) const override;

private:
  std::string m_address;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_KAFKABROKER_H_ */
