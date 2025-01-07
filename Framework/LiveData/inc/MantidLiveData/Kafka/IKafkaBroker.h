// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"
#include <memory>
#include <vector>

namespace Mantid {
namespace LiveData {

/**
  Defines the interface used to communicate with a Kafka broker such as
  subscribing to topics.
*/
class MANTID_LIVEDATA_DLL IKafkaBroker {
public:
  virtual ~IKafkaBroker() = default;

  virtual std::unique_ptr<IKafkaStreamSubscriber> subscribe(std::vector<std::string> topics,
                                                            SubscribeAtOption subscribeOption) const = 0;
  virtual std::unique_ptr<IKafkaStreamSubscriber> subscribe(std::vector<std::string> topics, int64_t offset,
                                                            SubscribeAtOption subscribeOption) const = 0;
};

} // namespace LiveData
} // namespace Mantid
