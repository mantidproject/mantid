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
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/MersenneTwister.h"
#include <Poco/Timer.h>
#include <mutex>

namespace Mantid {
namespace LiveData {
/** An implementation of ILiveListener for testing purposes that fills its event
    workspace buffer with randomly generated events.
 */
class FakeEventDataListener : public API::LiveListener {
public:
  FakeEventDataListener();
  ~FakeEventDataListener() override;

  std::string name() const override { return "FakeEventDataListener"; }
  bool supportsHistory() const override { return false; } // For the time being at least
  bool buffersEvents() const override { return true; }

  bool connect(const Poco::Net::SocketAddress &address) override;
  void start(Types::Core::DateAndTime startTime = Types::Core::DateAndTime()) override;
  std::shared_ptr<API::Workspace> extractData() override;

  bool isConnected() override;
  ILiveListener::RunStatus runStatus() override;
  int runNumber() const override;

private:
  void generateEvents(Poco::Timer &);

  DataObjects::EventWorkspace_sptr m_buffer;       ///< Used to buffer events between calls to extractData()
  std::unique_ptr<Kernel::MersenneTwister> m_rand; ///< Used in generation of random events
  Poco::Timer m_timer;                             ///< Used to call the event-generating function on a schedule
  int m_datarate;                                  ///< The data rate to (attempt to) generate in events/sec
  int m_callbackloop;                              ///< Number of times to loop within each generateEvents()
  /// call
  double m_endRunEvery; ///< Make a new run every N seconds
  int m_notyettimes;    ///< Number of calls to extractData for which to throw a
  /// NotYet exception
  int m_numExtractDataCalls; ///< Number of times extractData has been called

  /// Date and time of the next time to end the run
  Mantid::Types::Core::DateAndTime m_nextEndRunTime;

  /// Fake run number to give
  int m_runNumber;

  /// Mutex to exclude generateEvents() and extractData().
  std::mutex m_mutex;
};

} // namespace LiveData
} // namespace Mantid
