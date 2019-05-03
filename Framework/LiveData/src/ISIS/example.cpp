// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/TCPServer.h"
#include "Poco/Net/TCPServerConnection.h"
#include "Poco/Net/TCPServerConnectionFactory.h"
#include "Poco/Net/TCPServerParams.h"

#include "MantidKernel/Logger.h"
#include "MantidLiveData/ISIS/TCPEventStreamDefs.h"

namespace Mantid {
namespace LiveData {

/// connect to an event mode control progam and read live events
int liveData(const std::string &host) {
  Kernel::Logger g_log = Kernel::Logger("example");
  static char *junk_buffer[10000];
  Poco::Net::StreamSocket s;
  Poco::UInt16 port = 10000;
  Poco::Net::SocketAddress address(host, port);
  s.connect(address);
  TCPStreamEventDataSetup setup;
  while (s.available() < static_cast<int>(sizeof(setup))) {
    Poco::Thread::sleep(1000);
  }
  s.receiveBytes(&setup, sizeof(setup));
  if (!setup.isValid()) {
    throw std::runtime_error("version wrong");
  }
  g_log.information() << "run number " << setup.head_setup.run_number << '\n';
  TCPStreamEventDataNeutron events;
  while (true) {
    while (s.available() < static_cast<int>(sizeof(events.head))) {
      Poco::Thread::sleep(100);
    }
    s.receiveBytes(&events.head, sizeof(events.head));
    if (!events.head.isValid()) {
      throw std::runtime_error("corrupt stream - you should reconnect");
    }
    if (!(events.head.type == TCPStreamEventHeader::Neutron)) {
      throw std::runtime_error("corrupt stream - you should reconnect");
    }
    s.receiveBytes(junk_buffer, events.head.length -
                                    static_cast<uint32_t>(sizeof(events.head)));
    while (s.available() < static_cast<int>(sizeof(events.head_n))) {
      Poco::Thread::sleep(100);
    }
    s.receiveBytes(&events.head_n, sizeof(events.head_n));
    if (!events.head_n.isValid()) {
      throw std::runtime_error("corrupt stream - you should reconnect");
    }
    s.receiveBytes(junk_buffer,
                   events.head_n.length -
                       static_cast<uint32_t>(sizeof(events.head_n)));
    events.data.resize(events.head_n.nevents);
    uint32_t nread = 0;
    while (nread < events.head_n.nevents) {
      uint32_t ntoread = static_cast<uint32_t>(
          s.available() / static_cast<int>(sizeof(TCPStreamEventNeutron)));
      if (ntoread > (events.head_n.nevents - nread)) {
        ntoread = events.head_n.nevents - nread;
      }
      if (ntoread > 0) {
        s.receiveBytes(&(events.data[nread]),
                       ntoread *
                           static_cast<int>(sizeof(TCPStreamEventNeutron)));
        nread += ntoread;
      } else {
        Poco::Thread::sleep(100);
      }
    }
    if (!events.isValid()) {
      throw std::runtime_error("corrupt stream - you should reconnect");
    }
    // TCPStreamEventHeader& head = events.head;
    TCPStreamEventHeaderNeutron &head_n = events.head_n;
    g_log.information() << "Read " << nread << " events for frame number "
                        << head_n.frame_number << " time "
                        << head_n.frame_time_zero << '\n';
    for (int i = 0; i < 10; ++i) {
      g_log.information() << events.data[i].time_of_flight << " "
                          << events.data[i].spectrum << '\n';
    }
  }
  s.close();
  return 0;
}
} // namespace LiveData
} // namespace Mantid
