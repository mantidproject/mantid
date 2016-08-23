#ifndef MANTID_LIVEDATA_KAFKAEVENTSUBSCRIBER_H_
#define MANTID_LIVEDATA_KAFKAEVENTSUBSCRIBER_H_

#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"
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
class DLLExport KafkaTopicSubscriber : public IKafkaStreamSubscriber {
public:
  KafkaTopicSubscriber(std::string broker, std::string topic);
  ~KafkaTopicSubscriber();

  const std::string topic() const;

  virtual void subscribe() override;
  virtual bool consumeMessage(std::string *payload) override;

private:
  std::unique_ptr<RdKafka::KafkaConsumer> m_consumer;
  std::string m_brokerAddr;
  std::string m_topicName;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_KAFKAEVENTSUBSCRIBER_H_ */
