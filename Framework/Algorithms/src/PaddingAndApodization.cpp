// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PaddingAndApodization.h"
#include "MantidAlgorithms/ApodizationFunctions.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"

#include <cmath>
#include <numeric>
#include <vector>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace DataObjects;
using namespace HistogramData;
using API::Progress;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(PaddingAndApodization)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void PaddingAndApodization::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input),
      "The name of the input 2D workspace.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "The name of the output 2D workspace.");
  declareProperty("ApodizationFunction", "None",
                  boost::make_shared<Mantid::Kernel::StringListValidator>(
                      std::vector<std::string>{"None", "Lorentz", "Gaussian"}),
                  "The apodization function to apply to the data");
  declareProperty("DecayConstant", 1.5,
                  "The decay constant for the apodization function.");
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      "Padding", 0, mustBePositive,
      "The amount of padding to add to the data,"
      "it is the number of multiples of the data set."
      "i.e 0 means no padding and 1 will double the number of data points.");
  declareProperty("NegativePadding", false,
                  "If true padding is added to "
                  "both sides of the original data. Both sides "
                  "share the padding");
}

/** Executes the algorithm
 *
 */
void PaddingAndApodization::exec() {
  // Get original workspace
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  auto numSpectra = inputWS->getNumberHistograms();
  // Create output workspace with same dimensions as input
  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (inputWS != outputWS) {
    outputWS = create<API::MatrixWorkspace>(*inputWS);
  }

  // Share the X values
  for (size_t i = 0; i < static_cast<size_t>(numSpectra); ++i) {
    outputWS->setSharedX(i, inputWS->sharedX(i));
  }

  std::vector<int> spectra;
  spectra = std::vector<int>(numSpectra);
  std::iota(spectra.begin(), spectra.end(), 0);

  Progress prog(this, 0.0, 1.0, numSpectra + spectra.size());
  if (inputWS != outputWS) {

    // Copy all the Y and E data
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
    for (int64_t i = 0; i < int64_t(numSpectra); ++i) {
      PARALLEL_START_INTERUPT_REGION

      if (std::find(spectra.begin(), spectra.end(), i) != spectra.end()) {
        const auto index = static_cast<size_t>(i);
        outputWS->setSharedY(index, inputWS->sharedY(index));
        outputWS->setSharedE(index, inputWS->sharedE(index));
      }
      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }
  const std::string method = getProperty("ApodizationFunction");
  const double decayConstant = getProperty("DecayConstant");
  const int padding = getProperty("Padding");
  fptr apodizationFunction = getApodizationFunction(method);
  // Do the specified spectra only
  int specLength = static_cast<int>(spectra.size());
  std::vector<double> norm(specLength, 0.0);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int i = 0; i < specLength; ++i) {
    PARALLEL_START_INTERUPT_REGION
    const auto specNum = static_cast<size_t>(spectra[i]);

    if (spectra[i] > static_cast<int>(numSpectra)) {
      throw std::invalid_argument("The spectral index " +
                                  std::to_string(spectra[i]) +
                                  " is greater than the number of spectra!");
    }
    // Create output ws
    outputWS->setHistogram(specNum,
                           applyApodizationFunction(
                               addPadding(inputWS->histogram(specNum), padding),
                               decayConstant, apodizationFunction));
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  setProperty("OutputWorkspace", outputWS);
}

using fptr = double (*)(const double, const double);
/**
 * Gets a pointer to the relevant
 * apodization function
 * @param method :: [input] The name of the chosen function
 * @returns :: pointer to the function
 */
fptr PaddingAndApodization::getApodizationFunction(const std::string method) {
  if (method == "None") {
    return ApodizationFunctions::none;
  } else if (method == "Lorentz") {
    return ApodizationFunctions::lorentz;
  } else if (method == "Gaussian") {
    return ApodizationFunctions::gaussian;
  }
  throw std::invalid_argument("The apodization function selected " + method +
                              " is not a valid option");
}
/**
 * Applies the appodization function to the data.
 * @param histogram :: [input] Input histogram.
 * @param function :: [input] the apodization function
 * @param decayConstant :: [input] the decay constant for apodization function
 * @returns :: Histogram of the apodized data
 */
HistogramData::Histogram PaddingAndApodization::applyApodizationFunction(
    const HistogramData::Histogram &histogram, const double decayConstant,
    fptr function) {
  HistogramData::Histogram result(histogram);

  auto &xData = result.mutableX();
  auto &yData = result.mutableY();
  auto &eData = result.mutableE();

  for (size_t i = 0; i < yData.size(); ++i) {
    yData[i] *= function(xData[i], decayConstant);
    eData[i] *= function(xData[i], decayConstant);
  }

  return result;
}
/**
 * Adds padding to the data. The padding is
 * an integer multiple of the original data set.
 * i.e. padding =0 => none
 * padding = 2 => 2/3 of the output will be zero.
 * @param histogram :: [input] Input histogram
 * @param padding :: [input] the amount of padding to add
 * @returns :: Histogram of the padded data
 */
HistogramData::Histogram
PaddingAndApodization::addPadding(const HistogramData::Histogram &histogram,
                                  const int padding) {
  if (padding == 0) {
    return histogram;
  }

  auto &xData = histogram.x();
  auto &yData = histogram.y();
  auto &eData = histogram.e();
  auto incEData = eData.size() > 0 ? true : false;
  // assume approx evenly spaced
  if (xData.size() < 2) {
    throw std::invalid_argument("The xData does not contain "
                                "enought data points to add padding"
                                "dx = 0");
  }
  const double dx = xData[1] - xData[0];
  const auto dataSize = yData.size() * (1 + padding);

  // Create histogram with the same structure as histogram
  HistogramData::Histogram result(histogram);
  // Resize result to new size.
  result.resize(dataSize);
  auto &newXData = result.mutableX();
  auto &newYData = result.mutableY();
  auto &newEData = result.mutableE();

  // Start x counter at 1 dx below the first value to make
  // the std::generate algorithm work correctly.
  double x = xData.front() - dx;
  size_t offset = 0;
  bool negativePadding = getProperty("NegativePadding");
  if (negativePadding) {
    // non-zero offset is for padding before the data
    offset = padding * yData.size() / 2;
    x -= dx * double(offset);
  }

  // This covers all x values, no need to copy from xData
  std::generate(newXData.begin(), newXData.end(), [&x, &dx] {
    x += dx;
    return x;
  });

  // Do not rely on Histogram::resize to fill the extra elements with 0s
  // Fill in all ys with 0s first
  std::fill(newYData.begin(), newYData.end(), 0.0);
  // Then copy the old yData to the appropriate positions
  std::copy(yData.begin(), yData.end(), newYData.begin() + offset);

  if (incEData) {
    // The same reasoning as for ys
    std::fill(newEData.begin(), newEData.end(), 0.0);
    std::copy(eData.begin(), eData.end(), newEData.begin() + offset);
  }

  return result;
}

} // namespace Algorithms
} // namespace Mantid
