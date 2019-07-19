// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateEfficiency2.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include <cmath>
#include <limits>
#include <vector>

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CalculateEfficiency2)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

/// A private namespace for property names.
namespace PropertyNames {
const static std::string INPUT_WORKSPACE{"InputWorkspace"};
const static std::string OUTPUT_WORKSPACE{"OutputWorkspace"};
const static std::string MIN_THRESHOLD{"MinThreshold"};
const static std::string MAX_THRESHOLD{"MaxThreshold"};
} // namespace PropertyNames

namespace { // anonymous
static void applyBadPixelThreshold(MatrixWorkspace &outputWS,
                                   double minThreshold, double maxThreshold) {

  // Number of spectra
  const size_t numberOfSpectra = outputWS.getNumberHistograms();
  const auto &spectrumInfo = outputWS.spectrumInfo();

  for (size_t i = 0; i < numberOfSpectra; i++) {
    auto &YOut = outputWS.mutableY(i);
    auto &EOut = outputWS.mutableE(i);

    // Skip if we have a monitor or if the detector is masked.
    if (spectrumInfo.isMonitor(i)) {
      YOut.front() = 1.0;
      EOut.front() = 0.0;
      continue;
    } else if (spectrumInfo.isMasked(i)) {
      continue;
    }

    // if the pixel is outside the thresholds let make it EMPTY_DBL
    // In the documentation is "-inf"
    const auto y = YOut.front();
    if (y < minThreshold || y > maxThreshold) {
      YOut.front() = EMPTY_DBL();
      EOut.front() = EMPTY_DBL();
    }
  }
}
} // anonymous namespace

/** Initialization method.
 *
 */
void CalculateEfficiency2::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      PropertyNames::INPUT_WORKSPACE, "", Direction::Input),
                  "The workspace containing the flood data");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_WORKSPACE, "",
                                            Direction::Output),
      "The name of the workspace to be created as the output of the algorithm");

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty(PropertyNames::MIN_THRESHOLD, 0.0, positiveDouble,
                  "Minimum threshold for a pixel to be considered");
  declareProperty(PropertyNames::MAX_THRESHOLD, 2.0, positiveDouble->clone(),
                  "Maximum threshold for a pixel to be considered");
}

std::map<std::string, std::string> CalculateEfficiency2::validateInputs() {
  std::map<std::string, std::string> result;
  // Files from time-of-flight instruments must be integrated in Lambda before
  // using this algorithm
  MatrixWorkspace_const_sptr inputWS =
      getProperty(PropertyNames::INPUT_WORKSPACE);
  if (inputWS->blocksize() > 1)
    result[PropertyNames::INPUT_WORKSPACE] =
        "Input workspace must have only one bin";

  // get the thresholds once to error check and use in the main function
  m_minThreshold = getProperty("MinThreshold");
  m_maxThreshold = getProperty("MaxThreshold");
  if (m_minThreshold >= m_maxThreshold) {
    const std::string msg{"MinThreshold must be less than MaxThreshold"};
    result[PropertyNames::MIN_THRESHOLD] = msg;
    result[PropertyNames::MAX_THRESHOLD] = msg;
  }

  return result;
}

/** Executes the algorithm
 *
 */
void CalculateEfficiency2::exec() {
  // create the output workspace from the input
  auto childAlg = createChildAlgorithm("RebinToWorkspace", 0.0, 0.1);
  childAlg->setPropertyValue("WorkspaceToRebin",
                             getPropertyValue(PropertyNames::INPUT_WORKSPACE));
  childAlg->setPropertyValue("WorkspaceToMatch",
                             getPropertyValue(PropertyNames::INPUT_WORKSPACE));
  childAlg->setPropertyValue("OutputWorkspace",
                             getPropertyValue(PropertyNames::OUTPUT_WORKSPACE));
  childAlg->setProperty("PreserveEvents", false);
  childAlg->executeAsChildAlg();
  MatrixWorkspace_sptr outputWS = childAlg->getProperty("OutputWorkspace");

  // Loop over spectra and sum all the counts to get normalization
  // Skip monitors and masked detectors
  // returns tuple with (sum, err, npixels)
  progress(0.1, "Computing the counts.");
  auto counts = sumUnmaskedAndDeadPixels(*outputWS);
  if (counts.nPixels == 0) {
    throw std::runtime_error("No pixels being used for calculation");
  }

  progress(0.3, "Normalising the detectors.");
  averageAndNormalizePixels(*outputWS, counts);

  progress(0.5, "Applying bad pixel threshold.");
  applyBadPixelThreshold(*outputWS, m_minThreshold, m_maxThreshold);

  // do it again only using the pixels that are within the threshold
  progress(0.7, "Computing the counts.");
  counts = sumUnmaskedAndDeadPixels(*outputWS);
  if (counts.nPixels == 0) {
    throw std::runtime_error("All pixels are outside of the threshold values");
  }

  progress(0.9, "Normalising the detectors.");
  averageAndNormalizePixels(*outputWS, counts);

  setProperty(PropertyNames::OUTPUT_WORKSPACE, outputWS);
  progress(1.0, "Done!");
}

/*
 *  Sum up all the unmasked detector pixels.
 *
 * @param workspace: workspace where all the wavelength bins have been grouped
 */
SummedResults CalculateEfficiency2::sumUnmaskedAndDeadPixels(
    const MatrixWorkspace &workspace) {
  // Number of spectra
  const size_t numberOfSpectra = workspace.getNumberHistograms();
  SummedResults results;

  const auto &spectrumInfo = workspace.spectrumInfo();
  for (size_t i = 0; i < numberOfSpectra; i++) {

    // Retrieve the spectrum into a vector
    auto &YValues = workspace.y(i);
    auto &YErrors = workspace.e(i);

    // Skip if we have a monitor, if the detector is masked or if the pixel is
    // dead
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i) ||
        isEmpty(YValues.front()))
      continue;

    results.sum += YValues.front();
    results.error += YErrors.front() * YErrors.front();
    results.nPixels++;
  }
  results.error = std::sqrt(results.error);

  g_log.debug() << "Total of unmasked/dead pixels = " << results.nPixels
                << " from a total of " << numberOfSpectra << "\n";

  return results;
}

void CalculateEfficiency2::averageAndNormalizePixels(
    MatrixWorkspace &workspace, const SummedResults &counts) {

  // Number of spectra
  const size_t numberOfSpectra = workspace.getNumberHistograms();
  const auto &spectrumInfo = workspace.spectrumInfo();
  // Calculate the averages
  const double averageY = counts.sum / static_cast<double>(counts.nPixels);
  const double averageE = counts.error / static_cast<double>(counts.nPixels);

  for (size_t i = 0; i < numberOfSpectra; i++) {

    auto &y = workspace.mutableY(i);
    auto &e = workspace.mutableE(i);
    const auto yOriginal = y.front();

    // Skip if we have a monitor, the detector is masked, or it has already been
    // marked as outside of the threashold by being set to EMPTY_DBL
    if (spectrumInfo.isMasked(i) || spectrumInfo.isMonitor(i) ||
        isEmpty(yOriginal))
      continue;

    const auto eOriginal = e.front();

    // Normalize counts
    y.front() = yOriginal / averageY;
    const double signalToNoiseOrig = eOriginal / yOriginal;
    const double signalToNoiseAvg = averageE / averageY;
    e.front() = y.front() * std::sqrt((signalToNoiseOrig * signalToNoiseOrig) +
                                      (signalToNoiseAvg * signalToNoiseAvg));
  }

  g_log.debug() << "Averages :: counts = " << averageY
                << "; error = " << averageE << "\n";
}

} // namespace Algorithms
} // namespace Mantid
