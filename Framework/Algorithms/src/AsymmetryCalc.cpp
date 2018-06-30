//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AsymmetryCalc.h"
#include "MantidAPI/HistoWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayProperty.h"

#include <cmath>
#include <vector>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using API::Progress;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(AsymmetryCalc)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void AsymmetryCalc::init() {

  declareProperty(make_unique<API::WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "Name of the input workspace");
  declareProperty(
      make_unique<API::WorkspaceProperty<>>("OutputWorkspace", "",
                                            Direction::Output),
      "The name of the workspace to be created as the output of the algorithm");

  declareProperty(make_unique<ArrayProperty<int>>("ForwardSpectra"),
                  "The spectra numbers of the forward group");
  declareProperty(make_unique<ArrayProperty<int>>("BackwardSpectra"),
                  "The spectra numbers of the backward group");
  declareProperty("Alpha", 1.0, "The balance parameter (default 1)",
                  Direction::Input);
}

//----------------------------------------------------------------------------------------------
/** Validates the inputs.
 */
std::map<std::string, std::string> AsymmetryCalc::validateInputs() {

  std::map<std::string, std::string> result;

  std::vector<int> forwd = getProperty("ForwardSpectra");
  std::vector<int> backwd = getProperty("BackwardSpectra");

  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  if (inputWS) {
    auto list = inputWS->getIndicesFromSpectra(forwd);
    if (forwd.size() != list.size()) {
      result["ForwardSpectra"] =
          "Some of the spectra can not be found in the input workspace";
    }

    list = inputWS->getIndicesFromSpectra(backwd);
    if (backwd.size() != list.size()) {
      result["BackwardSpectra"] =
          "Some of the spectra can not be found in the input workspace";
    }
  }
  return result;
}

/** Executes the algorithm
 *
 */
void AsymmetryCalc::exec() {
  std::vector<int> forward_list = getProperty("ForwardSpectra");
  std::vector<int> backward_list = getProperty("BackwardSpectra");
  int forward = !forward_list.empty() ? forward_list[0] : 1;
  int backward = !backward_list.empty() ? backward_list[0] : 2;
  double alpha = getProperty("Alpha");

  // Get original workspace
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Make an intermediate workspace and prepare it for asymmetry calculation
  API::MatrixWorkspace_sptr tmpWS;
  if (forward_list.size() > 1 || backward_list.size() > 1) {
    // If forward or backward lists have more than 1 entries spectra need to be
    // grouped

    // First group spectra from the backward list leaving the rest ungrouped
    API::IAlgorithm_sptr group = createChildAlgorithm("GroupDetectors");
    group->setProperty("InputWorkspace", inputWS);
    group->setProperty("SpectraList", backward_list);
    group->setProperty("KeepUngroupedSpectra", true);
    group->execute();
    tmpWS = group->getProperty("OutputWorkspace");

    // Then group spectra from the forward list leaving the rest ungrouped
    group = createChildAlgorithm("GroupDetectors");
    group->setProperty("InputWorkspace", tmpWS);
    group->setProperty("SpectraList", forward_list);
    group->setProperty("KeepUngroupedSpectra", true);
    group->execute();
    tmpWS = group->getProperty("OutputWorkspace");

    // The order of grouping leaves the forward spectra group in the first
    // histogram
    // and the barckward one is the second
    forward = 0;
    backward = 1;
  } else {
    // If the forward and backward lists are empty or have at most 1 index
    // there is no need for grouping and the input workspace can be used
    // directly
    tmpWS = inputWS;

    // get workspace indices from spectra ids for forward and backward
    std::vector<specnum_t> specIDs(2);
    specIDs[0] = forward;
    specIDs[1] = backward;
    std::vector<size_t> indices = tmpWS->getIndicesFromSpectra(specIDs);

    forward = static_cast<int>(indices[0]);
    backward = static_cast<int>(indices[1]);
  }

  const size_t blocksize = inputWS->blocksize();
  assert(tmpWS->blocksize() == blocksize);

  // Create a point data workspace with only one spectra for forward
  auto outputWS = DataObjects::create<API::HistoWorkspace>(
      *inputWS, 1, tmpWS->points(forward));
  outputWS->getSpectrum(0).setDetectorID(static_cast<detid_t>(1));

  // Calculate asymmetry for each time bin
  // F-aB / F+aB
  Progress prog(this, 0.0, 1.0, blocksize);
  for (size_t j = 0; j < blocksize; ++j) {
    // cal F-aB
    double numerator = tmpWS->y(forward)[j] - alpha * tmpWS->y(backward)[j];
    // cal F+aB
    double denominator = (tmpWS->y(forward)[j] + alpha * tmpWS->y(backward)[j]);

    // cal F-aB / F+aB
    if (denominator != 0.0) {
      outputWS->mutableY(0)[j] = numerator / denominator;
    } else {
      outputWS->mutableY(0)[j] = 0.;
    }

    // Work out the error (as in 1st attachment of ticket #4188)
    double error = 1.0;
    if (denominator != 0.0) {
      // cal F + a2B
      double q1 = tmpWS->y(forward)[j] + alpha * alpha * tmpWS->y(backward)[j];
      // cal 1 + ((f-aB)/(F+aB))2
      double q2 = 1 + numerator * numerator / (denominator * denominator);
      // cal error
      error = sqrt(q1 * q2) / denominator;
    }
    outputWS->mutableE(0)[j] = error;

    prog.report();
  }

  assert(outputWS->x(0).size() == blocksize);

  // Update Y axis units
  outputWS->setYUnit("Asymmetry");

  setProperty("OutputWorkspace", std::move(outputWS));
}

} // namespace Algorithms
} // namespace Mantid
