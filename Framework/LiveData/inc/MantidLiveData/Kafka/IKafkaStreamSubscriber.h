#ifndef MANTID_LIVEDATA_IKAFKASTREAMSUBSCRIBER_H_
#define MANTID_LIVEDATA_IKAFKASTREAMSUBSCRIBER_H_

#include "MantidKernel/System.h"
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
class DLLExport IKafkaStreamSubscriber {
public:
  virtual ~IKafkaStreamSubscriber() = default;
  virtual void subscribe() = 0;
  virtual void subscribe(int64_t offset) = 0;
  virtual void consumeMessage(std::string *message, int64_t &offset,
                              int32_t &partition, std::string &topic) = 0;
  virtual std::unordered_map<std::string, std::vector<int64_t>>
  getOffsetsForTimestamp(int64_t timestamp) = 0;
  virtual void seek(const std::string &topic, uint32_t partition,
                    int64_t offset) = 0;
  virtual std::unordered_map<std::string, std::vector<int64_t>>
  getCurrentOffsets() = 0;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_IKAFKASTREAMSUBSCRIBER_H_ */
