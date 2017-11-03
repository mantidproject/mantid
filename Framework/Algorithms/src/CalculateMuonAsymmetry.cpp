//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAlgorithms/CalculateMuonAsymmetry.h"
#include "MantidAlgorithms/MuonAsymmetryHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/StartsWithValidator.h"

#include <cmath>
#include <numeric>
#include <vector>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using API::Progress;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CalculateMuonAsymmetry)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void CalculateMuonAsymmetry::init() {
  declareProperty(make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the input 2D workspace.");
  declareProperty(make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output 2D workspace.");
  std::vector<int> empty;
  declareProperty(
      Kernel::make_unique<Kernel::ArrayProperty<int>>("Spectra", empty),
      "The workspace indices to remove the exponential decay from.");
  declareProperty(
      "StartX", 0.1,
      "The lower limit for calculating the asymmetry (an X value).");
  declareProperty(
      "EndX", 15.0,
      "The upper limit for calculating the asymmetry  (an X value).");
  declareProperty(
      "FittingFunction",
      "name = GausOsc, A = 10.0, Sigma = 0.2, Frequency = 1.0, Phi = 0.0",
      "The additional fitting functions to be used.");
  declareProperty("InputDataType", "counts",
                  boost::make_shared<Mantid::Kernel::StringListValidator>(
                      std::vector<std::string>{"counts", "asymmetry"}),
                  "If the data is raw counts or asymmetry");
  std::vector<std::string> minimizerOptions =
      API::FuncMinimizerFactory::Instance().getKeys();
  Kernel::IValidator_sptr minimizerValidator =
      boost::make_shared<Kernel::StartsWithValidator>(minimizerOptions);
  declareProperty("Minimizer", "Levenberg-MarquardtMD", minimizerValidator,
                  "Minimizer to use for fitting.");
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      "MaxIterations", 500, mustBePositive->clone(),
      "Stop after this number of iterations if a good fit is not found");
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
      "NormalizationConstant", Direction::Output));
  std::vector<double> emptyDoubles;
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      "PreviousNormalizationConstant", emptyDoubles),
                  "Normalization constant used"
                  " to estimate asymmetry");
}
/*
* Validate the input parameters
* @returns map with keys corresponding to properties with errors and values
* containing the error messages.
*/
std::map<std::string, std::string> CalculateMuonAsymmetry::validateInputs() {
  // create the map
  std::map<std::string, std::string> validationOutput;
  // check start and end times
  double startX = getProperty("StartX");
  double endX = getProperty("EndX");
  if (startX > endX) {
    validationOutput["StartX"] = "Start time is after the end time.";
  } else if (startX == endX) {
    validationOutput["StartX"] = "Start and end times are equal, there is no "
                                 "data to apply the algorithm to.";
  }
  std::string dataType = getProperty("InputDataType");
  std::vector<double> oldNorm = getProperty("PreviousNormalizationConstant");
  std::vector<int> spectra = getProperty("Spectra");
  if (dataType == "asymmetry" && oldNorm.empty()) {
    validationOutput["PreviousNormalizationConstant"] =
        "Asymmetry data has been provided but"
        " no normalization constants have been provided.";
  }
  if (dataType == "asymmetry" && oldNorm.size() != spectra.size()) {

    validationOutput["PreviousNormalizationConstant"] =
        "Number of spectra and the list"
        " of normalization constants are inconsistant.";
  }
  return validationOutput;
}
/** Executes the algorithm
 *
 */

void CalculateMuonAsymmetry::exec() {
  std::vector<int> spectra = getProperty("Spectra");
  std::vector<double> oldNorm = getProperty("PreviousNormalizationConstant");
  // Get original workspace
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  auto numSpectra = inputWS->getNumberHistograms();
  // Create output workspace with same dimensions as input
  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (inputWS != outputWS) {
    outputWS = API::WorkspaceFactory::Instance().create(inputWS);
  }
  double startX = getProperty("StartX");
  double endX = getProperty("EndX");
  auto dataType = getPropertyValue("InputDataType");

  const Mantid::API::Run &run = inputWS->run();
  const double numGoodFrames = std::stod(run.getProperty("goodfrm")->value());

  g_log.warning("Assuming that the spectra and normalization constants are in "
                "the same order");

  // Share the X values
  for (size_t i = 0; i < static_cast<size_t>(numSpectra); ++i) {
    outputWS->setSharedX(i, inputWS->sharedX(i));
  }

  // No spectra specified = process all spectra
  if (spectra.empty()) {
    spectra = std::vector<int>(numSpectra);
    std::iota(spectra.begin(), spectra.end(), 0);
  }

  Progress prog(this, 0.0, 1.0, numSpectra + spectra.size());
  if (inputWS != outputWS) {

    // Copy all the Y and E data
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
    for (int64_t i = 0; i < int64_t(numSpectra); ++i) {
      PARALLEL_START_INTERUPT_REGION
      const auto index = static_cast<size_t>(i);
      outputWS->setSharedY(index, inputWS->sharedY(index));
      outputWS->setSharedE(index, inputWS->sharedE(index));
      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }

  // Do the specified spectra only
  int specLength = static_cast<int>(spectra.size());
  std::vector<double> norm(specLength, 0.0);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int i = 0; i < specLength; ++i) {
    PARALLEL_START_INTERUPT_REGION
    const auto specNum = static_cast<size_t>(spectra[i]);
    if (spectra[i] > static_cast<int>(numSpectra)) {
      g_log.error("The spectral index " + std::to_string(spectra[i]) +
                  " is greater than the number of spectra!");
      throw std::invalid_argument("The spectral index " +
                                  std::to_string(spectra[i]) +
                                  " is greater than the number of spectra!");
    }
    double estNormConst;
    if (dataType == "counts") {
      // inital estimate of N0
      estNormConst = estimateNormalisationConst(inputWS->histogram(specNum),
                                                numGoodFrames, startX, endX);
      // Calculate the normalised counts
      outputWS->setHistogram(
          specNum, normaliseCounts(inputWS->histogram(specNum), numGoodFrames));

    } else {
      estNormConst = oldNorm[i];
      outputWS->setHistogram(specNum, inputWS->histogram(specNum));
      // convert the data back to normalised counts
      outputWS->mutableY(specNum) += 1.0;
      outputWS->mutableY(specNum) *= estNormConst;
      outputWS->mutableE(specNum) *= estNormConst;
    }
    // get the normalisation constant
    const double normConst =
        getNormConstant(outputWS, spectra[i], estNormConst, startX, endX);
    // calculate the asymmetry
    outputWS->mutableY(specNum) /= normConst;
    outputWS->mutableY(specNum) -= 1.0;
    outputWS->mutableE(specNum) /= normConst;
    norm[i] = normConst;
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Update Y axis units
  outputWS->setYUnit("Asymmetry");
  setProperty("NormalizationConstant", norm);
  setProperty("OutputWorkspace", outputWS);
}

/**
 * Calculate normalisation constant after the exponential decay has been removed
 * to a linear fitting function
 * @param ws ::  workspace
 * @param wsIndex :: workspace index
 * @param estNormConstant :: estimate of normalisation constant
 * @param startX :: the smallest x value for the fit
 * @param endX :: the largest x value for the fit
 * @return normalisation constant
*/

double CalculateMuonAsymmetry::getNormConstant(API::MatrixWorkspace_sptr ws,
                                               int wsIndex,
                                               const double estNormConstant,
                                               const double startX,
                                               const double endX) {
  double retVal = 1.0;
  int maxIterations = getProperty("MaxIterations");
  auto minimizer = getProperty("Minimizer");
  API::IAlgorithm_sptr fit;
  fit = createChildAlgorithm("Fit", -1, -1, true);
  std::string tmpString = getProperty("FittingFunction");

  std::string function;
  function = "composite=ProductFunction;name=FlatBackground,A0=" +
             std::to_string(estNormConstant);
  function += ";(name=FlatBackground,A0=1.0,ties=(A0=1.0);";
  function += tmpString + ")";

  fit->setProperty("MaxIterations", maxIterations);
  fit->setPropertyValue("Function", function);
  fit->setProperty("InputWorkspace", ws);
  fit->setProperty("WorkspaceIndex", wsIndex);
  fit->setPropertyValue("Minimizer", minimizer);
  fit->setProperty("StartX", startX);
  fit->setProperty("EndX", endX);
  fit->execute();

  std::string fitStatus = fit->getProperty("OutputStatus");
  API::IFunction_sptr result = fit->getProperty("Function");
  const double A0 = result->getParameter(0);
  if (fitStatus == "success") { // to be explicit

    if (A0 < 0) {
      g_log.warning() << "When trying to fit Asymmetry normalisation constant "
                         "this constant comes out negative. For workspace "
                      << wsIndex << "."
                      << "To proceed Asym norm constant set to 1.0\n";
      retVal = 1.0;
    } else {
      retVal = A0;
    }
  } else {
    g_log.warning() << "Fit falled. Status = " << fitStatus
                    << "\nFor workspace index " << wsIndex
                    << "\nAsym norm constant set to 1.0\n";
    retVal = 1.0;
  }

  return retVal;
}

} // namespace Algorithm
} // namespace Mantid
