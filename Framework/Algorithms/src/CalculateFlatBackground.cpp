// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateFlatBackground.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VectorHelper.h"

#include <algorithm>
#include <climits>
#include <numeric>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateFlatBackground)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// Enumeration for the different operating modes.
enum class Modes { LINEAR_FIT, MEAN, MOVING_AVERAGE };

void CalculateFlatBackground::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                            Direction::Input),
      "The input workspace must either have constant width bins or is a "
      "distribution\n"
      "workspace. It is also assumed that all spectra have the same X bin "
      "boundaries");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Name to use for the output workspace.");
  declareProperty("StartX", Mantid::EMPTY_DBL(),
                  "The X value at which to start the background fit. Mandatory "
                  "for the Linear Fit and Mean modes, ignored by Moving "
                  "Average.");
  setPropertySettings("StartX", std::make_unique<EnabledWhenProperty>(
                                    "Mode", IS_NOT_EQUAL_TO, "Moving Average"));
  declareProperty("EndX", Mantid::EMPTY_DBL(),
                  "The X value at which to end the background fit. Mandatory "
                  "for the Linear Fit and Mean modes, ignored by Moving "
                  "Average.");
  setPropertySettings("EndX", std::make_unique<EnabledWhenProperty>(
                                  "Mode", IS_NOT_EQUAL_TO, "Moving Average"));
  declareProperty(
      std::make_unique<ArrayProperty<int>>("WorkspaceIndexList"),
      "Indices of the spectra that will have their background removed\n"
      "default: modify all spectra");
  std::vector<std::string> modeOptions{"Linear Fit", "Mean", "Moving Average"};
  declareProperty("Mode", "Linear Fit",
                  boost::make_shared<StringListValidator>(modeOptions),
                  "The background count rate is estimated either by taking a "
                  "mean, doing a linear fit, or taking the\n"
                  "minimum of a moving average (default: Linear Fit)");
  // Property to determine whether we subtract the background or just return the
  // background.
  std::vector<std::string> outputOptions{"Subtract Background",
                                         "Return Background"};
  declareProperty("OutputMode", "Subtract Background",
                  boost::make_shared<StringListValidator>(outputOptions),
                  "Once the background has been determined it can either be "
                  "subtracted from \n"
                  "the InputWorkspace and returned or just returned (default: "
                  "Subtract Background)");
  declareProperty("SkipMonitors", false,
                  "By default, the algorithm calculates and removes background "
                  "from monitors in the same way as from normal detectors\n"
                  "If this property is set to true, background is not "
                  "calculated/removed from monitors.",
                  Direction::Input);
  declareProperty("NullifyNegativeValues", true,
                  "When background is subtracted, signals in some time "
                  "channels may become negative.\n"
                  "If this option is true, signal in such bins is nullified "
                  "and the module of the removed signal"
                  "is added to the error. If false, the signal and errors are "
                  "left unchanged",
                  Direction::Input);
  declareProperty("AveragingWindowWidth", Mantid::EMPTY_INT(),
                  "The width of the moving average window in bins. Mandatory "
                  "for the Moving Average mode.");
  setPropertySettings("AveragingWindowWidth",
                      std::make_unique<EnabledWhenProperty>("Mode", IS_EQUAL_TO,
                                                            "Moving Average"));
}

void CalculateFlatBackground::exec() {
  // Retrieve the input workspace
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Copy over all the data
  const size_t numHists = inputWS->getNumberHistograms();
  const size_t blocksize = inputWS->blocksize();

  if (blocksize == 1)
    throw std::runtime_error("CalculateFlatBackground with only one bin is "
                             "not possible.");

  const bool skipMonitors = getProperty("SkipMonitors");
  const bool nullifyNegative = getProperty("NullifyNegativeValues");
  const std::string modeString = getProperty("Mode");
  Modes mode = Modes::LINEAR_FIT;
  if (modeString == "Mean") {
    mode = Modes::MEAN;
  } else if (modeString == "Moving Average") {
    mode = Modes::MOVING_AVERAGE;
  }
  double startX, endX;
  int windowWidth = 0;
  switch (mode) {
  case Modes::LINEAR_FIT:
  case Modes::MEAN:
    if (getPointerToProperty("StartX")->isDefault()) {
      throw std::runtime_error("StartX property not set to any value");
    }
    if (getPointerToProperty("EndX")->isDefault()) {
      throw std::runtime_error("EndX property not set to any value");
    }
    // Get the required X range
    checkRange(startX, endX);
    break;
  case Modes::MOVING_AVERAGE:
    if (getPointerToProperty("AveragingWindowWidth")->isDefault()) {
      throw std::runtime_error(
          "AveragingWindowWidth property not set to any value");
    }
    windowWidth = getProperty("AveragingWindowWidth");
    if (windowWidth <= 0) {
      throw std::runtime_error("AveragingWindowWidth zero or negative");
    }
    if (blocksize < static_cast<size_t>(windowWidth)) {
      throw std::runtime_error("AveragingWindowWidth is larger than the number "
                               "of bins in InputWorkspace");
    }
    break;
  }

  std::vector<int> wsInds = getProperty("WorkspaceIndexList");
  // check if the user passed an empty list, if so all of spec will be processed
  if (wsInds.empty()) {
    wsInds.resize(numHists);
    std::iota(wsInds.begin(), wsInds.end(), 0);
  }

  // Are we removing the background?
  const bool removeBackground =
      std::string(getProperty("outputMode")) == "Subtract Background";

  // Initialize the progress reporting object
  m_progress = std::make_unique<Progress>(this, 0.0, 0.2, numHists);

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (outputWS != inputWS) {
    outputWS = create<MatrixWorkspace>(*inputWS);
  }

  // For logging purposes.
  double backgroundTotal = 0;
  size_t calculationCount = 0;

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int64_t i = 0; i < static_cast<int64_t>(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Extract a histogram from inputWS, work on it and, finally, put it to
    // outputWS.
    auto histogram = inputWS->histogram(i);
    bool wasCounts = false;
    if (histogram.yMode() == HistogramData::Histogram::YMode::Counts) {
      wasCounts = true;
      histogram.convertToFrequencies();
    }
    bool skipCalculation =
        std::find(wsInds.cbegin(), wsInds.cend(), i) == wsInds.cend();
    if (!skipCalculation && skipMonitors) {
      const auto &spectrumInfo = inputWS->spectrumInfo();
      if (!spectrumInfo.hasDetectors(i)) {
        // Do nothing.
        // not every spectra is the monitor or detector, some spectra have no
        // instrument components attached.
        g_log.information(
            " Can not find detector for spectra N: " + std::to_string(i) +
            " Processing background anyway\n");
      } else if (spectrumInfo.isMonitor(i)) {
        skipCalculation = true;
      }
    }
    double background = 0;
    double variance = 0;
    if (!skipCalculation) {
      ++calculationCount;
      // Now call the function the user selected to calculate the background
      switch (mode) {
      case Modes::LINEAR_FIT:
        LinearFit(histogram, background, variance, startX, endX);
        break;
      case Modes::MEAN:
        Mean(histogram, background, variance, startX, endX);
        break;
      case Modes::MOVING_AVERAGE:
        MovingAverage(histogram, background, variance, windowWidth);
        break;
      }
    }
    if (background < 0) {
      g_log.debug() << "The background for spectra index " << i
                    << "was calculated to be " << background << '\n';
      g_log.warning() << "Problem with calculating the background number of "
                         "counts spectrum with index "
                      << i << ".";
      if (removeBackground) {
        g_log.warning() << " The spectrum has been left unchanged.\n";
      } else {
        g_log.warning() << " The output background has been set to zero.\n";
      }
    } else {
      backgroundTotal += background;
    }
    HistogramData::Histogram outHistogram(histogram);
    auto &ys = outHistogram.mutableY();
    auto &es = outHistogram.mutableE();
    if (removeBackground) {
      // When subtracting backgrounds, act only if background is positive.
      if (background >= 0) {
        for (size_t j = 0; j < ys.size(); ++j) {
          double val = ys[j] - background;
          double err = std::sqrt(es[j] * es[j] + variance);
          if (nullifyNegative && (val < 0)) {
            val = 0;
            // The error estimate must go up in this nonideal situation and the
            // value of background is a good estimate for it. However, don't
            // reduce the error if it was already more than that
            err = es[j] > background ? es[j] : background;
          }
          ys[j] = val;
          es[j] = err;
        }
      }
    } else {
      for (size_t j = 0; j < ys.size(); ++j) {
        const double originalVal = histogram.y()[j];
        if (background < 0) {
          ys[j] = 0;
          es[j] = 0;
        } else if (nullifyNegative && (background > originalVal)) {
          ys[j] = originalVal;
          es[j] = es[j] > background ? es[j] : background;
        } else {
          ys[j] = background;
          es[j] = std::sqrt(variance);
        }
      }
    }
    if (wasCounts) {
      outHistogram.convertToCounts();
    }
    outputWS->setHistogram(i, outHistogram);
    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  g_log.debug() << calculationCount << " spectra corrected\n";
  g_log.information() << "The mean background was "
                      << backgroundTotal / static_cast<double>(calculationCount)
                      << ".\n";
  // Assign the output workspace to its property
  setProperty("OutputWorkspace", outputWS);
}

/**
 * Checks that the range parameters have been set correctly.
 * @param startX the starting point
 * @param endX the ending point
 * @throw std::invalid_argument if XMin or XMax are not set, or XMax is less
 * than XMin
 */
void CalculateFlatBackground::checkRange(double &startX, double &endX) {
  // use the overloaded operator =() to get the X value stored in each property
  startX = getProperty("StartX");
  endX = getProperty("EndX");

  if (startX > endX) {
    const std::string failure("XMax must be greater than XMin.");
    g_log.error(failure);
    throw std::invalid_argument(failure);
  }
}

/**
 * Gets the mean number of counts in each bin the background region and the
 * variance (error^2) of that number.
 * @param histogram the histogram to operate on
 * @param background an output variable for the calculated background
 * @param variance an output variable for background's variance.
 * @param startX an X-value in the first bin that will be considered, must not
 * be greater endX
 * @param endX an X-value in the last bin that will be considered, must not
 * less than startX
 * @throw out_of_range if either startX or endX are out of the range of X-values
 * in the specified spectrum
 * @throw invalid_argument if endX has the value of first X-value one of the
 * spectra
 */
void CalculateFlatBackground::Mean(const HistogramData::Histogram &histogram,
                                   double &background, double &variance,
                                   const double startX,
                                   const double endX) const {
  const auto &XS = histogram.x();
  const auto &YS = histogram.y();
  const auto &ES = histogram.e();
  // the function checkRange should already have checked that startX <= endX,
  // but we still need to check values weren't out side the ranges
  if ((endX > XS.back()) || (startX < XS.front())) {
    throw std::out_of_range("Either the property startX or endX is outside the "
                            "range of X-values present in one of the specified "
                            "spectra");
  }
  // Get the index of the first bin contains the X-value, which means this is an
  // inclusive sum. The minus one is because lower_bound() returns index past
  // the last index pointing to a lower value. For example if startX has a
  // higher X value than the first bin boundary but lower than the second
  // lower_bound returns 1, which is the index of the second bin boundary
  ptrdiff_t startInd =
      std::lower_bound(XS.begin(), XS.end(), startX) - XS.begin() - 1;
  if (startInd == -1) { // happens if startX is the first X-value, e.g. the
                        // first X-value is zero and the user selects zero
    startInd = 0;
  }

  // the -1 matches definition of startIn, see the comment above that statement
  const ptrdiff_t endInd =
      std::lower_bound(XS.begin() + startInd, XS.end(), endX) - XS.begin() - 1;
  if (endInd == -1) { //
    throw std::invalid_argument("EndX was set to the start of one of the "
                                "spectra, it must greater than the first "
                                "X-value in any of the specified spectra");
  }

  // the +1 is because this is an inclusive sum (includes each bin that contains
  // each X-value). Hence if startInd == endInd we are still analyzing one bin
  const auto numBins = static_cast<double>(1 + endInd - startInd);
  // the +1 here is because the accumulate() stops one before the location of
  // the last iterator
  background =
      std::accumulate(YS.begin() + startInd, YS.begin() + endInd + 1, 0.0) /
      numBins;
  // The error on the total number of background counts in the background region
  // is taken as the sqrt the total number counts. To get the the error on the
  // counts in each bin just divide this by the number of bins. The variance =
  // error^2 that is the total variance divide by the number of bins _squared_.
  variance = std::accumulate(ES.begin() + startInd, ES.begin() + endInd + 1,
                             0.0, VectorHelper::SumSquares<double>()) /
             (numBins * numBins);
}

/**
 * Uses linear algorithm to do the fitting.
 * @param histogram the histogram to fit
 * @param background an output variable for the calculated background
 * @param variance an output variable for background's variance, currently
 * always zero.
 * @param startX an X value in the first bin to be included in the fit
 * @param endX an X value in the last bin to be included in the fit
 */
void CalculateFlatBackground::LinearFit(
    const HistogramData::Histogram &histogram, double &background,
    double &variance, const double startX, const double endX) {
  MatrixWorkspace_sptr WS = create<Workspace2D>(1, histogram);
  WS->setHistogram(0, histogram);
  IAlgorithm_sptr childAlg = createChildAlgorithm("Fit");

  IFunction_sptr func =
      API::FunctionFactory::Instance().createFunction("LinearBackground");
  childAlg->setProperty<IFunction_sptr>("Function", func);

  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty<bool>("CreateOutput", true);
  childAlg->setProperty<int>("WorkspaceIndex", 0);
  childAlg->setProperty<double>("StartX", startX);
  childAlg->setProperty<double>("EndX", endX);
  // Default minimizer doesn't work properly even on the easiest cases,
  // so Levenberg-MarquardtMD is used instead
  childAlg->setProperty<std::string>("Minimizer", "Levenberg-MarquardtMD");

  childAlg->executeAsChildAlg();

  std::string outputStatus = childAlg->getProperty("OutputStatus");
  if (outputStatus != "success") {
    g_log.warning("Unable to successfully fit the data: " + outputStatus);
    background = -1;
    return;
  }

  Mantid::API::ITableWorkspace_sptr output =
      childAlg->getProperty("OutputParameters");

  // Find rows with parameters we are after
  size_t rowA0, rowA1;
  output->find(static_cast<std::string>("A0"), rowA0, 0);
  output->find(static_cast<std::string>("A1"), rowA1, 0);

  // Linear function is defined as A0 + A1*x
  const double intercept = output->cell<double>(rowA0, 1);
  const double slope = output->cell<double>(rowA1, 1);

  const double centre = (startX + endX) / 2.0;

  // Calculate the value of the flat background by taking the value at the
  // centre point of the fit
  background = slope * centre + intercept;
  // ATM we don't calculate the error here.
  variance = 0;
}

/**
 * Utilizes cyclic boundary conditions when calculating the
 * average in the window.
 * @param histogram the histogram to operate on
 * @param background an output variable for the calculated background
 * @param variance an output variable for background's variance.
 * @param windowWidth the width of the averaging window in bins
 */
void CalculateFlatBackground::MovingAverage(
    const HistogramData::Histogram &histogram, double &background,
    double &variance, const size_t windowWidth) const {
  const auto &ys = histogram.y();
  const auto &es = histogram.e();
  double currentMin = std::numeric_limits<double>::max();
  double currentVariance = 0;

  for (size_t i = 0; i < ys.size(); ++i) {
    double sum = 0;
    double varSqSum = 0;
    for (size_t j = 0; j < windowWidth; ++j) {
      size_t index = i + j;
      if (index >= ys.size()) {
        // Cyclic boundary conditions.
        index -= ys.size();
      }
      sum += ys[index];
      varSqSum += es[index] * es[index];
    }
    const double average = sum / static_cast<double>(windowWidth);
    if (average < currentMin) {
      currentMin = average;
      currentVariance =
          varSqSum / static_cast<double>(windowWidth * windowWidth);
    }
  }
  background = currentMin;
  variance = currentVariance;
}

} // namespace Algorithms
} // namespace Mantid
