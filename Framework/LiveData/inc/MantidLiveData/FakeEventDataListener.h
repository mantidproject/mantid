#ifndef MANTID_LIVEDATA_FAKEEVENTDATALISTENER_H_
#define MANTID_LIVEDATA_FAKEEVENTDATALISTENER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/LiveListener.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include <Poco/Timer.h>
#include <mutex>

namespace Mantid {
namespace LiveData {
/** An implementation of ILiveListener for testing purposes that fills its event
    workspace buffer with randomly generated events.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class FakeEventDataListener : public API::LiveListener {
public:
  FakeEventDataListener();
  ~FakeEventDataListener() override;

  std::string name() const override { return "FakeEventDataListener"; }
  bool supportsHistory() const override {
    return false;
  } // For the time being at least
  bool buffersEvents() const override { return true; }

  bool connect(const Poco::Net::SocketAddress &address) override;
  void start(
      Types::Core::DateAndTime startTime = Types::Core::DateAndTime()) override;
  boost::shared_ptr<API::Workspace> extractData() override;

  bool isConnected() override;
  ILiveListener::RunStatus runStatus() override;
  int runNumber() const override;

private:
  void generateEvents(Poco::Timer &);

  DataObjects::EventWorkspace_sptr
      m_buffer; ///< Used to buffer events between calls to extractData()
  Kernel::PseudoRandomNumberGenerator
      *m_rand; ///< Used in generation of random events
  Poco::Timer
      m_timer;    ///< Used to call the event-generating function on a schedule
  int m_datarate; ///< The data rate to (attempt to) generate in events/sec
  int m_callbackloop; ///< Number of times to loop within each generateEvents()
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

#endif /* MANTID_LIVEDATA_FAKEEVENTDATALISTENER_H_ */
