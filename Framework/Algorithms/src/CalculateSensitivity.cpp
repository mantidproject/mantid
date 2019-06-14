// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateSensitivity.h"
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
#include <vector>

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CalculateSensitivity)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

/** Initialization method.
 *
 */
void CalculateSensitivity::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "The workspace containing the flood data");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                            Direction::Output),
      "The name of the workspace to be created as the output of the algorithm");

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty(
      "MinThreshold", 0.0, positiveDouble,
      "Minimum threshold for a pixel to be considered (default: no minimum).");
  declareProperty(
      "MaxThreshold", 2.0, positiveDouble,
      "Maximum threshold for a pixel to be considered (default: no maximum).");
}

/** Executes the algorithm
 *
 */
void CalculateSensitivity::exec() {

  // Minimum efficiency. Pixels with lower efficiency will be masked
  double minThreshold = getProperty("MinThreshold");
  // Maximum efficiency. Pixels with higher efficiency will be masked
  double maxThreshold = getProperty("MaxThreshold");

  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Files from EQSANS must be integrated in Lambda before using this algorithm
  assert(inputWS->blocksize() == 1); // Sanity check

  MatrixWorkspace_sptr outputWS = inputWS->clone();
  setProperty("OutputWorkspace", outputWS);

  // Loop over spectra and sum all the counts to get normalization
  // Skip monitors and masked detectors
  // returns tuple with (sum, err, npixels)
  progress(0.1, "Computing the counts.");
  auto counts = sumUnmaskedAndDeadPixels(outputWS);

  progress(0.3, "Normalising the detectors.");
  averageAndNormalizePixels(outputWS, counts);

  progress(0.5, "Applying bad pixel threshold.");
  applyBadPixelThreshold(outputWS, minThreshold, maxThreshold);

  progress(0.7, "Computing the counts.");
  counts = sumUnmaskedAndDeadPixels(outputWS);

  progress(0.9, "Normalising the detectors.");
  averageAndNormalizePixels(outputWS, counts);
}

/*
 *  Sum up all the unmasked detector pixels.
 *
 * @param workspace: workspace where all the wavelength bins have been grouped
 */
std::tuple<double, double, int>
CalculateSensitivity::sumUnmaskedAndDeadPixels(MatrixWorkspace_sptr workspace) {
  // Number of spectra
  const int numberOfSpectra =
      static_cast<int>(workspace->getNumberHistograms());
  double sum = 0.0;
  double error = 0.0;
  int nPixels = 0;

  const auto &spectrumInfo = workspace->spectrumInfo();
  for (int i = 0; i < numberOfSpectra; i++) {

    // Retrieve the spectrum into a vector
    auto &YValues = workspace->y(i);
    auto &YErrors = workspace->e(i);

    // Skip if we have a monitor, if the detector is masked or if the pixel is
    // dead
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i) ||
        isEmpty(YValues[0]))
      continue;

    sum += YValues[0];
    error += YErrors[0] * YErrors[0];
    nPixels++;
  }
  error = std::sqrt(error);

  g_log.debug() << "Total of unmasked/dead pixels = " << nPixels
                << " from a total of " << numberOfSpectra << "\n";

  return std::make_tuple(sum, error, nPixels);
}

void CalculateSensitivity::averageAndNormalizePixels(
    MatrixWorkspace_sptr workspace, std::tuple<double, double, int> counts) {

  // Number of spectra
  const size_t numberOfSpectra = workspace->getNumberHistograms();
  const auto &spectrumInfo = workspace->spectrumInfo();
  // Unpack the counts variable
  auto sum = std::get<0>(counts);
  auto error = std::get<1>(counts);
  auto nPixels = std::get<2>(counts);
  // Calculate the averages
  double averageY = sum / nPixels;
  double averageE = error / nPixels;

  for (size_t i = 0; i < numberOfSpectra; i++) {

    auto &y = workspace->mutableY(i);
    auto &e = workspace->mutableE(i);

    // Skip if we have a monitor or if the detector is masked.
    if (spectrumInfo.isMasked(i) || isEmpty(y[0]))
      continue;

    auto yOriginal = y[0];
    auto eOriginal = e[0];

    // If this detector is a monitor, skip to the next one
    if (spectrumInfo.isMonitor(i)) {
      y[0] = 1.0;
      e[0] = 0.0;
      continue;
    }
    // Normalize counts
    y[0] = yOriginal / averageY;
    e[0] = y[0] * std::sqrt(std::pow(eOriginal / yOriginal, 2) +
                            std::pow(averageE / averageY, 2));
  }

  g_log.debug() << "Averages :: counts = " << averageY
                << "; error = " << averageE << "\n";
}

void CalculateSensitivity::applyBadPixelThreshold(MatrixWorkspace_sptr outputWS,
                                                  double minThreshold,
                                                  double maxThreshold) {

  // Number of spectra
  const size_t numberOfSpectra = outputWS->getNumberHistograms();
  const auto &spectrumInfo = outputWS->spectrumInfo();

  for (size_t i = 0; i < numberOfSpectra; i++) {

    // Skip if we have a monitor or if the detector is masked.
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
      continue;

    auto &YOut = outputWS->mutableY(i);
    auto &EOut = outputWS->mutableE(i);
    // if the pixel is outside the thresholds let make it EMPTY_DBL
    // In the documentation is "-inf"
    auto y = YOut[0];
    if (y < minThreshold || y > maxThreshold) {
      YOut[0] = EMPTY_DBL();
      EOut[0] = EMPTY_DBL();
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
