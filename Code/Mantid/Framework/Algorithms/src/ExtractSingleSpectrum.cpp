//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ExtractSingleSpectrum.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractSingleSpectrum)

using namespace Kernel;
using namespace API;

void ExtractSingleSpectrum::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "The name of the input workspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name under which to store the output workspace.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", -1, mustBePositive,
                  "The workspace index number of the spectrum to extract.");
}

void ExtractSingleSpectrum::exec() {
  // Get hold of the input workspace
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  const int indexToExtract = getProperty("WorkspaceIndex");
  const size_t numHist = inputWorkspace->getNumberHistograms();
  if (static_cast<size_t>(indexToExtract) >= numHist) {
    throw Exception::IndexError(
        indexToExtract, inputWorkspace->getNumberHistograms(), this->name());
  }

  // Let crop do the rest
  IAlgorithm_sptr cropper =
      this->createChildAlgorithm("CropWorkspace", 0.0, 1.0);
  cropper->setProperty("InputWorkspace", inputWorkspace);
  cropper->setProperty("StartWorkspaceIndex", indexToExtract);
  cropper->setProperty("EndWorkspaceIndex", indexToExtract);
  cropper->executeAsChildAlg();

  setProperty<MatrixWorkspace_sptr>("OutputWorkspace",
                                    cropper->getProperty("OutputWorkspace"));
}

} // namespace Algorithms
} // namespace Mantid
