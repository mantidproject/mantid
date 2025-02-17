// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/LiveListener.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidLiveData/ADARA/ADARAParser.h"

#include <Poco/Net/StreamSocket.h>
#include <Poco/Runnable.h>
#include <Poco/Timer.h>
#include <set>

namespace Mantid {
namespace LiveData {

/** An implementation of ILiveListener for use at SNS.  Connects to the Stream
   Management
    Service and receives events from it.
 */
class SNSLiveEventDataListener : public API::LiveListener, public Poco::Runnable, public ADARA::Parser {
public:
  SNSLiveEventDataListener();
  ~SNSLiveEventDataListener() override;

  std::string name() const override { return "SNSLiveEventDataListener"; }
  bool supportsHistory() const override { return true; }
  bool buffersEvents() const override { return true; }

  bool connect(const Poco::Net::SocketAddress &address) override;
  void start(const Types::Core::DateAndTime startTime = Types::Core::DateAndTime()) override;
  std::shared_ptr<API::Workspace> extractData() override;

  ILiveListener::RunStatus runStatus() override;
  // Note: runStatus() might actually update the value of m_status, so
  // it probably shouldn't be called by other member functions.  The
  // logic it uses for updating m_status is only valid if the function
  // is only called by the MonitorLiveData algorithm.

  int runNumber() const override { return m_runNumber; };

  bool isConnected() override;

  void run() override; // the background thread.  What gets executed when we
                       // call POCO::Thread::start()
protected:
  using ADARA::Parser::rxPacket;
  // virtual bool rxPacket( const ADARA::Packet &pkt);
  // virtual bool rxPacket( const ADARA::RawDataPkt &pkt);
  bool rxPacket(const ADARA::BankedEventPkt &pkt) override;
  bool rxPacket(const ADARA::BeamMonitorPkt &pkt) override;
  bool rxPacket(const ADARA::GeometryPkt &pkt) override;
  bool rxPacket(const ADARA::BeamlineInfoPkt &pkt) override;
  bool rxPacket(const ADARA::RunStatusPkt &pkt) override;
  bool rxPacket(const ADARA::VariableU32Pkt &pkt) override;
  bool rxPacket(const ADARA::VariableDoublePkt &pkt) override;
  bool rxPacket(const ADARA::VariableStringPkt &pkt) override;
  bool rxPacket(const ADARA::DeviceDescriptorPkt &pkt) override;
  bool rxPacket(const ADARA::AnnotationPkt &pkt) override;
  bool rxPacket(const ADARA::RunInfoPkt &pkt) override;

private:
  // Workspace initialization needs to happen in 2 steps.  Part 1 must happen
  // before we receive *any* packets.
  void initWorkspacePart1();

  // We need data from both the geometry packet and the run status packet in
  // order to run the second part of the initialization.  Since I don't know
  // what order the packets will arrive in, I've put the necessary code in
  // this function.  Both rxPacket() functions will check to see if all the
  // data is available and call this function if it is.
  void initWorkspacePart2();

  void initMonitorWorkspace();

  // Check to see if all the conditions we need for initWorkspacePart2() have
  // been met.  Making this a function because it's starting to get a little
  // complicated and I didn't want to be repeating the same tests in several
  // places...
  bool readyForInitPart2() {
    if (m_instrumentXML.empty())
      return false;
    if (m_instrumentName.empty())
      return false;
    if (m_dataStartTime == Types::Core::DateAndTime())
      return false;

    return haveRequiredLogs();
  }

  // Returns true if we've got a value for every log listed in m_requiredLogs
  bool haveRequiredLogs();

  void appendEvent(const uint32_t pixelId, const double tof, const Mantid::Types::Core::DateAndTime pulseTime);
  // tof is "Time Of Flight" and is in units of microsecondss relative to the
  // start of the pulse
  // (There's some documentation that says nanoseconds, but Russell Taylor
  // assures me it's really is microseconds!)
  // pulseTime is the start of the pulse relative to Jan 1, 1990.
  // Both values are designed to be passed straight into the TofEvent
  // constructor.

  ILiveListener::RunStatus m_status{RunStatus::NoRun};
  int m_runNumber{0};
  DataObjects::EventWorkspace_sptr m_eventBuffer;
  ///< Used to buffer events between calls to extractData()

  bool m_workspaceInitialized{false};
  std::string m_wsName;
  detid2index_map m_indexMap;        // maps pixel id's to workspace indexes
  detid2index_map m_monitorIndexMap; // Same as above for the monitor workspace

  // We need these 2 strings to initialize m_buffer
  std::string m_instrumentName;
  std::string m_instrumentXML;

  // Names of log values that we need before we can initialize m_buffer.
  // We get the names by parsing m_instrumentXML;
  std::vector<std::string> m_requiredLogs;

  // Names of any monitor logs (these must be manually removed during the call
  // to extractData())
  std::vector<std::string> m_monitorLogs;

  Poco::Net::StreamSocket m_socket;
  bool m_isConnected{false};

  Poco::Thread m_thread;
  std::mutex m_mutex; // protects m_buffer & m_status
  bool m_pauseNetRead{false};
  bool m_stopThread{false}; // background thread checks this periodically.
                            // If true, the thread exits

  Types::Core::DateAndTime m_startTime; // The requested start time for the data
                                        // stream (needed by the run() function)

  // Used to initialize the scan_index property if we haven't received a packet
  // with the 'real' value by the time we call initWorkspacePart2.  (We can't
  // delay the call to initWorkspacePart2 because we might never receive a
  // 'real' value for that property.
  Types::Core::DateAndTime m_dataStartTime;

  // These 2 determine whether or not we filter out events that arrive when
  // the run is paused.
  bool m_runPaused{false};        // Set to true or false when we receive a
                                  // pause/resume marker in an annotation packet. (See
                                  // rxPacket( const ADARA::AnnotationPkt &pkt))
  bool m_keepPausedEvents{false}; // Set from a configuration property

  // Holds on to any exceptions that were thrown in the background thread so
  // that we can re-throw them in the forground thread
  std::shared_ptr<std::runtime_error> m_backgroundException;

  // --- Data structures necessary for handling all the process variable info
  // ---

  // maps <device id, variable id> to variable name
  // (variable names are unique, so we don't need to worry about device names.)
  using NameMapType = std::map<std::pair<unsigned int, unsigned int>, std::string>;
  NameMapType m_nameMap;

  // ---------------------------------------------------------------------------

  // In cases where we're replaying historical data from the SMS, we're likely
  // to get multiple value packets for various values, but we only want to
  // process the most recent one.  Unfortunately, the only way to do this is to
  // hold the packets in a cache until the SMS works its way through the older
  // data and starts sending out the data we actual want.  At that point, we
  // need to parse whatever variable value packets we have in order to set the
  // state of the system properly.

  // Maps the device ID / variable ID pair to the actual packet.  Using a map
  // means we will only keep one packet (the most recent one) for each variable
  using VariableMapType = std::map<std::pair<unsigned int, unsigned int>, std::shared_ptr<ADARA::Packet>>;
  VariableMapType m_variableMap;

  // Process all the variable value packets stored in m_variableMap
  void replayVariableCache();

  // ---------------------------------------------------------------------------
  bool m_ignorePackets{false}; // used by filterPacket() below...
  bool m_filterUntilRunStart{false};

  // Called by the rxPacket() functions to determine if the packet should be
  // processed. (Depending on when it last indexed its data, SMS might send us
  // packets that are older than we requested.)
  // Returns false if the packet should be processed, true if is should be
  // ignored
  bool ignorePacket(const ADARA::PacketHeader &hdr, const ADARA::RunStatus::Enum status = ADARA::RunStatus::NO_RUN);
  void setRunDetails(const ADARA::RunStatusPkt &pkt);

  // We have to defer calling setRunDetails() at the start of a run until the
  // foreground thread has called extractData() and retrieved the last data
  // from the previous state (which was probably NO_RUN).
  // This holds a copy of the RunStatusPkt until we can call setRunDetails().
  std::shared_ptr<ADARA::RunStatusPkt> m_deferredRunDetailsPkt;

  /// list of monitors that were seen on the stream but are not in the IDF
  std::set<detid_t> m_badMonitors;
};

} // namespace LiveData
} // namespace Mantid
