// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_ILIVELISTENER_H_
#define MANTID_API_ILIVELISTENER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/PropertyManager.h"
#include <Poco/Net/SocketAddress.h>
#include <string>

namespace Mantid {
namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Workspace;

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
  virtual void
  start(Types::Core::DateAndTime startTime = Types::Core::DateAndTime()) = 0;

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
  virtual boost::shared_ptr<Workspace> extractData() = 0;

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
   *  None    : No current run
   *  Begin   : A new run has begun since the last call to extractData
   *  Running : We are in a run
   *  End     : The run has ended since the last call to extractData
   */
  enum RunStatus { NoRun = 0, BeginRun = 1, Running = 2, EndRun = 4 };

  /** Gets the current run status of the listened-to data stream
   *  @return A value of the RunStatus enumeration indicating the present status
   */
  virtual ILiveListener::RunStatus runStatus() = 0;

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
using ILiveListener_sptr = boost::shared_ptr<ILiveListener>;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ILIVELISTENER_H_*/
