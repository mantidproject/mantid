#ifndef MANTID_LIVEDATA_IKAFKABROKER_H_
#define MANTID_LIVEDATA_IKAFKABROKER_H_

#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"
#include <memory>

namespace Mantid {
namespace LiveData {

/**
  Defines the interface used to communicate with a Kafka broker such as
  subscribing to topics.

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
class DLLExport IKafkaBroker {
public:
  ~IKafkaBroker() = default;

  virtual std::unique_ptr<IKafkaStreamSubscriber>
  subscribe(const std::string &topic) const = 0;
  virtual std::unique_ptr<IKafkaStreamSubscriber>
  subscribe(const std::string &topic, int64_t offset) const = 0;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_IKAFKABROKER_H_ */
