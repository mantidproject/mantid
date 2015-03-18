//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>

#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/AsymmetryCalc.h"

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

  declareProperty(
      new API::WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "Name of the input workspace");
  declareProperty(
      new API::WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name of the workspace to be created as the output of the algorithm");

  declareProperty(new ArrayProperty<int>("ForwardSpectra"),
                  "The spectra numbers of the forward group");
  declareProperty(new ArrayProperty<int>("BackwardSpectra"),
                  "The spectra numbers of the backward group");
  declareProperty("Alpha", 1.0, "The balance parameter (default 1)",
                  Direction::Input);
}

/** Executes the algorithm
 *
 */
void AsymmetryCalc::exec() {
  std::vector<int> forward_list = getProperty("ForwardSpectra");
  std::vector<int> backward_list = getProperty("BackwardSpectra");
  int forward = forward_list.size() ? forward_list[0] : 1;
  int backward = backward_list.size() ? backward_list[0] : 2;
  double alpha = getProperty("Alpha");

  // Get original workspace
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Make an intermediate workspace and prepare it for asymmetry calculation
  API::MatrixWorkspace_sptr tmpWS;
  if (forward_list.size() > 1 || backward_list.size() > 1) {
    // If forward or backward lists have more than 1 entries spectra need to be grouped

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

    // The order of grouping leaves the forward spectra group in the first histogram
    // and the barckward one is the second
    forward = 0;
    backward = 1;
  } else {
    // If the forward and backward lists are empty or have at most 1 index
    // there is no need for grouping and the input workspace can be used
    // directly
    tmpWS = inputWS;

    // get workspace indices from spectra ids for forward and backward
    std::vector<specid_t> specIDs(2);
    specIDs[0] = forward;
    specIDs[1] = backward;
    std::vector<size_t> indices;
    tmpWS->getIndicesFromSpectra(specIDs, indices);

    // If some spectra were not found, can't continue
    if (specIDs.size() != indices.size())
      throw std::invalid_argument(
          "Some of the spectra specified do not exist in a workspace");

    forward = static_cast<int>(indices[0]);
    backward = static_cast<int>(indices[1]);
  }

  const size_t blocksize = inputWS->blocksize();
  assert( tmpWS->blocksize() == blocksize );
  const bool isInputHistogram = inputWS->isHistogramData();

  // Create a point data workspace with only one spectra for forward
  API::MatrixWorkspace_sptr outputWS = API::WorkspaceFactory::Instance().create(
      inputWS, 1, blocksize, blocksize);

  // Get the reference to the input x values.
  auto& tmpX = tmpWS->readX(forward);

  // Calculate asymmetry for each time bin
  // F-aB / F+aB
  Progress prog(this, 0.0, 1.0, blocksize);
  for (size_t j = 0; j < blocksize; ++j) {
    // cal F-aB
    double numerator =
        tmpWS->dataY(forward)[j] - alpha * tmpWS->dataY(backward)[j];
    // cal F+aB
    double denominator =
        (tmpWS->dataY(forward)[j] + alpha * tmpWS->dataY(backward)[j]);

    // cal F-aB / F+aB
    if (denominator) {
      outputWS->dataY(0)[j] = numerator / denominator;
    } else {
      outputWS->dataY(0)[j] = 0.;
    }

    // Work out the error (as in 1st attachment of ticket #4188)
    double error = 1.0;
    if (denominator) {
      // cal F + a2B
      double q1 =
          tmpWS->dataY(forward)[j] + alpha * alpha * tmpWS->dataY(backward)[j];
      // cal 1 + ((f-aB)/(F+aB))2
      double q2 = 1 + numerator * numerator / (denominator * denominator);
      // cal error
      error = sqrt(q1 * q2) / denominator;
    }
    outputWS->dataE(0)[j] = error;

    // set the x values
    if ( isInputHistogram )
    {
      outputWS->dataX(0)[j] = ( tmpX[j] + tmpX[j+1] ) / 2;
    }
    else
    {
      outputWS->dataX(0)[j] = tmpX[j];
    }

    prog.report();
  }

  assert( outputWS->dataX(0).size() == blocksize );

  // Update Y axis units
  outputWS->setYUnit("Asymmetry");

  setProperty("OutputWorkspace", outputWS); 
}

} // namespace Algorithm
} // namespace Mantid
