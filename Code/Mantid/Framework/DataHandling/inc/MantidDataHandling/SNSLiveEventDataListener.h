#ifndef MANTID_DATAHANDLING_SNSLIVEEVENTDATALISTENER_H_
#define MANTID_DATAHANDLING_SNSLIVEEVENTDATALISTENER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/ADARAParser.h"
#include "MantidAPI/ILiveListener.h"
#include "MantidDataObjects/EventWorkspace.h"
//#include "MantidKernel/RandomNumberGenerator.h"
#include "MantidKernel/MultiThreaded.h"

#include <Poco/Timer.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Runnable.h>

namespace Mantid
{
  namespace DataHandling
  {

    /** An implementation of ILiveListener for use at SNS.  Connects to the Stream Management
        Service and receives events from it.

        Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
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
    class SNSLiveEventDataListener : public API::ILiveListener,
                                     public Poco::Runnable,
                                     public ADARA::Parser
    {
    public:
      SNSLiveEventDataListener();
      virtual ~SNSLiveEventDataListener();

      std::string name() const { return "SNSLiveEventDataListener"; }
      bool supportsHistory() const { return false; } // For the time being at least
      bool buffersEvents() const { return true; }

      bool connect(const Poco::Net::SocketAddress& address);
      void start(Kernel::DateAndTime startTime = Kernel::DateAndTime());
      boost::shared_ptr<API::MatrixWorkspace> extractData();

      ILiveListener::RunStatus runStatus();
      bool isConnected();

      virtual void run();  // the background thread.  What gets executed when we call
                           // POCO::Thread::start()
    protected:
      using ADARA::Parser::rxPacket;
      //virtual bool rxPacket( const ADARA::Packet &pkt);
      //virtual bool rxPacket( const ADARA::RawDataPkt &pkt);
      virtual bool rxPacket( const ADARA::RTDLPkt &pkt);
      virtual bool rxPacket( const ADARA::BankedEventPkt &pkt);
      virtual bool rxPacket( const ADARA::ClientHelloPkt &pkt);
      virtual bool rxPacket( const ADARA::GeometryPkt &pkt);
      virtual bool rxPacket( const ADARA::BeamlineInfoPkt &pkt);
      //virtual bool rxPacket( const ADARA::RunInfoPkt &pkt);

    private:
     
      // We need data from both the geometry packet and the run status packet in
      // order to initialize the workspace.  Since I don't know what order the
      // packets will arrive in, I've put the necessary code in this function.
      // Both rxPacket() functions will check to see if all the data is available
      // and call this function if it is.
      void initWorkspace();

      void appendEvent( uint32_t pixelId, double tof, const Mantid::Kernel::DateAndTime pulseTime);
      // tof is "Time Of Flight" and is in units of nanoseconds relative to the start of the pulse
      // pulseTime is the start of the pulse relative to Jan 1, 1990.
      // Both values are designed to be passed straight into the TofEvent constructor.
      
      DataObjects::EventWorkspace_sptr m_buffer; ///< Used to buffer events between calls to extractData()
      bool m_workspaceInitialized;
      std::string m_wsName;
      detid2index_map * m_indexMap;  // maps pixel id's to workspace indexes

      // We need these 2 strings to initialize m_buffer
      std::string m_instrumentName;
      std::string m_instrumentXML;

      struct timespec m_rtdlPulseTime;  // We get these two from the RTDL packet
      bool  m_rtdlRawFlag;

      Poco::Net::StreamSocket m_socket;
      //int m_sockfd;  // socket file descriptor
      bool m_isConnected;

      Poco::Thread m_thread;
      Poco::FastMutex m_mutex;
      bool m_stopThread;  // background thread checks this periodically.  If true, the
                          // thread exits

      Kernel::DateAndTime m_startTime;  // The requested start time for the data stream
                                        // (needed by the run() function)
      Kernel::DateAndTime m_heartbeat;  // The time when we received the last ClientHello
                                        // packet.  SMS is supposed to send these out
                                        // periodicaly.  If we don't get them, there's a
                                        // problem somewhere.
     
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_FAKEEVENTDATALISTENER_H_ */
