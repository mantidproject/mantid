#include "MantidDataHandling/FileEventDataListener.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{
  DECLARE_LISTENER(FileEventDataListener)

  // Get a reference to the logger
  Kernel::Logger& FileEventDataListener::g_log = Kernel::Logger::get("FileEventDataListener");

  /// Constructor
  FileEventDataListener::FileEventDataListener() : ILiveListener(),
      m_filename(ConfigService::Instance().getString("fileeventdatalistener.filename")),
      m_nextChunk(1),
      m_buffer()//, m_loadChunk(this,&FileEventDataListener::loadChunkImpl)
  {
    if ( m_filename.empty() ) g_log.error("Configuration property fileeventdatalistener.filename not found");

    if ( ! ConfigService::Instance().getValue("fileeventdatalistener.chunks",m_numChunks) )
    {
      g_log.error("Configuration property fileeventdatalistener.chunks not found");
    }
  }
    
  /// Destructor
  FileEventDataListener::~FileEventDataListener()
  {
  }
  
  bool FileEventDataListener::connect(const Poco::Net::SocketAddress&)
  {
    // Do nothing for now. Later, put in stuff to help test failure modes.
    return true;
  }

  bool FileEventDataListener::isConnected()
  {
    return true; // For the time being at least
  }

  void FileEventDataListener::start(Kernel::DateAndTime /*startTime*/) // Ignore the start time
  {
    return;
  }

  boost::shared_ptr<MatrixWorkspace> FileEventDataListener::extractData()
  {
    // For now, the load algorithm will just fail once it gets to the end of the file
    return loadChunkImpl(Poco::Void());
  }

  API::MatrixWorkspace_sptr FileEventDataListener::loadChunkImpl(Poco::Void)
  {
    Algorithm_sptr loader = AlgorithmManager::Instance().createUnmanaged("LoadEventPreNexus");
    loader->initialize();
    loader->setChild(true);
    loader->setLogging(false);
    loader->setPropertyValue("EventFilename",m_filename);
    loader->setProperty("ChunkNumber",m_nextChunk++); // Increment
    loader->setProperty("TotalChunks",m_numChunks);
    loader->setPropertyValue("OutputWorkspace","anon"); // Fake name
    loader->execute();

    IEventWorkspace_sptr evWS = loader->getProperty("OutputWorkspace");
    return evWS;
  }
} // namespace Mantid
} // namespace DataHandling
