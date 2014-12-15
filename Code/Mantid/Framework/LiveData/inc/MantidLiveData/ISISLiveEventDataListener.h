#ifndef MANTID_LIVEDATA_ISISLIVEEVENTDATALISTENER_H_
#define MANTID_LIVEDATA_ISISLIVEEVENTDATALISTENER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidLiveData/ISIS/TCPEventStreamDefs.h"

#include "MantidAPI/ILiveListener.h"
#include "MantidDataObjects/EventWorkspace.h"

#include "Poco/Net/StreamSocket.h"
#include <Poco/Runnable.h>
#include <Poco/Thread.h>

#include <map>

// Time we'll wait on a receive call (in seconds)
const long RECV_TIMEOUT = 30;
// Sleep time in case we need to wait for the data to become available (in milliseconds)
const long RECV_WAIT = 1;

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
struct idc_info;
typedef struct idc_info* idc_handle_t;

namespace Mantid
{
  namespace LiveData
  {

    /** ILiveListener is the interface implemented by classes which connect directly to
        instrument data acquisition systems (DAS) for retrieval of 'live' data into Mantid.

        Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class ISISLiveEventDataListener: public API::ILiveListener, public Poco::Runnable
    {
    public:

      /// Constructor.
      ISISLiveEventDataListener();
      /// Destructor. Should handle termination of any socket connections.
      virtual ~ISISLiveEventDataListener();

      //----------------------------------------------------------------------
      // Static properties
      //----------------------------------------------------------------------

      /// The name of this listener
      std::string name() const {return "ISISLiveEventDataListener";}
      /// Does this listener support requests for (recent) past data
      virtual bool supportsHistory() const {return false;}
      /// Does this listener buffer events (true) or histogram data (false)
      virtual bool buffersEvents() const {return true;}

      //----------------------------------------------------------------------
      // Actions
      //----------------------------------------------------------------------

      /** Connect to the specified address and start listening/buffering
       *  @param address   The IP address and port to contact
       *  @return True if the connection was successfully established
       */
      bool connect(const Poco::Net::SocketAddress& address);

      /** Commence the collection of data from the DAS. Must be called before extractData().
       *  This method facilitates requesting an historical startpoint. Implementations
       *  that don't support this may simply start collecting data when the connect() method
       *  is called (indeed this may be required by some protocols).
       *  @param startTime The timestamp of the earliest data requested (default: now).
       *                   Ignored if not supported by an implementation.
       *                   The value of 'now' is zero for compatibility with the SNS live stream.
       */
      virtual void start(Kernel::DateAndTime startTime = Kernel::DateAndTime());

      /** Get the data that's been buffered since the last call to this method
       *  (or since start() was called).
       *  This method should never return an empty shared pointer, and a given
       *  instance of a listener should return a workspace of the same dimension every time.
       *  The implementation should reset its internal buffer when this method is called
       *    - the returned workspace is for the caller to do with as they wish.
       *  IF THIS METHOD IS CALLED BEFORE start() THEN THE RESULTS ARE UNDEFINED!!!
       *  @return A pointer to the workspace containing the buffered data.
       *  @throws LiveData::Exception::NotYet If the listenere is not yet ready to
       *    return a workspace. This exception will be caught by LoadLiveData, which
       *    will call extractData() again a short while later. Any other exception
       *    will stop the calling algorithm.
       */
      virtual boost::shared_ptr<API::Workspace> extractData();

      //----------------------------------------------------------------------
      // State flags
      //----------------------------------------------------------------------

      /** Has the connection to the DAS been established?
       *  Could also be used to check for a continued connection.
       */
      bool isConnected();

      /** Gets the current run status of the listened-to data stream
       *  @return A value of the RunStatus enumeration indicating the present status
       */
      virtual ILiveListener::RunStatus runStatus();

      int runNumber() const;

      /** Sets a list of spectra to be extracted. Default is reading all available spectra.
       * @param specList :: A vector with spectra indices.
       */
      virtual void setSpectra(const std::vector<specid_t>& specList) {(void)specList;}

      /// the background thread.  What gets executed when we call POCO::Thread::start()
      virtual void run();

    protected:

      // Initialize the event buffer
      void initEventBuffer(const TCPStreamEventDataSetup& setup);
      // Save received event data in the buffer workspace
      void saveEvents(const std::vector<TCPStreamEventNeutron> &data, const Kernel::DateAndTime &pulseTime, size_t period);
      // Set the spectra-detector map
      void loadSpectraMap();
      // Load the instrument
      void loadInstrument(const std::string& instrName);
      // Get an integer value ising the IDC interface
      int getInt(const std::string& par) const;
      // Get an integer array ising the IDC interface
      void getIntArray(const std::string& par, std::vector<int>& arr, const size_t dim);

      // receive a header and check if it's valid
      template <typename T>
      void Receive(T &buffer, const std::string& head, const std::string &msg)
      {
          long timeout = 0;
          while( m_socket.available() < static_cast<int>(sizeof(buffer)) )
          {
              Poco::Thread::sleep(RECV_WAIT);
              timeout += RECV_WAIT;
              if ( timeout > RECV_TIMEOUT * 1000 ) throw std::runtime_error("Operation of receiving " + head + " timed out.");
          }
          m_socket.receiveBytes(&buffer, sizeof(buffer));
          if ( !buffer.isValid() )
          {
              throw std::runtime_error(msg);
          }
      }

      // receive data that cannot be processed
      template <typename T>
      void CollectJunk(T head) 
      {
        m_socket.receiveBytes(junk_buffer, head.length - static_cast<uint32_t>(sizeof(head)));
      }

      /// The socket communicating with the DAE
      Poco::Net::StreamSocket m_socket;
      /// Keep connection status
      bool m_isConnected;

      /// Thread that reads events from the DAE in the background
      Poco::Thread m_thread;
      /// background thread checks this periodically.  If true, the thread exits
      bool m_stopThread;
      /// Holds on to any exceptions that were thrown in the background thread so that we
      /// can re-throw them in the forground thread
      boost::shared_ptr<std::runtime_error> m_backgroundException;

      /// Used to buffer events between calls to extractData()
      std::vector<DataObjects::EventWorkspace_sptr> m_eventBuffer;
      /// Protects m_eventBuffer
      Poco::FastMutex m_mutex;
      /// Run start time
      Kernel::DateAndTime m_startTime;
      /// Run number
      int m_runNumber;

      /// the DAE handle to use with IDC commands
      idc_handle_t m_daeHandle;

      /// number of periods
      int m_numberOfPeriods;

      /// number of spectra
      int m_numberOfSpectra;

      /// buffer to collect data that cannot be processed
      char junk_buffer[1000];

      /// list of warnings for repeated conditions
      /// If the same condition happens repeatedly the warning is issued once
      /// and is deleted from the list
      std::map<std::string, std::string> m_warnings;

      /// reporter function called when the IDC reading routines raise an error
      static void IDCReporter(int status, int code, const char* message);
    };

  } // namespace LiveData
} // namespace Mantid

#endif /*MANTID_LIVEDATA_ISISLIVEEVENTDATALISTENER_H_*/
