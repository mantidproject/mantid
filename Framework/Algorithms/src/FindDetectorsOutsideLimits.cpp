// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FindDetectorsOutsideLimits.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MultiThreaded.h"

#include <cmath>
#include <fstream>

namespace Mantid {
namespace Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(FindDetectorsOutsideLimits)

const std::string FindDetectorsOutsideLimits::category() const {
  return "Diagnostics";
}

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using Geometry::IDetector_const_sptr;

/// Initialisation method.
void FindDetectorsOutsideLimits::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the input workspace2D");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "Each histogram from the input workspace maps to a histogram in this\n"
      "workspace with one value that indicates if there was a dead detector");
  declareProperty(
      "HighThreshold", EMPTY_DBL(),
      "Spectra whose total number of counts are equal to or above this value\n"
      "will be marked bad (default off)");
  declareProperty(
      "LowThreshold", 0.0,
      "Spectra whose total number of counts are equal to or below this value\n"
      "will be marked bad (default 0)");
  auto mustBePosInt = boost::make_shared<BoundedValidator<int>>();
  mustBePosInt->setLower(0);
  declareProperty(
      "StartWorkspaceIndex", 0, mustBePosInt,
      "The index number of the first spectrum to include in the calculation\n"
      "(default 0)");
  declareProperty(
      "EndWorkspaceIndex", EMPTY_INT(), mustBePosInt,
      "The index number of the last spectrum to include in the calculation\n"
      "(default the last histogram)");
  declareProperty(
      "RangeLower", EMPTY_DBL(),
      "No bin with a boundary at an x value less than this will be used\n"
      "in the summation that decides if a detector is 'bad' (default: the\n"
      "start of each histogram)");
  declareProperty(
      "RangeUpper", EMPTY_DBL(),
      "No bin with a boundary at an x value higher than this value will\n"
      "be used in the summation that decides if a detector is 'bad'\n"
      "(default: the end of each histogram)");
  declareProperty("NumberOfFailures", 0, Direction::Output);
}

/**
 * Executes the algorithm
 *  @throw runtime_error Thrown if the algorithm cannot execute
 *  @throw invalid_argument is the LowThreshold property is greater than
 * HighThreshold
 */
void FindDetectorsOutsideLimits::exec() {
  double lowThreshold = getProperty("LowThreshold");
  double highThreshold = getProperty("HighThreshold");
  bool useHighThreshold = !isEmpty(highThreshold);
  if (useHighThreshold && highThreshold <= lowThreshold) {
    throw std::invalid_argument(
        "The high threshold must be higher than the low threshold");
  }

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  int minIndex = getProperty("StartWorkspaceIndex");
  int maxIndex = getProperty("EndWorkspaceIndex");
  const int inputLength = static_cast<int>(inputWS->getNumberHistograms());
  if (isEmpty(maxIndex))
    maxIndex = inputLength - 1;
  if (maxIndex < minIndex) {
    std::ostringstream os;
    os << "Invalid workspace indices. Min=" << minIndex << ",Max=" << maxIndex;
    throw std::invalid_argument(os.str());
  }

  if (minIndex > inputLength || maxIndex > inputLength) {
    std::ostringstream os;
    os << "Workspace indices must be lower than workspace size. Size="
       << inputLength << ", Min=" << minIndex << ",Max=" << maxIndex;
    throw std::invalid_argument(os.str());
  }

  const double rangeLower = getProperty("RangeLower");
  const double rangeUpper = getProperty("RangeUpper");
  // Get the integrated input workspace; converting to a Workspace2D
  MatrixWorkspace_sptr countsWS = integrateSpectra(
      inputWS, minIndex, maxIndex, rangeLower, rangeUpper, true);
  if (boost::dynamic_pointer_cast<EventWorkspace>(countsWS))
    throw std::runtime_error("Error! Integration output is not a Workspace2D.");

  // Create a new workspace for the results, copy from the input to ensure that
  // we copy over the instrument and current masking
  MaskWorkspace_sptr outputWS = this->generateEmptyMask(inputWS);

  const double deadValue(1.0); // delete the data

  const int diagLength = maxIndex - minIndex;
  const int progStep = static_cast<int>(std::ceil(diagLength / 100.0));

  bool checkForMask = false;
  Geometry::Instrument_const_sptr instrument = inputWS->getInstrument();
  if (instrument != nullptr) {
    checkForMask = ((instrument->getSource() != nullptr) &&
                    (instrument->getSample() != nullptr));
  }

  int numFailed(0);
  const auto &spectrumInfo = countsWS->spectrumInfo();
#pragma omp parallel for if (countsWS->threadSafe() &&                         \
                                                  outputWS->threadSafe()),     \
                                                  reduction(+ : numFailed)
  for (int i = minIndex; i <= maxIndex; ++i) {
    bool keepData = true;
    int countsInd = i - minIndex;
    if (i % progStep == 0) {
      progress(static_cast<double>(countsInd) / diagLength);
      interruption_point();
    }

    if (checkForMask && spectrumInfo.hasDetectors(countsInd)) {
      if (spectrumInfo.isMonitor(countsInd))
        continue; // do include or exclude from mask
      if (spectrumInfo.isMasked(countsInd))
        keepData = false;
    }

    const double &yValue = countsWS->y(countsInd)[0];
    // Mask out NaN and infinite
    if (!std::isfinite(yValue)) {
      keepData = false;
    } else {
      if (yValue <= lowThreshold) {
        keepData = false;
      }

      if (useHighThreshold && (yValue >= highThreshold)) {
        keepData = false;
      }
    }

    if (!keepData) {
      outputWS->mutableY(i)[0] = deadValue;
      ++numFailed;
    }
  }

  g_log.information() << numFailed
                      << " spectra fell outside the given limits.\n";
  setProperty("NumberOfFailures", numFailed);
  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWS);
}
} // namespace Algorithms
} // namespace Mantid
