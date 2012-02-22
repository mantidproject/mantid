/*WIKI*

* First, a chunk of data is loaded from the [[LiveListener]].
** This is saved in the '''ChunkWS''' workspace.
* The '''ProcessingAlgorithm''' is called to process the ChunkWS.
** The output is the '''ProcessedChunkWS'''.
* The ProcessedChunkWS is added to the '''AccumulationWorkspace'''.
** The process can be Add, Conjoin, or Replace.

* If you've specified '''PostProcessingAlgorithm''':
** The AccumulationWorkspace is processed to create the '''OutputWorkspace'''.
* If there is NO PostProcessingAlgorithm:
** The OutputWorkspace is the same as the AccumulationWorkspace, which you do not need to specify.

*WIKI*/

#include "MantidDataHandling/LoadLiveData.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadLiveData)
  

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadLiveData::LoadLiveData()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadLiveData::~LoadLiveData()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string LoadLiveData::name() const { return "LoadLiveData";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int LoadLiveData::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadLiveData::initDocs()
  {
    this->setWikiSummary("Load a chunk of live data. You should call StartLiveData, and not this algorithm directly.");
    this->setOptionalMessage("Load a chunk of live data. You should call StartLiveData, and not this algorithm directly.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadLiveData::init()
  {
    this->initProps();
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the processing on the chunk of workspace data, using the
   * algorithm or scrip given in the algorithm properties
   *
   * @param chunkWS :: chunk workspace to process
   * @return the processed workspace sptr
   */
  Mantid::API::Workspace_sptr LoadLiveData::processChunk(Mantid::API::MatrixWorkspace_sptr chunkWS)
  {
    // TODO: Data processing
    return chunkWS;
  }


  //----------------------------------------------------------------------------------------------
  /** Perform the PostProcessing steps on the accumulated workspace.
   * Uses the m_accumWS member in a (hopefully) read-only manner.
   * Sets the m_outputWS member to the processed result.
   */
  void LoadLiveData::runPostProcessing()
  {
    // TODO: Data processing
    m_outputWS = m_accumWS;
  }


  //----------------------------------------------------------------------------------------------
  /** Accumulate the data by adding (summing) to the output workspace.
   * Calls the Plus algorithm
   * Sets m_accumWS.
   *
   * @param chunkWS :: processed live data chunk workspace
   */
  void LoadLiveData::addChunk(Mantid::API::Workspace_sptr chunkWS)
  {
    IAlgorithm_sptr alg = this->createSubAlgorithm("Plus");
    alg->setProperty("LHSWorkspace", m_accumWS);
    alg->setProperty("RHSWorkspace", chunkWS);
    alg->setProperty("OutputWorkspace", m_accumWS);
    alg->execute();
    if (!alg->isExecuted())
    {
      throw std::runtime_error("Error when calling Plus to add the chunk of live data. See log.");
    }
    else
    {
      // TODO: What about workspace groups?
      MatrixWorkspace_sptr temp = alg->getProperty("OutputWorkspace");
      m_accumWS = temp;
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Accumulate the data by replacing the output workspace.
   * Sets m_accumWS.
   *
   * @param chunkWS :: processed live data chunk workspace
   */
  void LoadLiveData::replaceChunk(Mantid::API::Workspace_sptr chunkWS)
  {
    // When the algorithm exits the chunk workspace will be renamed
    // and overwrite the old one
    m_accumWS = chunkWS;
  }


  //----------------------------------------------------------------------------------------------
  /** Accumulate the data by conjoining the spectra (adding them)
   * to the output workspace.
   * Calls ConjoinWorkspaces algorithm.
   * Sets m_accumWS.
   *
   * @param chunkWS :: processed live data chunk workspace
   */
  void LoadLiveData::conjoinChunk(Mantid::API::Workspace_sptr chunkWS)
  {
    IAlgorithm_sptr alg = this->createSubAlgorithm("ConjoinWorkspaces");
    alg->setProperty("InputWorkspace1", m_accumWS);
    alg->setProperty("InputWorkspace2", chunkWS);
    alg->setProperty("CheckOverlapping", false);
    alg->execute();
    if (!alg->isExecuted())
    {
      throw std::runtime_error("Error when calling ConjoinWorkspaces to conjoin the spectra of the chunk of live data. See log.");
    }
    else
    {
      // TODO: What about workspace groups?
      MatrixWorkspace_sptr temp = alg->getProperty("OutputWorkspace");
      m_accumWS = temp;
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadLiveData::exec()
  {
    // The full, post-processed output workspace
    m_outputWS = this->getProperty("OutputWorkspace");

    // Validate inputs
    if (this->hasPostProcessing())
    {
      if (this->getPropertyValue("AccumulationWorkspace").empty())
        throw std::invalid_argument("Must specify the AccumulationWorkspace parameter if using PostProcessing.");

      // The accumulated but not post-processed output workspace
      m_accumWS = this->getProperty("AccumulationWorkspace");
    }
    else
    {
      // No post-processing, so the accumulation and output are the same
      m_accumWS = m_outputWS;
    }

    // Get or create the live listener
    ILiveListener_sptr listener = this->getLiveListener();

    // The listener returns a MatrixWorkspace containing the chunk of live data.
    MatrixWorkspace_sptr chunkWS = listener->extractData();

    // TODO: Have the ILiveListener tell me exactly the time stamp
    DateAndTime lastTimeStamp = DateAndTime::get_current_time();
    this->setPropertyValue("LastTimeStamp", lastTimeStamp.to_ISO8601_string());

    // Now we process the chunk
    Workspace_sptr processed = this->processChunk(chunkWS);

    // How do we accumulate the data?
    std::string accum = this->getPropertyValue("AccumulationMethod");

    // If the AccumulationWorkspace does not exist, we always replace the AccumulationWorkspace.
    if (!m_accumWS)
      accum = "Replace";

    g_log.notice() << "Performing the " << accum << " operation." << std::endl;

    // Perform the accumulation and set the AccumulationWorkspace workspace
    if (accum == "Replace")
      this->replaceChunk(processed);
    else if (accum == "Conjoin")
      this->conjoinChunk(processed);
    else
      // Default to Add.
      this->addChunk(processed);

    // At this point, m_accumWS is set.

    if (this->hasPostProcessing())
    {
      // ----------- Run post-processing -------------
      this->runPostProcessing();
      // Set both output workspaces
      this->setProperty("AccumulationWorkspace", m_accumWS);
      this->setProperty("OutputWorkspace", m_outputWS);
    }
    else
    {
      // ----------- No post-processing -------------
      m_outputWS = m_accumWS;
      // We DO NOT set AccumulationWorkspace.
      this->setProperty("OutputWorkspace", m_outputWS);
    }

  }



} // namespace Mantid
} // namespace DataHandling
