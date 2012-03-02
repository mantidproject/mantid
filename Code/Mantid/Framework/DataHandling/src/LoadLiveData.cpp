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
#include "MantidKernel/WriteLock.h"
#include "MantidKernel/ReadLock.h"
#include "MantidAPI/Workspace.h"

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
  : LiveDataAlgorithm()
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
  /** Run either the chunk or post-processing step
   *
   * @param inputWS :: workspace being processed
   * @param PostProcess :: flag, TRUE if doing the post-processing
   * @return the processed workspace. Will point to inputWS if no processing is to do
   */
  Mantid::API::Workspace_sptr LoadLiveData::runProcessing(Mantid::API::Workspace_sptr inputWS, bool PostProcess)
  {
    if (!inputWS)
      throw std::runtime_error("LoadLiveData::runProcessing() called for an empty input workspace.");
    // Prevent others writing to the workspace while we run.
    ReadLock _lock(*inputWS);

    // Make algorithm and set the properties
    IAlgorithm_sptr alg = this->makeAlgorithm(PostProcess);
    if (alg)
    {
      if (PostProcess)
        g_log.notice() << "Performing post-processing";
      else
        g_log.notice() << "Performing chunk processing";
      g_log.notice() << " using " << alg->name() << std::endl;

      // Run the processing algorithm

      // Make a unique anonymous names for the workspace, to put in ADS
      std::string inputName = "__anonymous_livedata_input_" + this->getPropertyValue("OutputWorkspace");
      // Transform the chunk in-place
      std::string outputName = inputName;

      // Except, no need for anonymous names with the post-processing
      if (PostProcess)
      {
        inputName = this->getPropertyValue("AccumulationWorkspace");
        outputName = this->getPropertyValue("OutputWorkspace");
      }

      // For python scripts to work we need to go through the ADS
      AnalysisDataService::Instance().addOrReplace(inputName, inputWS);
      if (!AnalysisDataService::Instance().doesExist(inputName))
        g_log.error() << "Something really wrong happened when adding " << inputName << " to ADS. " << this->getPropertyValue("OutputWorkspace") << std::endl;

      alg->setPropertyValue("InputWorkspace", inputName);
      alg->setPropertyValue("OutputWorkspace", outputName);
      alg->setChild(true);
      alg->execute();
      if (!alg->isExecuted())
        throw std::runtime_error("Error processing the workspace using " + alg->name() + ". See log for details.");

      // Retrieve the output.
      Property * prop = alg->getProperty("OutputWorkspace");
      IWorkspaceProperty * wsProp = dynamic_cast<IWorkspaceProperty*>(prop);
      if (!wsProp)
        throw std::runtime_error("The " + alg->name() + " Algorithm's OutputWorkspace property is not a WorkspaceProperty!");
      Workspace_sptr temp = wsProp->getWorkspace();

      if (!PostProcess)
      {
        // Remove the chunk workspace from the ADS, it is no longer needed there.
        AnalysisDataService::Instance().remove(inputName);
      }

      return temp;
    }
    else
      // Don't do any processing.
      return inputWS;
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
    return runProcessing(chunkWS, false);
  }


  //----------------------------------------------------------------------------------------------
  /** Perform the PostProcessing steps on the accumulated workspace.
   * Uses the m_accumWS member in a (hopefully) read-only manner.
   * Sets the m_outputWS member to the processed result.
   */
  void LoadLiveData::runPostProcessing()
  {
    m_outputWS = runProcessing(m_accumWS, true);
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
    // Acquire locks on the workspaces we use
    WriteLock _lock1(*m_accumWS);
    ReadLock _lock2(*chunkWS);

    // Choose the appropriate algorithm to add chunks
    std::string algoName = "PlusMD";
    MatrixWorkspace_sptr mws = boost::dynamic_pointer_cast<MatrixWorkspace>(chunkWS);
    if (mws) algoName = "Plus";

    IAlgorithm_sptr alg = this->createSubAlgorithm(algoName);
    alg->setProperty("LHSWorkspace", m_accumWS);
    alg->setProperty("RHSWorkspace", chunkWS);
    alg->setProperty("OutputWorkspace", m_accumWS);
    alg->execute();
    if (!alg->isExecuted())
    {
      throw std::runtime_error("Error when calling " + alg->name() + " to add the chunk of live data. See log.");
    }
    else
    {
      // Get the output as the generic Workspace type
      Property * prop = alg->getProperty("OutputWorkspace");
      IWorkspaceProperty * wsProp = dynamic_cast<IWorkspaceProperty*>(prop);
      if (!wsProp)
        throw std::runtime_error("The " + alg->name() + " Algorithm's OutputWorkspace property is not a WorkspaceProperty!");
      Workspace_sptr temp = wsProp->getWorkspace();
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
  /** Accumulate the data by appending the spectra into the
   * the output workspace.
   * Calls AppendSpectra algorithm.
   * Sets m_accumWS.
   *
   * @param chunkWS :: processed live data chunk workspace
   */
  void LoadLiveData::appendChunk(Mantid::API::Workspace_sptr chunkWS)
  {
    IAlgorithm_sptr alg;
    {
      ReadLock _lock1(*m_accumWS);
      ReadLock _lock2(*chunkWS);

      alg = this->createSubAlgorithm("AppendSpectra");
      alg->setProperty("InputWorkspace1", m_accumWS);
      alg->setProperty("InputWorkspace2", chunkWS);
      alg->setProperty("ValidateInputs", false);
      alg->execute();
      if (!alg->isExecuted())
      {
        throw std::runtime_error("Error when calling conjoinChunk to append the spectra of the chunk of live data. See log.");
      }
    } // Release the locks.

    // TODO: What about workspace groups?
    MatrixWorkspace_sptr temp = alg->getProperty("OutputWorkspace");
    m_accumWS = temp;
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadLiveData::exec()
  {
    this->validateInputs();

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
    DateAndTime lastTimeStamp = DateAndTime::getCurrentTime();
    this->setPropertyValue("LastTimeStamp", lastTimeStamp.toISO8601String());

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
    else if (accum == "Append")
      this->appendChunk(processed);
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
