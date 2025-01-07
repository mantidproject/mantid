// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidLiveData/DllConfig.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Mantid {
namespace LiveData {

enum class SubscribeAtOption {
  OFFSET,  // Specify an offset to join at when calling subscribe(), topic must
           // have a single partition
  LATEST,  // Get only messages which the broker receives after the consumer
           // subscribes
  LASTONE, // Get the last message, topic must have a single partition
  LASTTWO, // Get the last two messages, topic must have a single partition
  TIME
};

/**
  Interface for classes that subscribe to Kafka streams.
*/
class MANTID_LIVEDATA_DLL IKafkaStreamSubscriber {
public:
  virtual ~IKafkaStreamSubscriber() = default;
  virtual void subscribe() = 0;
  virtual void subscribe(int64_t offset) = 0;
  virtual void consumeMessage(std::string *message, int64_t &offset, int32_t &partition, std::string &topic) = 0;
  virtual std::unordered_map<std::string, std::vector<int64_t>> getOffsetsForTimestamp(int64_t timestamp) = 0;
  virtual void seek(const std::string &topic, uint32_t partition, int64_t offset) = 0;
  virtual std::unordered_map<std::string, std::vector<int64_t>> getCurrentOffsets() = 0;
};

} // namespace LiveData
} // namespace Mantid
