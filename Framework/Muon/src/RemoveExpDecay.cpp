// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMuon/RemoveExpDecay.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PhysicalConstants.h"

#include <cmath>
#include <numeric>
#include <vector>

namespace {
/// Number of microseconds in one second (10^6)
constexpr double MICROSECONDS_PER_SECOND{1000000.0};
/// Muon lifetime in microseconds
constexpr double MUON_LIFETIME_MICROSECONDS{
    Mantid::PhysicalConstants::MuonLifetime * MICROSECONDS_PER_SECOND};
} // namespace

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using API::Progress;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(MuonRemoveExpDecay)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void MuonRemoveExpDecay::init() {
  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the input 2D workspace.");
  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output 2D workspace.");
  std::vector<int> empty;
  declareProperty(
      std::make_unique<Kernel::ArrayProperty<int>>("Spectra",
                                                      std::move(empty)),
      "The workspace indices to remove the exponential decay from.");
}

/** Executes the algorithm
 *
 */
void MuonRemoveExpDecay::exec() {
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
    auto emptySpectrum =
        std::all_of(inputWS->y(specNum).begin(), inputWS->y(specNum).end(),
                    [](double value) { return value == 0.; });
    if (emptySpectrum) {
      // if the y values are all zero do not change them
      m_log.warning("Dead detector found at spectrum number " +
                    std::to_string(specNum));
      outputWS->setHistogram(specNum, inputWS->histogram(specNum));
    } else {
      // Remove decay from Y and E
      outputWS->setHistogram(specNum, removeDecay(inputWS->histogram(specNum)));

      // do scaling and subtract 1
      const double normConst = calNormalisationConst(outputWS, spectra[i]);
      outputWS->mutableY(specNum) /= normConst;
      outputWS->mutableY(specNum) -= 1.0;
      outputWS->mutableE(specNum) /= normConst;
    }

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Update Y axis units
  outputWS->setYUnit("Asymmetry");

  setProperty("OutputWorkspace", outputWS);
}

/**
 * Corrects the data and errors for one spectrum.
 * The muon lifetime is in microseconds, not seconds, because the data is in
 * microseconds.
 * @param histogram :: [input] Input histogram
 * @returns :: Histogram with exponential removed from Y and E
 */
HistogramData::Histogram MuonRemoveExpDecay::removeDecay(
    const HistogramData::Histogram &histogram) const {
  HistogramData::Histogram result(histogram);

  auto &yData = result.mutableY();
  auto &eData = result.mutableE();
  for (size_t i = 0; i < yData.size(); ++i) {
    const double factor = exp(result.x()[i] / MUON_LIFETIME_MICROSECONDS);
    // Correct the Y data
    if (yData[i] != 0.0) {
      yData[i] *= factor;
    } else {
      yData[i] = 0.1 * factor;
    }

    // Correct the E data
    if (eData[i] != 0.0) {
      eData[i] *= factor;
    } else {
      eData[i] = factor;
    }
  }

  return result;
}

/**
 * calculate normalisation constant after the exponential decay has been removed
 * to a linear fitting function
 * @param ws ::  workspace
 * @param wsIndex :: workspace index
 * @return normalisation constant
 */
double MuonRemoveExpDecay::calNormalisationConst(API::MatrixWorkspace_sptr ws,
                                                 int wsIndex) {
  double retVal = 1.0;

  API::IAlgorithm_sptr fit;
  fit = createChildAlgorithm("Fit", -1, -1, true);

  std::stringstream ss;
  ss << "name=LinearBackground,A0=" << ws->y(wsIndex)[0] << ",A1=" << 0.0
     << ",ties=(A1=0.0)";
  std::string function = ss.str();

  fit->setPropertyValue("Function", function);
  fit->setProperty("InputWorkspace", ws);
  fit->setProperty("WorkspaceIndex", wsIndex);
  fit->setPropertyValue("Minimizer", "Levenberg-MarquardtMD");
  fit->setProperty("Ties", "A1=0.0");
  fit->execute();

  std::string fitStatus = fit->getProperty("OutputStatus");
  API::IFunction_sptr result = fit->getProperty("Function");
  std::vector<std::string> paramnames = result->getParameterNames();

  // Check order of names
  if (paramnames[0] != "A0") {
    g_log.error() << "Parameter 0 should be A0, but is " << paramnames[0]
                  << '\n';
    throw std::invalid_argument(
        "Parameters are out of order @ 0, should be A0");
  }
  if (paramnames[1] != "A1") {
    g_log.error() << "Parameter 1 should be A1, but is " << paramnames[1]
                  << '\n';
    throw std::invalid_argument(
        "Parameters are out of order @ 0, should be A1");
  }

  if (fitStatus == "success") {
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

} // namespace Algorithms
} // namespace Mantid
