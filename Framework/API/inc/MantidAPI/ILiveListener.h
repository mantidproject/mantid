// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/PropertyManager.h"
#include <Poco/Net/SocketAddress.h>
#include <optional>
#include <string>

namespace Mantid {
namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Workspace;

/** Listener connection / health, independent of the DAS run state.
 *  These values are orthogonal to RunStatus.
 */
enum class ListenerState {
  Disconnected, ///< Not connected to the DAS
  Connected,    ///< Connected and reading
  ReadWait,     ///< Connected but paused at a run boundary (back-pressure)
  Error         ///< Background thread reported an exception
};

/** ILiveListener is the interface implemented by classes which connect directly
   to
    instrument data acquisition systems (DAS) for retrieval of 'live' data into
   Mantid.
 */
class MANTID_API_DLL ILiveListener : public Kernel::PropertyManager {
public:
  //----------------------------------------------------------------------
  // Static properties
  //----------------------------------------------------------------------

  /// The name of this listener
  virtual std::string name() const = 0;
  /// Does this listener support requests for (recent) past data
  virtual bool supportsHistory() const = 0;
  /// Does this listener buffer events (true) or histogram data (false)
  virtual bool buffersEvents() const = 0;

  //----------------------------------------------------------------------
  // Actions
  //----------------------------------------------------------------------

  /** Connect to the specified address and start listening/buffering
   * @param address The IP address and port to contact
   * @return True if the connection was successfully established
   */
  virtual bool connect(const Poco::Net::SocketAddress &address) = 0;

  /** Commence the collection of data from the DAS. Must be called before
   * extractData().
   *  This method facilitates requesting an historical startpoint.
   * Implementations
   *  that don't support this may simply start collecting data when the
   * connect() method
   *  is called (indeed this may be required by some protocols).
   *  @param startTime The timestamp of the earliest data requested (default:
   * now).
   *      Ignored if not supported by an implementation.
   *      The value of 'now' is zero
   *      The value of 'start of run' is 1 second
   *      for compatibility with the SNS live stream and ISIS Kafka live stream.
   */
  virtual void start(Types::Core::DateAndTime startTime = Types::Core::DateAndTime()) = 0;

  /** Get the data that's been buffered since the last call to this method
   *  (or since start() was called).
   *  This method should never return an empty shared pointer, and a given
   *  instance of a listener should return a workspace of the same dimension
   * every time.
   *  The implementation should reset its internal buffer when this method is
   * called
   *    - the returned workspace is for the caller to do with as they wish.
   *  IF THIS METHOD IS CALLED BEFORE start() THEN THE RESULTS ARE UNDEFINED!!!
   *  @return A pointer to the workspace containing the buffered data.
   *  @throws LiveData::Exception::NotYet If the listenere is not yet ready to
   *    return a workspace. This exception will be caught by LoadLiveData, which
   *    will call extractData() again a short while later. Any other exception
   *    will stop the calling algorithm.
   */
  virtual std::shared_ptr<Workspace> extractData() = 0;

  //----------------------------------------------------------------------
  // State information
  //----------------------------------------------------------------------

  /** Has the connection to the DAS been established?
   *  Could also be used to check for a continued connection.
   */
  virtual bool isConnected() = 0;

  /** Indicates that a reset (or period change?) signal has been received from
   * the DAS.
   *  An example is the SNS SMS (!) statistics reset packet.
   *  A concrete listener should discard any buffered events on receipt of such
   * a signal.
   *  It is the client's responsibility to call this method, if necessary, prior
   * to
   *  extracting the data. Calling this method resets the flag.
   */
  virtual bool dataReset() = 0;

  /** The possible run statuses (initial list taken from SNS SMS protocol)
   *  Values are assigned in transition order.
   *  NoRun      : No current run.
   *  JoiningRun : NEW_RUN received but workspace initialisation not yet complete.
   *               The DAS is in a run; the listener is still bootstrapping.
   *               Transitions to Running once init completes (via BeginRun edge).
   *  BeginRun   : A new run has begun since the last call to extractData.
   *  Running    : We are in a run and the workspace is initialised.
   *  EndRun     : The run has ended since the last call to extractData.
   */
  enum RunStatus { NoRun = 0, JoiningRun = 1, BeginRun = 2, Running = 3, EndRun = 4 };

  // ----- Pure state queries (no side effects, all const) -----------------

  /** Current DAS run state (NoRun / JoiningRun / BeginRun / Running / EndRun),
   *  as last reported by the background thread.  This is a pure getter —
   *  calling it must not mutate any internal state.
   *  Default implementation returns NoRun for listeners that have no concept
   *  of run boundaries.  Override to reflect listener-specific state.
   */
  virtual RunStatus runState() const { return NoRun; }

  /** Whether the current run has been paused by a DAS annotation.
   *  Orthogonal to runState(): runState() returns Running whether or not
   *  the run is paused.  Default returns false.
   */
  virtual bool isPaused() const { return false; }

  /** Listener connection / health.
   *  Every concrete listener must override this to reflect actual connection
   *  state.  Returns one of: Disconnected, ReadWait, Connected, Error.
   */
  virtual ListenerState listenerState() const = 0;

  /** The run-state transition (if any) that the most recent extractData() call
   *  consumed.  Cleared only when extractData() commits a *new* transition.
   *  Reports run-state edges (BeginRun / EndRun) only; pause/resume are not
   *  reported here — use isPaused() for those.
   *  Listeners that have no transition concept always return nullopt.
   */
  virtual std::optional<RunStatus> lastTransition() const { return std::nullopt; }

  /** Gets the current run status of the listened-to data stream.
   *  @return A value of the RunStatus enumeration indicating the present status
   *  @deprecated Use runState() / lastTransition() and call extractData() to
   *    commit pending transitions.  This method will be removed in a future
   *    release.
   */
  [[deprecated("Use runState() / lastTransition() and call extractData() "
               "to commit pending transitions.")]]
  virtual RunStatus runStatus();

  /// Returns the run number of the current run
  virtual int runNumber() const = 0;

  /** Sets a list of spectra to be extracted.
   * @param specList :: A vector with spectra indices.
   */
  virtual void setSpectra(const std::vector<specnum_t> &specList) = 0;

  /** Allow listener to see calling algorithm
   * @param callingAlgorithm : const ref to calling algorithm
   */
  virtual void setAlgorithm(const class IAlgorithm &callingAlgorithm) = 0;
};

/// Shared pointer to an ILiveListener
using ILiveListener_sptr = std::shared_ptr<ILiveListener>;

} // namespace API
} // namespace Mantid
