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
      m_tempWSname("__filelistenerchunk"), m_nextChunk(1), m_chunkload(NULL)
  {
    if ( m_filename.empty() ) g_log.error("Configuration property fileeventdatalistener.filename not found. The algorithm will fail!");

    if ( ! ConfigService::Instance().getValue("fileeventdatalistener.chunks",m_numChunks) )
    {
      g_log.error("Configuration property fileeventdatalistener.chunks not found. The algorithm will fail!");
      m_numChunks = 0; // Set it to 0 so the algorithm just fails
    }
  }
    
  /// Destructor
  FileEventDataListener::~FileEventDataListener()
  {
    // Don't disappear until any running job has finished or bad things happen!
    if ( m_chunkload ) m_chunkload->wait();
    // Clean up the hidden workspace if necessary
    if ( AnalysisDataService::Instance().doesExist(m_tempWSname) ) AnalysisDataService::Instance().remove(m_tempWSname);
    // Don't leak memory
    delete m_chunkload;
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
    // Kick off loading the first chunk (which will include loading the instrument etc.)
    loadChunk();
    return;
  }

  boost::shared_ptr<MatrixWorkspace> FileEventDataListener::extractData()
  {
    // Once the end of the file is reached, this method throws to stop the calling algorithm.
    // This is equivalent to the end of the run - which we still need to figure out how to handle.
    if ( m_chunkload == NULL ) throw std::runtime_error("The whole file has been read!");

    // If the loading of the chunk isn't finished, then we need to wait
    m_chunkload->wait();
    if ( ! m_chunkload->data() ) throw std::runtime_error("LoadEventPreNexus failed for some reason.");
    // The loading succeeded: get the workspace from the ADS.
    MatrixWorkspace_sptr chunk = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("__filelistenerchunk");
    // Remove the workspace from the ADS now we've extracted it
    AnalysisDataService::Instance().remove(m_tempWSname);
    // Delete the ActiveResult to signify that we're done with it.
    delete m_chunkload;
    m_chunkload = NULL;
    // Kick off the loading of the next chunk (unless we're at the end of the file)
    if ( m_nextChunk <= m_numChunks )
    {
      loadChunk();
    }
    else
    {
      m_loader.reset(); // Clear the algorithm so that it releases its handle on the workspace
    }

    return chunk;
  }

  /// Load the next chunk of data. Calls Algorithm::executeAsync to do it in another thread.
  void FileEventDataListener::loadChunk()
  {
    m_loader = AlgorithmManager::Instance().createUnmanaged("LoadEventPreNexus");
    m_loader->initialize();
//    loader->setChild(true); // It can't be a child because the output needs to go in the ADS
    m_loader->setLogging(false);
    m_loader->setPropertyValue("EventFilename",m_filename);
    m_loader->setProperty("ChunkNumber",m_nextChunk++); // post-increment
    m_loader->setProperty("TotalChunks",m_numChunks);
    m_loader->setPropertyValue("OutputWorkspace",m_tempWSname); // Goes into 'hidden' workspace
    m_chunkload = new Poco::ActiveResult<bool>(m_loader->executeAsync());
  }

} // namespace Mantid
} // namespace DataHandling
