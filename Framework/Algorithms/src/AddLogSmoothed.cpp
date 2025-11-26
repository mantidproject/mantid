// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/AddLogSmoothed.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Smoothing.h"
#include "MantidKernel/Spline.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddLogSmoothed)

//----------------------------------------------------------------------------------------------

namespace {
namespace PropertyNames {
std::string const INPUT_WKSP("InputWorkspace");
std::string const LOG_NAME("LogName");
std::string const SMOOTHING_METHOD("SmoothingMethod");
std::string const PARAMS("Params");
std::string const NEW_LOG_NAME("NewLogName");
} // namespace PropertyNames
} // namespace

namespace {
enum class SmoothingMethod { BOXCAR, FFT_ZERO, FFT_BUTTERWORTH, enum_count };
const std::vector<std::string> smoothingMethods{"BoxCar", "Zeroing", "Butterworth"};
typedef Mantid::Kernel::EnumeratedString<SmoothingMethod, &smoothingMethods> SMOOTH;
} // namespace

namespace {
using namespace Mantid::Types::Core;
std::vector<double> getUniformXValues(std::vector<double> const &xVec) {
  std::vector<double> newX;
  newX.reserve(xVec.size());
  if (xVec.size() < 2) {
    newX = xVec;
  } else {
    double const xf = xVec.back(), xi = xVec.front();
    double const dx = (xf - xi) / static_cast<double>(xVec.size() - 1);
    for (std::size_t i = 0; i < xVec.size(); i++) {
      newX.push_back(xi + static_cast<double>(i) * dx);
    }
  }
  return newX;
}

std::vector<DateAndTime> relativeToAbsoluteTime(DateAndTime const &startTime, std::vector<double> const &relTimes) {
  // Convert time in sec to DateAndTime
  std::vector<DateAndTime> timeFull;
  timeFull.reserve(relTimes.size());
  std::transform(relTimes.begin(), relTimes.end(), std::back_inserter(timeFull),
                 [&startTime](const double time) { return startTime + time; });
  return timeFull;
}
} // namespace

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AddLogSmoothed::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::INPUT_WKSP, "", Direction::InOut),
                  "An input/output workspace. The new log will be added to it.");
  declareProperty(
      PropertyNames::LOG_NAME, "", std::make_shared<MandatoryValidator<std::string>>(),
      "The name that will identify the log entry to be smoothed.\nThis log must be a numerical series (double).");
  declareProperty(std::make_unique<Kernel::EnumeratedStringProperty<SmoothingMethod, &smoothingMethods>>(
                      PropertyNames::SMOOTHING_METHOD),
                  "The smoothing method to use");
  declareProperty(std::make_unique<ArrayProperty<int>>(PropertyNames::PARAMS, std::vector<int>()),
                  "The parameters which will be passed to the smoothing function.");
  declareProperty(
      PropertyNames::NEW_LOG_NAME, "",
      "Name of the newly created log. If not specified, the string '_smoothed' will be appended to the original name");
}

std::map<std::string, std::string> AddLogSmoothed::validateInputs() {
  std::map<std::string, std::string> issues;

  // validate parameters based on smoothing method chosen
  SMOOTH type = getPropertyValue(PropertyNames::SMOOTHING_METHOD);
  std::vector<int> params = getProperty(PropertyNames::PARAMS);
  switch (type) {
  case SmoothingMethod::BOXCAR: {
    if (params.empty()) {
      issues[PropertyNames::PARAMS] = "Boxcar smoothing requires the window width be passed as parameter";
    } else if (params[0] < 0) {
      issues[PropertyNames::PARAMS] = "Boxcar smoothing requires a positive window; given " + std::to_string(params[0]);
    } else if (params[0] % 2 == 0) {
      issues[PropertyNames::PARAMS] =
          "Boxcar smoothing requires an odd window size: " + std::to_string(params[0]) + " is even";
    }
    break;
  }
  case SmoothingMethod::FFT_ZERO: {
    if (params.empty()) {
      issues[PropertyNames::PARAMS] = "FFT zeroing requires the cutoff frequency as a parameter";
    } else if (params[0] <= 1) {
      issues[PropertyNames::PARAMS] =
          "The cutoff in FFT zeroing must be larger than 1; passed " + std::to_string(params[0]);
    }
    break;
  }
  case SmoothingMethod::FFT_BUTTERWORTH: {
    if (params.size() < 2) {
      issues[PropertyNames::PARAMS] =
          "Butterworth smoothing requires two parameters, passed " + std::to_string(params.size());
    } else if (params[0] <= 1 || params[1] < 1) {
      issues[PropertyNames::PARAMS] =
          "In Butterworth smoothing, cutoff must be greater than 1 and order must be greater than 0";
    }
    break;
  }
  default: {
    issues[PropertyNames::SMOOTHING_METHOD] =
        "Parameter validation for smoothing method " + std::string(type) + " has not been implemented";
  }
  }
  if (issues.count(PropertyNames::PARAMS))
    issues[PropertyNames::SMOOTHING_METHOD] = issues[PropertyNames::PARAMS];

  // validate input workspace: must have a log with LogName
  std::string logName = getPropertyValue(PropertyNames::LOG_NAME);
  MatrixWorkspace_const_sptr ws = getProperty(PropertyNames::INPUT_WKSP);
  if (!ws) {
    issues[PropertyNames::INPUT_WKSP] = "No matrix workspace specified for input workspace";
    return issues;
  }
  Run const &run = ws->run();
  if (!run.hasProperty(logName)) {
    issues[PropertyNames::LOG_NAME] = "Log " + logName + " not found in the workspace sample logs.";
    return issues;
  }
  auto const *tsp = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(logName));
  if (!tsp) {
    issues[PropertyNames::LOG_NAME] = "Log " + logName + " must be a numerical time series (TimeSeries<double>).";
  } else {
    std::size_t const minBoxCarSize = (params.empty() ? 0 : static_cast<std::size_t>(params[0]));
    std::size_t const MIN_SPLINE_POINTS{5UL}; // minimum points needed for spline fits
    std::size_t minSize = (type == SmoothingMethod::BOXCAR ? minBoxCarSize : MIN_SPLINE_POINTS);
    if (static_cast<std::size_t>(tsp->size()) < minSize) {
      issues[PropertyNames::LOG_NAME] = "Log " + logName +
                                        " has insufficient number of points: " + std::to_string(tsp->size()) + " < " +
                                        std::to_string(minSize);
    }
  }
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AddLogSmoothed::exec() {
  MatrixWorkspace_sptr ws = getProperty(PropertyNames::INPUT_WKSP);
  std::vector<int> params = getProperty(PropertyNames::PARAMS);
  std::string logName = getPropertyValue(PropertyNames::LOG_NAME);
  std::string newLogName = getPropertyValue(PropertyNames::NEW_LOG_NAME);
  if (newLogName.empty())
    newLogName = logName + "_smoothed";

  // retrieve the time series data
  Run &run = ws->mutableRun();
  auto *tsp = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(logName));
  std::vector<double> values = tsp->valuesAsVector();
  std::vector<double> times = tsp->timesAsVectorSeconds();

  // Perform smoothing
  auto output = std::make_unique<TimeSeriesProperty<double>>(newLogName);
  SMOOTH smoothingMethod = getPropertyValue(PropertyNames::SMOOTHING_METHOD);
  switch (smoothingMethod) {
  case SmoothingMethod::BOXCAR: {
    std::vector<double> newValues = Smoothing::boxcarSmooth(values, params[0]);
    output->addValues(tsp->timesAsVector(), newValues);
    break;
  }
  case SmoothingMethod::FFT_ZERO: {
    std::vector<double> flatTimes = getUniformXValues(times);
    std::vector<double> splinedValues = CubicSpline<double, double>::getSplinedYValues(flatTimes, times, values);
    std::vector<double> smoothedValues = Smoothing::fftSmooth(splinedValues, params[0]);
    output->addValues(relativeToAbsoluteTime(tsp->nthTime(0), flatTimes), smoothedValues);
    break;
  }
  case SmoothingMethod::FFT_BUTTERWORTH: {
    std::vector<double> flatTimes = getUniformXValues(times);
    std::vector<double> splinedValues = CubicSpline<double, double>::getSplinedYValues(flatTimes, times, values);
    std::vector<double> smoothedValues = Smoothing::fftButterworthSmooth(splinedValues, params[0], params[1]);
    output->addValues(relativeToAbsoluteTime(tsp->nthTime(0), flatTimes), smoothedValues);
    break;
  }
  default:
    throw Mantid::Kernel::Exception::NotImplementedError("Smoothing method " + std::string(smoothingMethod) +
                                                         " has not been implemented");
  }

  // Add the log
  run.addProperty(output.release(), true);

  g_log.notice() << "Added log named " << newLogName << " to " << ws << '\n';
}

} // namespace Mantid::Algorithms
