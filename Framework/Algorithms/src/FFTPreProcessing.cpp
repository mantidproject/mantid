//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FFTPreProcessing.h"
#include "MantidAlgorithms/ApodizationFunctionHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/WorkspaceFactory.h"
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
using API::Progress;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFTPreProcessing)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void FFTPreProcessing::init() {
  declareProperty(make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the input 2D workspace.");
  declareProperty(make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
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
void FFTPreProcessing::exec() {

  // Get original workspace
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  auto numSpectra = inputWS->getNumberHistograms();
  // Create output workspace with same dimensions as input
  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (inputWS != outputWS) {
    outputWS = API::WorkspaceFactory::Instance().create(inputWS);
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

typedef double (*fptr)(const double time, const double decayConstant);
/**
* Gets a pointer to the relevant
* apodization function
* @param method :: [input] The name of the chosen function
* @returns :: pointer to the function
*/
fptr FFTPreProcessing::getApodizationFunction(const std::string method) {
  if (method == "None") {
    return none;
  } else if (method == "Lorentz") {
    return lorentz;
  } else if (method == "Gaussian") {
    return gaussian;
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
HistogramData::Histogram FFTPreProcessing::applyApodizationFunction(
    const HistogramData::Histogram &histogram, const double decayConstant,
    fptr function) {
  HistogramData::Histogram result(histogram);
  result.points();
  auto &xData = result.mutableX();
  auto &yData = result.mutableY();
  auto &eData = result.mutableE();

  for (size_t i = 0; i < yData.size(); ++i) {
    yData[i] *= function(xData[i], decayConstant);
    eData[i] *= function(xData[i], decayConstant);
  }
  result.binEdges();
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
FFTPreProcessing::addPadding(const HistogramData::Histogram &histogram,
                             const int padding) {

  HistogramData::Histogram result(histogram);
  if(padding==0){
      return histogram;
  }
  // make sure point data
  result.points();
  auto &xData = result.x();
  auto &yData = result.y();
  auto &eData = result.e();
  auto incEData = eData.size() > 0 ? true : false;
  // assume approx evenly spaced
  if (xData.size() < 2) {
    throw std::invalid_argument("The xData does not contain "
                                "enought data points to add padding"
                                "dx = 0");
  }
  double dx = xData[1] - xData[0];
  auto dataSize = yData.size();
  std::vector<double> newXData(dataSize * (1 + padding), 0.0);
  std::vector<double> newYData(dataSize * (1 + padding), 0.0);
  std::vector<double> newEData(dataSize * (1 + padding), 0.0);
  double x = xData.back();
  size_t offset = 0;
  bool negativePadding = getProperty("NegativePadding");
  if (negativePadding) {
    // non-zero offset is for padding before the data
    offset = padding * dataSize / 2;
    x = xData.front() - dx * (1. + double(offset));
  }
  std::generate(newXData.begin(), newXData.end(), [&x, &dx] {
    x += dx;
    return x;
  });
  std::copy(xData.begin(), xData.end(), newXData.begin() + offset);
  std::copy(yData.begin(), yData.end(), newYData.begin() + offset);
  if (incEData) {
    std::copy(eData.begin(), eData.end(), newEData.begin() + offset);
    result = HistogramData::Histogram(
        HistogramData::Points(newXData), HistogramData::Counts(newYData),
        HistogramData::CountStandardDeviations(newEData));

  } else {
    result = HistogramData::Histogram(HistogramData::Points(newXData),
                                      HistogramData::Counts(newYData));
  }
  result.binEdges();
  return result;
}

} // namespace Algorithm
} // namespace Mantid
