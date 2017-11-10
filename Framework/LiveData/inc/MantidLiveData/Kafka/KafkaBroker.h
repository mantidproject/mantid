#ifndef MANTID_LIVEDATA_KAFKABROKER_H_
#define MANTID_LIVEDATA_KAFKABROKER_H_

#include "MantidLiveData/Kafka/IKafkaBroker.h"

namespace Mantid {
namespace LiveData {

/**
  Wraps communication with a Kafka broker at a given address.

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
class DLLExport KafkaBroker : public IKafkaBroker {
public:
  KafkaBroker(std::string address);

  std::unique_ptr<IKafkaStreamSubscriber>
  subscribe(const std::string &topic) const override;
  std::unique_ptr<IKafkaStreamSubscriber>
  subscribe(const std::string &topic, int64_t offset) const override;

private:
  std::string m_address;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_KAFKABROKER_H_ */
