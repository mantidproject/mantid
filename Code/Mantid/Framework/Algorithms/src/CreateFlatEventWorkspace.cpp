#include "MantidAlgorithms/CreateFlatEventWorkspace.h"

#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateFlatEventWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CreateFlatEventWorkspace::CreateFlatEventWorkspace() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CreateFlatEventWorkspace::~CreateFlatEventWorkspace() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string CreateFlatEventWorkspace::name() const {
  return "CreateFlatEventWorkspace";
};

/// Algorithm's version for identification. @see Algorithm::version
int CreateFlatEventWorkspace::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateFlatEventWorkspace::category() const {
  return "CorrectionFunctions\\BackgroundCorrections";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateFlatEventWorkspace::init() {
  this->declareProperty(
      new Mantid::API::WorkspaceProperty<EventWorkspace>(
          "InputWorkspace", "", Mantid::Kernel::Direction::Input),
      "An input event workspace to use as a source for the events.");

  this->declareProperty("RangeStart", EMPTY_DBL(),
                        "Set the lower bound for sampling the background.");
  this->declareProperty("RangeEnd", EMPTY_DBL(),
                        "Set the upper bound for sampling the background.");

  this->declareProperty(
      new Mantid::API::WorkspaceProperty<>("OutputWorkspace", "",
                                           Mantid::Kernel::Direction::Output),
      "Output event workspace containing a flat background.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateFlatEventWorkspace::exec() {
  // Get the workspaces
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS;

  // Get the background region start/end
  double start = getProperty("RangeStart");
  double end = getProperty("RangeEnd");

  double sampleRange = end - start;
  g_log.debug() << "Total Range = " << sampleRange << std::endl;

  // What are the min/max values for the experimental data ?
  double dataMin, dataMax;
  inputWS->getEventXMinMax(dataMin, dataMax);

  g_log.debug() << "Data Range (" << dataMin << " < x < " << dataMax << ")"
                << std::endl;

  // How many times do we need to replicate the extracted background region in
  // order to fill up
  // the entire tof/x range covered by the data ?
  int nRegions = static_cast<int>((dataMax - dataMin) / sampleRange);

  g_log.debug() << "We will need to replicate the selected region " << nRegions
                << " times." << std::endl;

  // Extract the region we are using for the background
  IAlgorithm_sptr crop_alg = this->createChildAlgorithm("CropWorkspace");
  crop_alg->setProperty("InputWorkspace", inputWS);
  crop_alg->setProperty("XMin", start);
  crop_alg->setProperty("XMax", end);
  crop_alg->setPropertyValue("OutputWorkspace", "__extracted_chunk");
  crop_alg->execute();
  MatrixWorkspace_sptr chunkws = crop_alg->getProperty("OutputWorkspace");

  // Now lets shift the region to the start of the data.
  IAlgorithm_sptr shift_alg = this->createChildAlgorithm("ChangeBinOffset");
  shift_alg->setProperty("InputWorkspace", chunkws);
  // shift_alg->setPropertyValue("OutputWorkspace", outputWsName);
  shift_alg->setProperty("Offset", -(start - dataMin));
  shift_alg->executeAsChildAlg();
  outputWS = shift_alg->getProperty("OutputWorkspace");

  IAlgorithm_sptr clone = this->createChildAlgorithm("CloneWorkspace");
  clone->setProperty("InputWorkspace", outputWS);
  clone->setPropertyValue("OutputWorkspace", "__background_chunk");
  clone->executeAsChildAlg();
  Workspace_sptr tmp = clone->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr tmpChunkWs =
      boost::dynamic_pointer_cast<MatrixWorkspace>(tmp);

  for (int i = 0; i < nRegions; ++i) {

    IAlgorithm_sptr shiftchunk = this->createChildAlgorithm("ChangeBinOffset");
    shiftchunk->setProperty("InputWorkspace", tmpChunkWs);
    shiftchunk->setProperty("OutputWorkspace", tmpChunkWs);
    shiftchunk->setProperty("Offset", sampleRange);
    shiftchunk->executeAsChildAlg();
    tmpChunkWs = shiftchunk->getProperty("OutputWorkspace");

    // Now add this chunk onto the output
    IAlgorithm_sptr plus_alg = this->createChildAlgorithm("Plus");
    plus_alg->setProperty("LHSWorkspace", outputWS);
    plus_alg->setProperty("RHSWorkspace", tmpChunkWs);
    plus_alg->setProperty("OutputWorkspace", outputWS);
    plus_alg->executeAsChildAlg();
    outputWS = plus_alg->getProperty("OutputWorkspace");
    tmpChunkWs = plus_alg->getProperty("RHSWorkspace");
  }

  // Crop the output workspace to be the same range as the input data
  IAlgorithm_sptr finalcrop_alg = this->createChildAlgorithm("CropWorkspace");
  finalcrop_alg->setProperty("InputWorkspace", outputWS);
  finalcrop_alg->setProperty("XMin", dataMin);
  finalcrop_alg->setProperty("XMax", dataMax);
  finalcrop_alg->execute();
  outputWS = finalcrop_alg->getProperty("OutputWorkspace");

  EventWorkspace_sptr outputEWS =
      boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
  outputEWS->clearMRU();

  // Need to reset the matrixworkspace/histogram representation to be the
  // whole xrange (rather than just the extracted chunk).
  MantidVecPtr xnew;
  outputEWS->getEventXMinMax(dataMin, dataMax);
  xnew.access().push_back(dataMin);
  xnew.access().push_back(dataMax);
  outputEWS->setAllX(xnew);

  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
