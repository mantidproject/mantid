#ifndef MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODER_H_
#define MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODER_H_

#include "MantidLiveData/Kafka/IKafkaBroker.h"
#include "MantidLiveData/Kafka/IKafkaStreamSubscriber.h"
#include "MantidDataObjects/EventWorkspace.h"

#include <atomic>
#include <thread>

namespace Mantid {
namespace LiveData {

/**
  High-level interface to ISIS Kafka event system. It requires
  3 topic names of the data streams.

  A call to capture() starts the process of capturing the stream on a separate
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
class DLLExport ISISKafkaEventStreamDecoder {
public:
  ISISKafkaEventStreamDecoder(const IKafkaBroker &broker,
                              std::string eventTopic, std::string runInfoTopic,
                              std::string spDetTopic);
  ~ISISKafkaEventStreamDecoder();
  ISISKafkaEventStreamDecoder(const ISISKafkaEventStreamDecoder &) = delete;
  ISISKafkaEventStreamDecoder &
  operator=(const ISISKafkaEventStreamDecoder &) = delete;

public:
  ///@name Start/stop
  ///@{
  void startCapture() noexcept;
  void stopCapture() noexcept;
  ///@}

  ///@name Querying
  ///@{
  bool isRunning() const noexcept { return m_capturing; }
  boost::shared_ptr<std::runtime_error> exceptionHandled() const noexcept {
    return m_exception;
  }
  ///@}

private:
  void captureImpl() noexcept;
  void captureImplExcept();

  void initLocalEventBuffer();

  /// Flag indicating if user interruption has been requested
  std::atomic<bool> m_interrupt;
  /// Subscriber for the event stream
  std::unique_ptr<IKafkaStreamSubscriber> m_eventStream;
  /// Local event workspace buffer
  DataObjects::EventWorkspace_sptr m_localEvents;
  /// Subscriber for the run info stream
  std::unique_ptr<IKafkaStreamSubscriber> m_runStream;
  /// Subscriber for the run info stream
  std::unique_ptr<IKafkaStreamSubscriber> m_spDetStream;

  /// Associated thread running the capture process
  std::thread m_thread;
  /// Flag indicating that the decoder is capturing
  std::atomic<bool> m_capturing;
  /// Exception object indicating there was an error
  boost::shared_ptr<std::runtime_error> m_exception;
};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODER_H_ */
