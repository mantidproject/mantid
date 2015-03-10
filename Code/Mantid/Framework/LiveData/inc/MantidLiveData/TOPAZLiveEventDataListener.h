#ifndef MANTID_LIVEDATA_TOPAZLIVEEVENTDATALISTENER_H_
#define MANTID_LIVEDATA_TOPAZLIVEEVENTDATALISTENER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ILiveListener.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/MultiThreaded.h"

#include <Poco/Timer.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Runnable.h>

namespace Mantid {
namespace LiveData {

/** An implementation of ILiveListener for use on the TOPAZ beamline at SNS.
    Connects to the old DAS system and receives events from it.

    Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
    National Laboratory

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
class TOPAZLiveEventDataListener : public API::ILiveListener,
                                   public Poco::Runnable {
public:
    TOPAZLiveEventDataListener();
    virtual ~TOPAZLiveEventDataListener();

    std::string name() const { return "TOPAZLiveEventDataListener"; }
    bool supportsHistory() const { return false; }
    bool buffersEvents() const { return true; }

    bool connect(const Poco::Net::SocketAddress &address);
    void start(Kernel::DateAndTime startTime = Kernel::DateAndTime());
    boost::shared_ptr<API::Workspace> extractData();

    ILiveListener::RunStatus runStatus();
    // Called by the MonitorLiveData algorithm.


    int runNumber() const { return m_runNumber; };

    bool isConnected();

    virtual void run(); // the background thread.  What gets executed when we
                        // call POCO::Thread::start()
protected:

private:

    void initWorkspace();
    void initMonitorWorkspace();

    void appendEvent(uint32_t pixelId, double tof,
                     const Mantid::Kernel::DateAndTime pulseTime);
    // tof is "Time Of Flight" and is in units of microsecondss relative to the
    // start of the pulse.  (There's some documentation that says nanoseconds,
    // but Russell Taylor assures me it's really in microseconds!)
    // pulseTime is the start of the pulse relative to Jan 1, 1990.
    // Both values are designed to be passed straight into the TofEvent
    // constructor.

    ILiveListener::RunStatus m_status;
    bool m_workspaceInitialized;
    
    DataObjects::EventWorkspace_sptr
        m_eventBuffer; ///< Used to buffer events between calls to extractData()
        
    // Names of any monitor logs (these must be manually removed during the call
    // to extractData())
    std::vector<std::string> m_monitorLogs;

    std::string m_wsName;
    detid2index_map m_indexMap;        // maps pixel id's to workspace indexes
    detid2index_map m_monitorIndexMap; // Same as above for the monitor workspace
    
    // We need these 2 strings to initialize m_buffer
    //std::string m_instrumentName;
    //std::string m_instrumentXML;

    Poco::Net::StreamSocket   m_tcpSocket;   // used for initial connection to event_catcher
    Poco::Net::DatagramSocket m_dataSocket;  // used to receive actual event data
    Poco::Net::SocketAddress  m_dataAddr;
    bool m_isConnected;
    
    // Buffer to hold the UDP data pakcets
    unsigned char *m_udpBuf;
    unsigned int m_udpBufSize;

    int m_runNumber;

    Poco::FastMutex m_mutex; // protects m_eventBuffer & m_status
    Poco::Thread m_thread;
    bool m_stopThread; // background thread checks this periodically.  
                        // If true, the thread exits

    // Holds on to any exceptions that were thrown in the background thread so
    // that we can re-throw them in the forground thread
    boost::shared_ptr<std::runtime_error> m_backgroundException;

};

} // namespace LiveData
} // namespace Mantid

#endif /* MANTID_LIVEDATA_TOPAZLIVEEVENTDATALISTENER_H_ */
