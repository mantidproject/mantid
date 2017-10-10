#ifndef MANTID_LIVEDATA_ISISKAFKAEVENTLISTENER_H_
#define MANTID_LIVEDATA_ISISKAFKAEVENTLISTENER_H_
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/LiveListener.h"

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------

namespace Mantid {
namespace LiveData {
class KafkaEventStreamDecoder;

/**
  Implementation of a live listener to consume messages from the Kafka system
  at ISIS. It currently parses the events directly using flatbuffers so will
  need updating if the schema changes.

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
 */
class DLLExport KafkaEventListener : public API::LiveListener {
public:
  KafkaEventListener();
  /// Destructor. Should handle termination of any socket connections.
  ~KafkaEventListener() override = default;

  //----------------------------------------------------------------------
  // Static properties
  //----------------------------------------------------------------------

  /// The name of this listener
  std::string name() const override { return "KafkaEventListener"; }
  /// Does this listener support requests for (recent) past data
  bool supportsHistory() const override { return true; }
  /// Does this listener buffer events (true) or histogram data (false)
  bool buffersEvents() const override { return true; }

  //----------------------------------------------------------------------
  // Actions
  //----------------------------------------------------------------------

  bool connect(const Poco::Net::SocketAddress &address) override;
  void start(
      Types::Core::DateAndTime startTime = Types::Core::DateAndTime()) override;
  boost::shared_ptr<API::Workspace> extractData() override;

  //----------------------------------------------------------------------
  // State flags
  //----------------------------------------------------------------------
  bool isConnected() override;
  ILiveListener::RunStatus runStatus() override;
  int runNumber() const override;

private:
  std::unique_ptr<KafkaEventStreamDecoder> m_decoder = nullptr;
};

} // namespace LiveData
} // namespace Mantid

#endif /*MANTID_LIVEDATA_ISISKAFKAEVENTLISTENER_H_*/
