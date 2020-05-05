// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/PSIBackgroundSubtraction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Muon {

namespace {
constexpr char *MINIMISER = "Levenberg-Marquardt";
constexpr double FIT_TOLERANCE = 10;
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PSIBackgroundSubtraction)

void PSIBackgroundSubtraction::init() {

  declareProperty(
      std::make_unique<WorkspaceProperty<>>(
          "InputWorkspace", "", Direction::Input, PropertyMode::Mandatory),
      "Input workspace containing the PSI bin data "
      "which the background correction will be applied to.");

  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      "MaxIterations", 500, mustBePositive,
      "Stop after this number of iterations if a good fit is not found");
}

std::map<std::string, std::string> PSIBackgroundSubtraction::validateInputs() {
  std::map<std::string, std::string> errors;

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  if (inputWS->YUnit() != "Counts") {
    errors["InputWorkspace"] = "Input Workspace should be a counts workspace.";
  }
  return errors;
}

void PSIBackgroundSubtraction::exec() {
  setRethrows(true);
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  // Caclulate and subtract background from inputWS
  calculateBackgroundUsingFit(*inputWS);
}
/**
 * Calculate the background of a PSI workspace by performing a fit, comprising
 of a FlatBackground and ExpDecayMuon, on the second half of the PSI data.
 * @param inputWorkspace ::  Input PSI workspace which is modified inplace
 */
void PSIBackgroundSubtraction::calculateBackgroundUsingFit(
    MatrixWorkspace &inputWorkspace) {
  IAlgorithm_sptr fit = setupFitAlgorithm(inputWorkspace.getName());
  auto numberOfHistograms = inputWorkspace.getNumberHistograms();
  for (size_t index = 0; index < numberOfHistograms; ++index) {
    auto &countsdata = inputWorkspace.mutableY(index);
    auto &timedata = inputWorkspace.x(index);
    auto maxtime = timedata.back();
    auto [background, fitQuality] =
        calculateBackgroundFromFit(fit, maxtime, static_cast<int>(index));
    // If fit quality is poor, do not subtract the background and instead log a
    // warning
    if (fitQuality > FIT_TOLERANCE) {
      g_log.warning()
          << "Fit quality obtained in PSIBackgroundSubtraction is poor. "
          << "Skipping background calculation for WorkspaceIndex: "
          << static_cast<int>(index) << "\n";
    } else {
      countsdata = countsdata - background;
    }
  }
}
/**
 * Setup the fit algorithm used to obtain the background from PSI data
 of a FlatBackground and ExpDecayMuon, on the second half of the PSI data.
 * @param wsName ::  Input workspace name
 * @return Initalised fitting algorithm
 */
IAlgorithm_sptr
PSIBackgroundSubtraction::setupFitAlgorithm(const std::string &wsName) {
  std::string functionstring = "name=FlatBackground,A0=0;name=ExpDecayMuon";
  IFunction_sptr func =
      FunctionFactory::Instance().createInitialized(functionstring);

  IAlgorithm_sptr fit = createChildAlgorithm("Fit");
  int maxIterations = getProperty("MaxIterations");
  fit->initialize();
  fit->setProperty("Function", func);
  fit->setProperty("MaxIterations", maxIterations);
  fit->setPropertyValue("Minimizer", MINIMISER);
  fit->setProperty("CreateOutput", false);
  fit->setProperty("InputWorkspace", wsName);
  return fit;
}
/**
 * Runs the fit algorithm used to obtain the background from PSI data.
 * @param fit ::  Reference to the child fit algorithm used to obtain the
 * background
 * @param maxTime :: The maximum value of time (x-axis), used for fit range
 * @param workspaceIndex :: Index which the fit will be performed on
 * @return A tuple of background and fit quality
 */
std::tuple<double, double> PSIBackgroundSubtraction::calculateBackgroundFromFit(
    IAlgorithm_sptr &fit, double maxTime, int workspaceIndex) {
  fit->setProperty("StartX", maxTime / 2);
  fit->setProperty("EndX", maxTime);
  fit->setProperty("WorkspaceIndex", workspaceIndex);
  fit->execute();
  IFunction_sptr func = fit->getProperty("Function");
  double flatbackground = func->getParameter("f0.A0");
  double chi2 = std::stod(fit->getPropertyValue("OutputChi2overDof"));
  return std::make_tuple(flatbackground, chi2);
}
} // namespace Muon
} // namespace Mantid
