#include "MantidAlgorithms/RebinToWorkspace.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using DataObjects::EventWorkspace;

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

namespace {
bool needToRebin(const MatrixWorkspace_sptr &left,
                 const MatrixWorkspace_sptr &rght) {
  // if pointers match they are the same object
  if (left == rght)
    return false;

  // look for first non-equal x-axis between the workspaces
  const size_t numHist = left->getNumberHistograms();
  for (size_t i = 0; i < numHist; ++i) {
    if (left->getSpectrum(i).readX() != rght->getSpectrum(i).readX()) {
      return true;
    }
  }
  // everything must be the same
  return false;
}
} // namespace

/**
 * Execute the algorithm
 */
void RebinToWorkspace::exec() {
  // The input workspaces ...
  MatrixWorkspace_sptr toRebin = getProperty("WorkspaceToRebin");
  MatrixWorkspace_sptr toMatch = getProperty("WorkspaceToMatch");
  bool preserveEvents = getProperty("PreserveEvents");

  if (needToRebin(toRebin, toMatch)) {
    // TODO should copy code from Rebin to do the right thing spectrum by
    // spectrum
    if (!WorkspaceHelpers::commonBoundaries(*toMatch)) {
      throw std::runtime_error(
          "WorkspaceToMatch must have common bin boundaries in all spectra");
    }

    // First we need to create the parameter vector from the workspace with
    // which we are matching
    std::vector<double> rb_params = createRebinParameters(toMatch);

    IAlgorithm_sptr runRebin = createChildAlgorithm("Rebin");
    runRebin->setProperty<MatrixWorkspace_sptr>("InputWorkspace", toRebin);
    runRebin->setPropertyValue("OutputWorkspace", "rebin_out");
    runRebin->setProperty("params", rb_params);
    runRebin->setProperty("PreserveEvents", preserveEvents);
    runRebin->setProperty("IgnoreBinErrors", true);
    runRebin->setChildStartProgress(0.);
    runRebin->setChildStartProgress(1.);
    runRebin->executeAsChildAlg();
    MatrixWorkspace_sptr outputWS = runRebin->getProperty("OutputWorkspace");
    setProperty("OutputWorkspace", outputWS);
  } else { // don't need to rebin
    g_log.information("Workspaces have matched binning");
    MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

    bool inPlace = (toRebin == outputWS);
    bool isEvents =
        bool(boost::dynamic_pointer_cast<const EventWorkspace>(toRebin));

    if (inPlace) {
      if (isEvents && (!preserveEvents)) {
        // convert to a MatrixWorkspace
        IAlgorithm_sptr convert =
            createChildAlgorithm("ConvertToMatrixWorkspace");
        convert->setProperty<MatrixWorkspace_sptr>("InputWorkspace", toRebin);
        convert->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
        convert->setChildStartProgress(0.);
        convert->setChildStartProgress(1.);
        convert->executeAsChildAlg();
        outputWS = convert->getProperty("OutputWorkspace");
      }
      // all other cases are already handled by not doing anything
    } else {
      if (isEvents && (!preserveEvents)) {
        // convert to a MatrixWorkspace
        // convert to a MatrixWorkspace
        IAlgorithm_sptr convert =
            createChildAlgorithm("ConvertToMatrixWorkspace");
        convert->setProperty<MatrixWorkspace_sptr>("InputWorkspace", toRebin);
        convert->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
        convert->setChildStartProgress(0.);
        convert->setChildStartProgress(1.);
        convert->executeAsChildAlg();
        outputWS = convert->getProperty("OutputWorkspace");
      } else {
        outputWS = toRebin->clone();
      }
    }

    setProperty("OutputWorkspace", outputWS);
  }
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

  const auto &matchXdata = toMatch->x(0);
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

Parallel::ExecutionMode RebinToWorkspace::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  // Probably we can relax these restrictions based on particular combination
  // with storage mode of WorkspaceToRebin, but this is simple and sufficient
  // for now.
  if (storageModes.at("WorkspaceToMatch") != Parallel::StorageMode::Cloned)
    throw std::runtime_error("WorkspaceToMatch must have " +
                             Parallel::toString(Parallel::StorageMode::Cloned));
  return Parallel::getCorrespondingExecutionMode(
      storageModes.at("WorkspaceToRebin"));
}

} // namespace Algorithms
} // namespace Mantid
