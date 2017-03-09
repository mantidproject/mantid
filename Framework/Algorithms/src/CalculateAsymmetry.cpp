//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AsymmetryHelper.h"
#include "MantidAlgorithms/CalculateAsymmetry.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"

#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/ArrayProperty.h"

#include <cmath>
#include <numeric>
#include <vector>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using API::Progress;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CalculateAsymmetry)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void CalculateAsymmetry::init() {
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
}

/** Executes the algorithm
 *
 */
void CalculateAsymmetry::exec() {
  std::vector<int> spectra = getProperty("Spectra");

  // Get original workspace
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  int numSpectra = static_cast<int>(inputWS->size() / inputWS->blocksize());
  // Create output workspace with same dimensions as input
  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (inputWS != outputWS) {
    outputWS = API::WorkspaceFactory::Instance().create(inputWS);
  }
  // Share the X values
  for (size_t i = 0; i < static_cast<size_t>(numSpectra); ++i) {
    outputWS->setSharedX(i, inputWS->sharedX(i));
  }

  // No spectra specified = process all spectra
  if (spectra.empty()) {
    std::vector<int> allSpectra(numSpectra);
    std::iota(allSpectra.begin(), allSpectra.end(), 0);
    spectra.swap(allSpectra);
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
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int i = 0; i < specLength; ++i) {
    PARALLEL_START_INTERUPT_REGION
    const auto specNum = static_cast<size_t>(spectra[i]);
    if (spectra[i] > numSpectra) {
      g_log.error("Spectra size greater than the number of spectra!");
      throw std::invalid_argument(
          "Spectra size greater than the number of spectra!");
    }

    // check start and end times
    double startX = getProperty("StartX");
    double endX = getProperty("EndX");

    if (startX > endX) {
      g_log.warning()
          << "Start time is after the end time. Swapping the start and end."
          << '\n';
      double tmp = endX;
      endX = startX;
      startX = tmp;
    } else if (startX == endX) {
      throw std::runtime_error("Start and end times are equal, there is no "
                               "data to apply the algorithm to.");
    }

    auto xData = inputWS->histogram(specNum).binEdges();
    if (startX < xData[0]) {
      g_log.warning() << "Start time is before the first data point. Using "
                         "first data point." << '\n';
    }
    if (endX > xData[xData.size() - 1]) {
      g_log.warning()
          << "End time is after the last data point. Using last data point."
          << '\n';
      g_log.warning() << "Data at late times may dominate the normalisation."
                      << '\n';
    }

    const Mantid::API::Run &run = inputWS->run();
    const double numGoodFrames = std::stod(run.getProperty("goodfrm")->value());
    // inital estimate of N0
    const double estNormConst = estimateNormalisationConst(
        inputWS->histogram(specNum), numGoodFrames, startX, endX);
    // Calculate the normalised counts
    outputWS->setHistogram(
        specNum, normaliseCounts(inputWS->histogram(specNum), numGoodFrames));
    // get the normalisation constant
    const double normConst =
        getNormConstant(outputWS, spectra[i], estNormConst, startX, endX);
    // calculate the asymmetry
    outputWS->mutableY(specNum) /= normConst;
    outputWS->mutableY(specNum) -= 1.0;
    outputWS->mutableE(specNum) /= normConst;

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Update Y axis units
  outputWS->setYUnit("Asymmetry");

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
double CalculateAsymmetry::getNormConstant(API::MatrixWorkspace_sptr ws,
                                           int wsIndex,
                                           const double estNormConstant,
                                           const double startX,
                                           const double endX) {
  double retVal = 1.0;

  API::IAlgorithm_sptr fit;
  fit = createChildAlgorithm("Fit", -1, -1, true);
  std::string tmpString = getProperty("FittingFunction");
  if (tmpString == "") {
    g_log.warning(
        "There is no additional function defined. Using original estimate");
    return estNormConstant;
  }
  std::stringstream ss;

  ss << "composite=ProductFunction;name=FlatBackground,A0=" << estNormConstant;
  ss << ";(";

  ss << "name=FlatBackground,A0=1.0"
     << ",ties=(A0=1.0);";

  ss << tmpString;
  ss << ")";
  std::string function = ss.str();
  fit->setPropertyValue("Function", function);
  fit->setProperty("InputWorkspace", ws);
  fit->setProperty("WorkspaceIndex", wsIndex);
  fit->setPropertyValue("Minimizer", "Levenberg-MarquardtMD");
  fit->setProperty("StartX", startX);
  fit->setProperty("EndX", endX);
  fit->execute();

  std::string fitStatus = fit->getProperty("OutputStatus");
  API::IFunction_sptr result = fit->getProperty("Function");
  std::vector<std::string> paramnames = result->getParameterNames();
  // Check order of names
  if (paramnames[0].compare("f0.A0") != 0) {
    g_log.error() << "Parameter 0 should be f0.A0, but is " << paramnames[0]
                  << '\n';
    throw std::invalid_argument(
        "Parameters are out of order @ 0, should be f0.A0");
  }

  if (!fitStatus.compare("success")) {
    const double A0 = result->getParameter(0);

    if (A0 < 0) {
      g_log.warning() << "When trying to fit Asymmetry normalisation constant "
                         "this constant comes out negative."
                      << "To proceed Asym norm constant set to 1.0\n";
    } else {
      retVal = A0;
    }
  } else {
    g_log.warning() << "Fit falled. Status = " << fitStatus
                    << "\nFor workspace index " << wsIndex
                    << "\nAsym norm constant set to 1.0\n";
  }

  return retVal;
}

} // namespace Algorithm
} // namespace Mantid
