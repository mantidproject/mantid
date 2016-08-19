#include "MantidLiveData/ISIS/ISISKafkaEventListener.h"

#include "MantidAPI/LiveListenerFactory.h"

#include <librdkafka/rdkafkacpp.h>

namespace {
/// static logger
Mantid::Kernel::Logger g_log("ISISKafaEventListener");
}

namespace Mantid {
namespace LiveData {

DECLARE_LISTENER(ISISKafkaEventListener)

/// @copydoc ILiveListener::connect
bool ISISKafkaEventListener::connect(const Poco::Net::SocketAddress &address) {
  UNUSED_ARG(address);
  return false;
}

/// @copydoc ILiveListener::start
void ISISKafkaEventListener::start(Kernel::DateAndTime startTime) {
  UNUSED_ARG(startTime);
}

/// @copydoc ILiveListener::extractData
boost::shared_ptr<API::Workspace> ISISKafkaEventListener::extractData() {
  return boost::shared_ptr<API::Workspace>();
}


/// @copydoc ILiveListener::runStatus
API::ILiveListener::RunStatus ISISKafkaEventListener::runStatus() {
  return Running;
}

/// @copydoc ILiveListener::runNumber
int ISISKafkaEventListener::runNumber() const { return -1; }
}
}
