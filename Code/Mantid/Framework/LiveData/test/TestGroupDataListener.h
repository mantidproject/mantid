#ifndef MANTID_LIVEDATA_TESTGROUPDATALISTENER_H_
#define MANTID_LIVEDATA_TESTGROUPDATALISTENER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ILiveListener.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace Mantid
{
  namespace LiveData
  {
    /** An implementation of ILiveListener for testing purposes that gives back a buffer
        with an identical number of events every time extractData is called.
     */
    class TestGroupDataListener : public API::ILiveListener
    {
    public:
      TestGroupDataListener();
      ~TestGroupDataListener();

      std::string name() const { return "TestDataListener"; }
      bool supportsHistory() const { return false; }
      bool buffersEvents() const { return true; }

      bool connect(const Poco::Net::SocketAddress& address);
      void start(Kernel::DateAndTime startTime = Kernel::DateAndTime());
      boost::shared_ptr<API::Workspace> extractData();

      bool isConnected();
      ILiveListener::RunStatus runStatus();
      int runNumber() const;

    private:
      API::WorkspaceGroup_sptr m_buffer;

      void createWorkspace();

    };

  } // namespace LiveData
} // namespace Mantid

#endif  /* MANTID_LIVEDATA_TESTGROUPDATALISTENER_H_ */
