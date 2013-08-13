#ifndef MANTID_LIVEDATA_TESTDATALISTENER_H_
#define MANTID_LIVEDATA_TESTDATALISTENER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ILiveListener.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"

namespace Mantid
{
  namespace LiveData
  {
    /** An implementation of ILiveListener for testing purposes that gives back a buffer
        with an identical number of events every time extractData is called.
     */
    class TestDataListener : public API::ILiveListener
    {
    public:
      TestDataListener();
      ~TestDataListener();

      std::string name() const { return "TestDataListener"; }
      bool supportsHistory() const { return false; }
      bool buffersEvents() const { return true; }

      bool connect(const Poco::Net::SocketAddress& address);
      void start(Kernel::DateAndTime startTime = Kernel::DateAndTime());
      boost::shared_ptr<API::Workspace> extractData();

      bool isConnected();
      ILiveListener::RunStatus runStatus();

    private:
      DataObjects::EventWorkspace_sptr m_buffer;
      Kernel::PseudoRandomNumberGenerator * m_rand;

      void createEmptyWorkspace();

      /// Number of times extractData() was called since start or last reset.
      int m_timesCalled;

      /// For testing: set the reset flag after this many calls to extractData()
      int m_resetAfter;

      /// For testing: set the status flag to m_newStatus after this many calls to extractData()
      int m_changeStatusAfter;

      /// For testing: set the status flag to this after m_changeStatusAfter calls to extractData()
      ILiveListener::RunStatus m_newStatus;

    };

  } // namespace LiveData
} // namespace Mantid

#endif  /* MANTID_LIVEDATA_TESTDATALISTENER_H_ */
