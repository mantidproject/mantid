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
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Smoothing.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddLogSmoothed)

//----------------------------------------------------------------------------------------------

namespace {
enum class SmoothingMethod { BOXCAR, FFT_ZERO, FFT_BUTTERWORTH, enum_count };
const std::vector<std::string> smoothingMethods{"BoxCar", "Zeroing", "Butterworth"};
typedef Mantid::Kernel::EnumeratedString<SmoothingMethod, &smoothingMethods> SMOOTH;
} // namespace

namespace {
// wrap GSL pointers in unique pointers with deleters for memory leak safety in case of failures
constexpr auto interp_accel_deleter = [](gsl_interp_accel *p) { gsl_interp_accel_free(p); };
constexpr auto spline_deleter = [](gsl_spline *p) { gsl_spline_free(p); };

using accel_uptr = std::unique_ptr<gsl_interp_accel, decltype(interp_accel_deleter)>;
using spline_uptr = std::unique_ptr<gsl_spline, decltype(spline_deleter)>;

accel_uptr make_interp_accel() { return accel_uptr(gsl_interp_accel_alloc()); }
spline_uptr make_cubic_spline(std::vector<double> const &x, std::vector<double> const &y) {
  spline_uptr spline(gsl_spline_alloc(gsl_interp_cspline, x.size()));
  gsl_spline_init(spline.get(), x.data(), y.data(), x.size());
  return spline;
}
} // namespace

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AddLogSmoothed::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::InOut),
                  "An input/output workspace. The new log will be added to it.");
  declareProperty(
      "LogName", "", std::make_shared<MandatoryValidator<std::string>>(),
      "The name that will identify the log entry to ne smoothed.\nThis log must be a numerical series (double).");
  declareProperty(
      std::make_unique<Kernel::EnumeratedStringProperty<SmoothingMethod, &smoothingMethods>>("SmoothingMethod"),
      "The smoothing method to use");
  declareProperty(std::make_unique<ArrayProperty<int>>("Params", std::vector<int>()),
                  "The parameters which will be passed to the smoothing function.");
  declareProperty(
      "NewLogName", "",
      "Name of the newly created log. If not specified, the string '_smoothed' will be appended to the original name");
}

std::map<std::string, std::string> AddLogSmoothed::validateInputs() {
  std::map<std::string, std::string> issues;

  // validate parameters based on smoothing method chosen
  SMOOTH type = getPropertyValue("SmoothingMethod");
  std::vector<int> params = getProperty("Params");
  switch (type) {
  case SmoothingMethod::BOXCAR: {
    if (params.empty()) {
      issues["Params"] = "Boxcar smoothing requires the window width be passed as parameter";
    } else if (params[0] % 2 == 0) {
      issues["Params"] = "Boxcar smoothing requires an odd number of points as the window width";
    }
    break;
  }
  case SmoothingMethod::FFT_ZERO: {
    if (params.empty()) {
      issues["Params"] = "FFT zeroing requires the cutoff frequency as a parameter";
    } else if (params[0] <= 1) {
      issues["Params"] = "The cutoff in FFT zeroing must be larger than 1";
    }
    break;
  }
  case SmoothingMethod::FFT_BUTTERWORTH: {
    if (params.size() < 2) {
      issues["Params"] = "Butterworth smoothing requires two parameters, cutoff and order";
    } else if (params[0] <= 1 || params[1] < 1) {
      issues["Params"] = "In Butterworth smoothing, cutoff must be greater than one and order greater than zero";
    }
    break;
  }
  default:
    break;
  }
  if (issues.count("Params"))
    issues["SmoothingMethod"] = issues["Params"];

  // validate input workspace: must have a log with LogName and not one with NewLogName
  std::string logName = getPropertyValue("LogName");
  MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
  if (!ws)
    issues["InputWorkspace"] = "No matrix workspace specified for input workspace";
  Run &run = ws->mutableRun();
  if (!run.hasProperty(logName))
    issues["LogName"] = "Log " + logName + " not found in the workspace sample logs.";
  Property *prop = run.getProperty(logName);
  if (!prop)
    issues["LogName"] = "Log " + logName + " not found in the workspace sample logs.";
  auto *tsp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
  if (!tsp)
    issues["LogName"] = "Log " + logName +
                        " is not a numerical series (TimeSeriesProperty<double>) so we can't perform its derivative.";
  return issues;
}

std::vector<double> AddLogSmoothed::getUniformXValues(std::vector<double> const &xVec) {
  double xf = xVec.back(), xi = xVec.front();
  double dx = (xf - xi) / static_cast<double>(xVec.size());
  std::vector<double> newX;
  newX.reserve(xVec.size());
  for (double x = xi; x <= xf; x += dx) {
    newX.push_back(x);
  }
  return newX;
}

std::vector<double> AddLogSmoothed::getSplinedYValues(std::vector<double> const &newX, std::vector<double> const &x,
                                                      std::vector<double> const &y) {
  auto acc = make_interp_accel();
  auto cspline = make_cubic_spline(x, y);
  std::vector<double> newY;
  newY.reserve(newX.size());
  for (auto xi = newX.cbegin(); xi != newX.cend(); xi++) {
    newY.push_back(gsl_spline_eval(cspline.get(), *xi, acc.get()));
  }
  return newY;
}

std::vector<DateAndTime> AddLogSmoothed::timesToDateAndTime(DateAndTime const &start,
                                                            std::vector<double> const &times) {
  // Convert time in sec to DateAndTime
  std::vector<DateAndTime> timeFull;
  timeFull.reserve(times.size());
  std::transform(times.begin(), times.end(), std::back_inserter(timeFull),
                 [&start](const double time) { return start + time; });
  return timeFull;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AddLogSmoothed::exec() {
  MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
  std::vector<int> params = getProperty("Params");
  std::string logName = getPropertyValue("LogName");
  std::string newLogName = getPropertyValue("NewLogName");
  if (newLogName.empty())
    newLogName = logName + "_smoothed";

  // Progress progress(this, 0.0, 1.0, 1.0);

  // retrieve the time series data
  Run &run = ws->mutableRun();
  auto *tsp = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(logName));
  std::vector<double> values = tsp->valuesAsVector();
  std::vector<double> times = tsp->timesAsVectorSeconds();

  // Perform smoothing
  auto output = new TimeSeriesProperty<double>(newLogName);
  SMOOTH smoothingMethod = getPropertyValue("SmoothingMethod");
  switch (smoothingMethod) {
  case SmoothingMethod::BOXCAR: {
    std::vector<double> newValues = Smoothing::boxcarSmooth(values, params[0]);
    output->addValues(tsp->timesAsVector(), newValues);
    break;
  }
  case SmoothingMethod::FFT_ZERO: {
    std::vector<double> flatTimes = this->getUniformXValues(times);
    std::vector<double> splinedValues = this->getSplinedYValues(flatTimes, times, values);
    std::vector<double> smoothedValues = Smoothing::fftSmooth(splinedValues, params[0]);
    output->addValues(timesToDateAndTime(tsp->nthTime(0), flatTimes), smoothedValues);
    break;
  }
  case SmoothingMethod::FFT_BUTTERWORTH: {
    std::vector<double> flatTimes = this->getUniformXValues(times);
    std::vector<double> splinedValues = this->getSplinedYValues(flatTimes, times, values);
    std::vector<double> smoothedValues = Smoothing::fftButterworthSmooth(splinedValues, params[0], params[1]);
    output->addValues(timesToDateAndTime(tsp->nthTime(0), flatTimes), smoothedValues);
    break;
  }
  default:
    break;
  }

  // Add the log
  run.addProperty(output, true);

  g_log.notice() << "Added log named " << newLogName << '\n';
}

} // namespace Mantid::Algorithms
