/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAlgorithms/CreateFlatEventWorkspace.h"


#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreateFlatEventWorkspace)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreateFlatEventWorkspace::CreateFlatEventWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreateFlatEventWorkspace::~CreateFlatEventWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string CreateFlatEventWorkspace::name() const { return "CreateFlatEventWorkspace";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int CreateFlatEventWorkspace::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string CreateFlatEventWorkspace::category() const { return "CorrectionFunctions\\BackgroundCorrections";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CreateFlatEventWorkspace::initDocs()
  {
    this->setWikiSummary("Creates a flat event workspace that can be used for background removal.");
    this->setOptionalMessage("Creates a flat event workspace that can be used for background removal.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CreateFlatEventWorkspace::init()
  {
      this->declareProperty(new Mantid::API::WorkspaceProperty<EventWorkspace>("InputWorkspace","",Mantid::Kernel::Direction::Input),
                      "An input event workspace to use as a source for the events.");

      this->declareProperty("RangeStart", EMPTY_DBL(), "Set the lower bound for sampling the background.");
      this->declareProperty("RangeEnd", EMPTY_DBL(), "Set the upper bound for sampling the background.");

      this->declareProperty(new Mantid::API::WorkspaceProperty<EventWorkspace>("OutputWorkspace","",Mantid::Kernel::Direction::Output),
                      "Output event workspace containing a flat background.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CreateFlatEventWorkspace::exec()
  {
      // Get the workspaces
      EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
      EventWorkspace_sptr outputWS = getProperty("OutputWorkspace");

      std::string outputWsName = this->getPropertyValue("OutputWorkspace");

      // Get the background region start/end
      double start = getProperty("RangeStart");
      double end = getProperty("RangeEnd");

      double sampleRange = end - start;
      g_log.debug() << "Total Range = " << sampleRange << std::endl;

      // What are the min/max values for the experimental data ?
      double dataMin, dataMax;
      inputWS->getEventXMinMax(dataMin, dataMax);

      g_log.debug() << "Data Range (" << dataMin << " < x < " << dataMax << ")" << std::endl;

      // How many times do we need to replicate the extracted background region in order to fill up
      // the entire tof/x range covered by the data ?
      int nRegions = ((dataMax - dataMin) / sampleRange);

      g_log.debug() << "We will need to replicate the selected region " << nRegions << " times." << std::endl;

      // Extract the region we are using for the background
      IAlgorithm_sptr crop_alg = this->createSubAlgorithm("CropWorkspace");
      crop_alg->setAlwaysStoreInADS(true);
      crop_alg->setProperty("InputWorkspace", inputWS);
      crop_alg->setProperty("XMin", start);
      crop_alg->setProperty("XMax", end);
      crop_alg->setPropertyValue("OutputWorkspace", "__extracted_chunk");
      crop_alg->execute();

      // Now lets shift the region to the start of the data.
      IAlgorithm_sptr shift_alg = this->createSubAlgorithm("ChangeBinOffset");
      shift_alg->setAlwaysStoreInADS(true);
      shift_alg->setPropertyValue("InputWorkspace", "__extracted_chunk");
      shift_alg->setPropertyValue("OutputWorkspace", outputWsName);
      shift_alg->setProperty("Offset", -(start - dataMin));
      shift_alg->executeAsSubAlg();

      IAlgorithm_sptr clone = this->createSubAlgorithm("CloneWorkspace");
      clone->setAlwaysStoreInADS(true);
      clone->setPropertyValue("InputWorkspace", outputWsName);
      clone->setPropertyValue("OutputWorkspace", "__background_chunk");
      clone->executeAsSubAlg();

      for (int i = 0; i < nRegions; ++i) {

          IAlgorithm_sptr shiftchunk = this->createSubAlgorithm("ChangeBinOffset");
          shiftchunk->setAlwaysStoreInADS(true);
          shiftchunk->setPropertyValue("InputWorkspace", "__background_chunk");
          shiftchunk->setPropertyValue("OutputWorkspace", "__background_chunk");
          shiftchunk->setProperty("Offset", sampleRange);
          shiftchunk->executeAsSubAlg();

          // Now add this chunk onto the output
          IAlgorithm_sptr plus_alg = this->createSubAlgorithm("Plus");
          plus_alg->setAlwaysStoreInADS(true);
          plus_alg->setPropertyValue("LHSWorkspace", outputWsName);
          plus_alg->setPropertyValue("RHSWorkspace", "__background_chunk");
          plus_alg->setPropertyValue("OutputWorkspace", outputWsName);
          plus_alg->executeAsSubAlg();
      }

      // Crop the output workspace to be the same range as the input data
      IAlgorithm_sptr finalcrop_alg = this->createSubAlgorithm("CropWorkspace");
      finalcrop_alg->setAlwaysStoreInADS(true);
      finalcrop_alg->setProperty("InputWorkspace", outputWsName);
      finalcrop_alg->setProperty("XMin", dataMin);
      finalcrop_alg->setProperty("XMax", dataMax);
      finalcrop_alg->setPropertyValue("OutputWorkspace", outputWsName);
      finalcrop_alg->execute();

      this->setProperty("OutputWorkspace", outputWsName);
  }



} // namespace Algorithms
} // namespace Mantid
