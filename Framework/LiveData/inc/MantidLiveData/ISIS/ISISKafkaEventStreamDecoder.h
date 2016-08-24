#ifndef MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODER_H_
#define MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODER_H_

#include "MantidLiveData/Kafka/IKafkaBroker.h"
#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"

#include <Poco/Runnable.h>

namespace Mantid {
namespace LiveData {

/**
  High-level interface to ISIS Kafka event system. It requires
  3 topic names of the data streams.

  It subclasses Poco::Runnable in order to be able to be run from a separate
  thread.

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
class DLLExport ISISKafkaEventStreamDecoder : public Poco::Runnable {
public:
  ISISKafkaEventStreamDecoder(const IKafkaBroker &broker,
                              std::string eventTopic, std::string runInfoTopic,
                              std::string spDetTopic);
  void run() override;

private:
  std::unique_ptr<IKafkaStreamSubscriber> m_eventStream;
  std::unique_ptr<IKafkaStreamSubscriber> m_runInfoStream;
  std::unique_ptr<IKafkaStreamSubscriber> m_spDetStream;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODER_H_ */
