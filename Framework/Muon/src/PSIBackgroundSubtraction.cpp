// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/PSIBackgroundSubtraction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidKernel/BoundedValidator.h"

#include <math.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid::Muon {

namespace {
constexpr const char *MINIMISER = "Levenberg-Marquardt";
constexpr double FIT_TOLERANCE = 10;
const std::string FIRST_GOOD = "First good spectra ";
const std::string LAST_GOOD = "Last good spectra ";

std::pair<double, double> getRangeFromWorkspace(MatrixWorkspace const &inputWorkspace, const size_t &index) {
  auto firstGoodIndex = std::stoi(inputWorkspace.getLog(FIRST_GOOD + std::to_string(index))->value());
  auto lastGoodIndex = std::stoi(inputWorkspace.getLog(LAST_GOOD + std::to_string(index))->value());

  auto midGoodIndex = int(floor(((double(lastGoodIndex) - double(firstGoodIndex)) / 2.)));

  double midGood = inputWorkspace.readX(index)[midGoodIndex];

  double lastGood = inputWorkspace.readX(index)[lastGoodIndex];

  return std::make_pair(midGood, lastGood);
}

} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PSIBackgroundSubtraction)

void PSIBackgroundSubtraction::init() {

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::InOut, PropertyMode::Mandatory),
      "Input workspace containing the PSI bin data "
      "which the background correction will be applied to.");

  declareProperty(std::make_unique<API::FunctionProperty>("Function", Direction::InOut, PropertyMode::Optional),
                  "An optional fit function that will be added on top of the default FlatBackground and ExpDecayMuon "
                  "functions, before the combined function is used for the background subtraction.");

  declareProperty("StartX", EMPTY_DBL(),
                  "An X value in the first bin to be included in the calculation of the background. If this is not "
                  "provided, it will use the first X found in the InputWorkspace.");
  declareProperty("EndX", EMPTY_DBL(),
                  "An X value in the last bin to be included in the calculation of the background. If this is not "
                  "provided, it will use the last X found in the InputWorkspace.");

  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("MaxIterations", 500, mustBePositive,
                  "Stop after this number of iterations if a good fit is not found");

  auto mustBeGreater0 = std::make_shared<Kernel::BoundedValidator<int>>();
  mustBeGreater0->setLower(1);
  declareProperty("Binning", 1, mustBeGreater0, "Constant sized rebinning of the data");
}

std::map<std::string, std::string> PSIBackgroundSubtraction::validateInputs() {
  std::map<std::string, std::string> errors;

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  if (!inputWS) {
    errors["InputWorkspace"] = "Input Workspace must be Matrix workspace.";
    return errors;
  }
  if (inputWS->YUnit() != "Counts") {
    errors["InputWorkspace"] = "Input Workspace should be a counts workspace.";
  }
  const Mantid::API::Run &run = inputWS->run();
  for (size_t index = 0; index < inputWS->getNumberHistograms(); index++) {

    int firstGood = 0;
    int lastGood = 0;
    try {
      firstGood = std::stoi(run.getProperty(FIRST_GOOD + std::to_string(index))->value());
    } catch (Kernel::Exception::NotFoundError &) {
      errors["InputWorkspace"] = "Input Workspace should should contain first good data. ";
    }
    try {
      lastGood = std::stoi(run.getProperty(LAST_GOOD + std::to_string(index))->value());
    } catch (Kernel::Exception::NotFoundError &) {
      errors["InputWorkspace"] += "\n Input Workspace should should contain last good data. ";
    }
    if (lastGood <= firstGood) {
      errors["InputWorkspace"] += "\n Input Workspace should have last good data > first good data. ";
    }
    if (firstGood < 0) {
      errors["InputWorkspace"] += "\n Input Workspace should have first good data > 0. ";
    }
    if (lastGood >= int(inputWS->readX(index).size())) {
      errors["InputWorkspace"] += "\n Input Workspace should have last good data < number of bins. ";
    }
  }

  if (!isDefault("StartX") && !isDefault("EndX")) {
    const double startX = getProperty("StartX");
    const double endX = getProperty("EndX");
    if (startX > endX) {
      errors["StartX"] = "StartX must be less than EndX.";
      errors["EndX"] = "EndX must be greater than StartX.";
    }
  }
  return errors;
}

void PSIBackgroundSubtraction::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  // Caclulate and subtract background from inputWS
  calculateBackgroundUsingFit(inputWS);
}

/**
 * Calculate the background of a PSI workspace by performing a fit, comprising
 of a FlatBackground and ExpDecayMuon, on the second half of the PSI data.
 * @param inputWorkspace ::  Input PSI workspace which is modified inplace
 */
void PSIBackgroundSubtraction::calculateBackgroundUsingFit(MatrixWorkspace_sptr &inputWorkspace) {
  auto fit = setupFitAlgorithm(inputWorkspace->getName());
  auto numberOfHistograms = inputWorkspace->getNumberHistograms();
  std::vector<double> backgroundValues(numberOfHistograms);
  for (size_t index = 0; index < numberOfHistograms; ++index) {
    std::pair<double, double> range = getRange(*inputWorkspace, index);
    auto [background, fitQuality] = calculateBackgroundFromFit(fit, range, static_cast<int>(index));
    // If fit quality is poor, do not subtract the background and instead log
    // a warning
    if (fitQuality > FIT_TOLERANCE) {
      g_log.warning() << "Fit quality obtained in PSIBackgroundSubtraction is poor. "
                      << "Skipping background calculation for WorkspaceIndex: " << static_cast<int>(index) << "\n";
    } else {
      backgroundValues[index] = background;
    }
  }
  // Create background workspace
  auto wsAlg = createChildAlgorithm("CreateWorkspace", 0.7, 1.0);
  wsAlg->setProperty<std::vector<double>>("DataX", std::vector<double>(2, 0.0));
  wsAlg->setProperty<std::vector<double>>("DataY", backgroundValues);
  wsAlg->setProperty<int>("NSpec", static_cast<int>(numberOfHistograms));
  wsAlg->execute();
  MatrixWorkspace_sptr backgroundWS = wsAlg->getProperty("OutputWorkspace");
  backgroundWS->setYUnit("Counts");

  // Subtract background from inputworkspace
  auto minusAlg = createChildAlgorithm("Minus");
  minusAlg->setProperty("LHSWorkspace", inputWorkspace);
  minusAlg->setProperty("RHSWorkspace", backgroundWS);
  minusAlg->setProperty("OutputWorkspace", inputWorkspace);
  minusAlg->execute();
}
/**
 * Setup the fit algorithm used to obtain the background from PSI data
 of a FlatBackground and ExpDecayMuon, on the second half of the PSI data.
 * @param wsName ::  Input workspace name
 * @return Initalised fitting algorithm
 */
IAlgorithm_sptr PSIBackgroundSubtraction::setupFitAlgorithm(const std::string &wsName) {
  auto fit = createChildAlgorithm("Fit");
  int maxIterations = getProperty("MaxIterations");
  fit->initialize();
  fit->setProperty("Function", getFunction());
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
std::tuple<double, double> PSIBackgroundSubtraction::calculateBackgroundFromFit(IAlgorithm_sptr &fit,
                                                                                const std::pair<double, double> &range,
                                                                                const int &workspaceIndex) {
  fit->setProperty("StartX", range.first);
  fit->setProperty("EndX", range.second);
  fit->setProperty("WorkspaceIndex", workspaceIndex);
  fit->execute();
  IFunction_sptr func = fit->getProperty("Function");
  setProperty("Function", func);

  double flatbackground = func->getParameter("f0.A0");
  double chi2 = std::stod(fit->getPropertyValue("OutputChi2overDof"));
  return std::make_tuple(flatbackground, chi2);
}

/**
 * Gets the Function to use for the background subtraction.
 * @return An IFunction_sptr used in the Fit algorithm.
 */
IFunction_sptr PSIBackgroundSubtraction::getFunction() const {
  std::string funcString = "name=FlatBackground,A0=0;name=ExpDecayMuon";
  if (!getPointerToProperty("Function")->isDefault())
    funcString += ";" + getPropertyValue("Function");

  return FunctionFactory::Instance().createInitialized(funcString);
}

/**
 * Gets the X range to use for fitting to the current index in the
 * InputWorkspace. If a Start or End X is not provided, the start or end X from
 * the InputWorkspace is used instead.
 * @param inputWorkspace :: The workspace being fitted too.
 * @param index :: The workspace index the fit will be performed on.
 * @return An X range to use for the fitting.
 */
std::pair<double, double> PSIBackgroundSubtraction::getRange(MatrixWorkspace const &inputWorkspace,
                                                             const std::size_t &index) const {
  double startX = getProperty("StartX");
  double endX = getProperty("EndX");
  if (isEmpty(startX) || isEmpty(endX)) {
    const auto range = getRangeFromWorkspace(inputWorkspace, index);
    if (isEmpty(startX))
      startX = range.first;
    if (isEmpty(endX))
      endX = range.second;
  }
  return std::make_pair(startX, endX);
}

} // namespace Mantid::Muon
