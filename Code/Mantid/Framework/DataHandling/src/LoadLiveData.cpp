/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
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
  Mantid::API::MatrixWorkspace_sptr LoadLiveData::processChunk(Mantid::API::MatrixWorkspace_sptr chunkWS)
  {
    // TODO: Data processing
    return chunkWS;
  }


  //----------------------------------------------------------------------------------------------
  /** Accumulate the data by adding (summing) to the output workspace.
   * Calls the Plus algorithm
   *
   * @param chunkWS :: processed live data chunk workspace
   */
  void LoadLiveData::addOutput(Mantid::API::MatrixWorkspace_sptr chunkWS)
  {
    Mantid::API::MatrixWorkspace_sptr outWS = this->getProperty("OutputWorkspace");
    IAlgorithm_sptr alg = this->createSubAlgorithm("Plus");
    alg->setProperty("LHSWorkspace", outWS);
    alg->setProperty("RHSWorkspace", chunkWS);
    alg->setProperty("OutputWorkspace", outWS);
    alg->execute();
    if (!alg->isExecuted())
    {
      throw std::runtime_error("Error when calling Plus to add the chunk of live data. See log.");
    }
    else
    {
      outWS = alg->getProperty("OutputWorkspace");
      this->setProperty("OutputWorkspace", outWS);
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Accumulate the data by replacing the output workspace
   *
   * @param chunkWS :: processed live data chunk workspace
   */
  void LoadLiveData::replaceOutput(Mantid::API::MatrixWorkspace_sptr chunkWS)
  {
    // When the algorithm exits the chunk workspace will be renamed
    // and overwrite the old one
    this->setProperty("OutputWorkspace", chunkWS);
  }


  //----------------------------------------------------------------------------------------------
  /** Accumulate the data by conjoining the spectra (adding them)
   * to the output workspace.
   * Calls ConjoinWorkspaces algorithm.
   *
   * @param chunkWS :: processed live data chunk workspace
   */
  void LoadLiveData::conjoinOutput(Mantid::API::MatrixWorkspace_sptr chunkWS)
  {
    Mantid::API::MatrixWorkspace_sptr outWS = this->getProperty("OutputWorkspace");
    IAlgorithm_sptr alg = this->createSubAlgorithm("ConjoinWorkspaces");
    alg->setProperty("InputWorkspace1", outWS);
    alg->setProperty("InputWorkspace2", chunkWS);
    alg->setProperty("CheckOverlapping", false);
    alg->execute();
    if (!alg->isExecuted())
    {
      throw std::runtime_error("Error when calling ConjoinWorkspaces to conjoin the spectra of the chunk of live data. See log.");
    }
    else
    {
      outWS = alg->getProperty("InputWorkspace1");
      this->setProperty("OutputWorkspace", outWS);
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadLiveData::exec()
  {
    // The full (accumulated) output workspace
    MatrixWorkspace_sptr outWS = this->getProperty("OutputWorkspace");

    // Get or create the live listener
    ILiveListener_sptr listener = this->getLiveListener();

    // The listener returns a MatrixWorkspace containing the chunk of live data.
    MatrixWorkspace_sptr chunkWS = listener->extractData();

    // TODO: Have the ILiveListener tell me exactly the time stamp
    DateAndTime lastTimeStamp = DateAndTime::get_current_time();
    this->setPropertyValue("LastTimeStamp", lastTimeStamp.to_ISO8601_string());

    // Now we process the chunk
    MatrixWorkspace_sptr processed = this->processChunk(chunkWS);

    // How do we accumulate the data?
    std::string accum = this->getPropertyValue("AccumulationMethod");

    // If the output does not exist, we always replace the output.
    if (!outWS)
      accum = "Replace";

    // Perform the accumulation and set the output workspace
    if (accum == "Replace")
      this->replaceOutput(processed);
    else if (accum == "Conjoin")
      this->conjoinOutput(processed);
    else
      // Default to Add.
      this->addOutput(processed);

  }



} // namespace Mantid
} // namespace DataHandling
