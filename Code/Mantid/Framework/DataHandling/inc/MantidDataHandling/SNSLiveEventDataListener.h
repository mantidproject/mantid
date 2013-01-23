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
      boost::shared_ptr<API::Workspace> extractData();

      ILiveListener::RunStatus runStatus();
      // Note: runStatus() might actually update the value of m_status, so
      // it probably shouldn't be called by other member functions.  The
      // logic it uses for updating m_status is only valid if the function
      // is only called by the MonitorLiveData algorithm.

      bool isConnected();

      virtual void run();  // the background thread.  What gets executed when we call
                           // POCO::Thread::start()
    protected:
      using ADARA::Parser::rxPacket;
      //virtual bool rxPacket( const ADARA::Packet &pkt);
      //virtual bool rxPacket( const ADARA::RawDataPkt &pkt);
      virtual bool rxPacket( const ADARA::RTDLPkt &pkt);
      virtual bool rxPacket( const ADARA::BankedEventPkt &pkt);
      virtual bool rxPacket( const ADARA::HeartbeatPkt &pkt);
      virtual bool rxPacket( const ADARA::GeometryPkt &pkt);
      virtual bool rxPacket( const ADARA::BeamlineInfoPkt &pkt);
      virtual bool rxPacket( const ADARA::RunStatusPkt &pkt);
      virtual bool rxPacket( const ADARA::VariableU32Pkt &pkt);
      virtual bool rxPacket( const ADARA::VariableDoublePkt &pkt);
      virtual bool rxPacket( const ADARA::VariableStringPkt &pkt);
      virtual bool rxPacket( const ADARA::DeviceDescriptorPkt &pkt);
      virtual bool rxPacket( const ADARA::AnnotationPkt &pkt);
      //virtual bool rxPacket( const ADARA::RunInfoPkt &pkt);

    private:
     
      // Workspace initialization needs to happen in 2 steps.  Part 1 must happen
      // before we receive *any* packets.
      void initWorkspacePart1();

      // We need data from both the geometry packet and the run status packet in
      // order to run the second part of the initialization.  Since I don't know what
      // order the packets will arrive in, I've put the necessary code in this
      // function.  Both rxPacket() functions will check to see if all the data is
      // available and call this function if it is.
      void initWorkspacePart2();

      // Check to see if all the conditions we need for initWorkspacePart2() have been
      // met.  Making this a function because it's starting to get a little complicated
      // and I didn't want to be repeating the same tests in several places...
      bool readyForInitPart2()
      {
        if (m_instrumentXML.size() == 0)  return false;
        if (m_instrumentName.size() == 0) return false;
        if (m_dataStartTime == Kernel::DateAndTime())  return false;

        return true;
      }

      void appendEvent( uint32_t pixelId, double tof, const Mantid::Kernel::DateAndTime pulseTime);
      // tof is "Time Of Flight" and is in units of microsecondss relative to the start of the pulse
      // (There's some documentation that says nanoseconds, but Russell Taylor assures me it's really
      // is microseconds!)
      // pulseTime is the start of the pulse relative to Jan 1, 1990.
      // Both values are designed to be passed straight into the TofEvent constructor.

      ILiveListener::RunStatus m_status;
      DataObjects::EventWorkspace_sptr m_eventBuffer; ///< Used to buffer events between calls to extractData()

      bool m_workspaceInitialized;
      std::string m_wsName;
      detid2index_map * m_indexMap;  // maps pixel id's to workspace indexes

      // We need these 2 strings to initialize m_buffer
      std::string m_instrumentName;
      std::string m_instrumentXML;

      uint64_t m_rtdlPulseId;  // We get this from the RTDL packe  

      Poco::Net::StreamSocket m_socket;
      //int m_sockfd;  // socket file descriptor
      bool m_isConnected;

      Poco::Thread m_thread;
      Poco::FastMutex m_mutex;  // protects m_buffer & m_status
      bool m_pauseNetRead;
      bool m_stopThread;  // background thread checks this periodically.  If true, the
                          // thread exits

      Kernel::DateAndTime m_startTime;  // The requested start time for the data stream
                                        // (needed by the run() function)
      Kernel::DateAndTime m_heartbeat;  // The time when we received the last ClientHello
                                        // packet.  SMS is supposed to send these out
                                        // periodicaly.  If we don't get them, there's a
                                        // problem somewhere.

      // Used to initialize the a few properties (run_start and scan_index) if we haven't received
      // the packets with the 'real' values by the time we call initWorkspacePart2.  (We can't
      // delay the call to initWorkspacePart2 because we might never receive 'real' values for
      // those properties.
      Kernel::DateAndTime m_dataStartTime;

      // These 2 determine whether or not we filter out events that arrive when
      // the run is paused.
      bool m_runPaused; // Set to true or false when we receive a pause/resume marker in an
                        // annotation packet. (See rxPacket( const ADARA::AnnotationPkt &pkt))
      int m_keepPausedEvents; // Set from a configuration property. (Should be a bool, but
                              // appearantly, we can't read bools from the config file?!?)


      // --- Data structures necessary for handling all the process variable info ---

      // maps <device id, variable id> to variable name
      // (variable names are unique, so we don't need to worry about device names.)
      typedef std::map <std::pair <unsigned, unsigned>, std::string> NameMapType;
      NameMapType m_nameMap;



      // ----------------------------------------------------------------------------

      static Kernel::Logger& g_log;   ///< reference to the logger class
     
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_FAKEEVENTDATALISTENER_H_ */
