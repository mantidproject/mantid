//--------------------------------
// Includes
//------------------------------
#include "MantidAlgorithms/RebinToWorkspace.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RebinToWorkspace)

//---------------------------
// Private Methods
//---------------------------
/**
 * Initialise the algorithm
 */
void RebinToWorkspace::init() {
  //  using namespace Mantid::DataObjects;
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "WorkspaceToRebin", "", Kernel::Direction::Input,
                      Kernel::make_unique<API::HistogramValidator>()),
                  "The workspace on which to perform the algorithm "
                  "This must be a Histogram workspace, not Point data. "
                  "If this is a problem try ConvertToHistogram.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "WorkspaceToMatch", "", Kernel::Direction::Input,
                      Kernel::make_unique<API::HistogramValidator>()),
                  "The workspace to match the bin boundaries against. "
                  "This must be a Histogram workspace, not Point data. "
                  "If this is a problem try ConvertToHistogram.");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                               Kernel::Direction::Output),
      "The name of the workspace to be created as the output of the algorithm");
  declareProperty("PreserveEvents", true,
                  "Keep the output workspace as an EventWorkspace, if the "
                  "input has events (default). "
                  "If the input and output EventWorkspace names are the same, "
                  "only the X bins are set, which is very quick. "
                  "If false, then the workspace gets converted to a "
                  "Workspace2D histogram.");
}

/**
 * Execute the algorithm
 */
void RebinToWorkspace::exec() {
  // The input workspaces ...
  MatrixWorkspace_sptr toRebin = getProperty("WorkspaceToRebin");
  MatrixWorkspace_sptr toMatch = getProperty("WorkspaceToMatch");
  bool PreserveEvents = getProperty("PreserveEvents");

  // First we need to create the parameter vector from the workspace with which
  // we are matching
  std::vector<double> rb_params = createRebinParameters(toMatch);

  IAlgorithm_sptr runRebin = createChildAlgorithm("Rebin");
  runRebin->setProperty<MatrixWorkspace_sptr>("InputWorkspace", toRebin);
  runRebin->setPropertyValue("OutputWorkspace", "rebin_out");
  runRebin->setProperty("params", rb_params);
  runRebin->setProperty("PreserveEvents", PreserveEvents);
  runRebin->setProperty("IgnoreBinErrors", true);
  runRebin->executeAsChildAlg();
  progress(1);
  MatrixWorkspace_sptr ws = runRebin->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", ws);
}

/**
 * Create the vector of rebin parameters
 * @param toMatch :: A shared pointer to the workspace with the desired binning
 * @returns :: A vector to hold the rebin parameters once they have been
 * calculated
 */
std::vector<double> RebinToWorkspace::createRebinParameters(
    Mantid::API::MatrixWorkspace_sptr toMatch) {
  using namespace Mantid::API;

  auto &matchXdata = toMatch->x(0);
  // params vector should have the form [x_1, delta_1,x_2, ...
  // ,x_n-1,delta_n-1,x_n), see Rebin.cpp
  std::vector<double> rb_params;
  int xsize = static_cast<int>(matchXdata.size());
  rb_params.reserve(xsize * 2);
  for (int i = 0; i < xsize; ++i) {
    // bin bound
    rb_params.push_back(matchXdata[i]);
    // Bin width
    if (i < xsize - 1)
      rb_params.push_back(matchXdata[i + 1] - matchXdata[i]);
  }
  return rb_params;
}
