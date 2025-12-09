// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/AddLogInterpolated.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/GSL_Helpers.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Spline.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <algorithm>
#include <span>
#include <stdexcept>

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddLogInterpolated)

using namespace API;
using namespace Kernel;

namespace {
namespace PropertyNames {
std::string const INPUT_WKSP("Workspace");
std::string const LOG_INTERP("LogToInterpolate");
std::string const LOG_MATCH("LogToMatch");
std::string const NEW_LOG_NAME("NewLogName");
} // namespace PropertyNames
} // namespace

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AddLogInterpolated::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::INPUT_WKSP, "", Direction::InOut),
                  "An input/output workspace containing the log to interpolate. The new log will be added to it.");

  declareProperty(PropertyNames::LOG_INTERP, "", std::make_shared<MandatoryValidator<std::string>>(),
                  "The name of the log entry to be interpolated. This log must be a numerical series (double).");

  declareProperty(
      PropertyNames::LOG_MATCH, "", std::make_shared<MandatoryValidator<std::string>>(),
      "The name of the log entry defining the interpolation points. This log must be a numerical series (double).");

  declareProperty(PropertyNames::NEW_LOG_NAME, "",
                  "Name of the newly created log. If not specified, the string '_interpolated' will be appended to the "
                  "original name");
}

//----------------------------------------------------------------------------------------------
/** Input validation for the WorkspaceToInterpolate
 */
std::map<std::string, std::string> AddLogInterpolated::validateInputs() {
  std::map<std::string, std::string> issues;

  // validate input workspace: must have a log with LogName
  MatrixWorkspace_const_sptr ws = getProperty(PropertyNames::INPUT_WKSP);
  if (!ws) {
    issues[PropertyNames::INPUT_WKSP] = "No matrix workspace specified for input workspace";
    return issues;
  }

  // make sure the workspace has the needed logs
  Run const &run = ws->run();
  std::string logInterp = getPropertyValue(PropertyNames::LOG_INTERP);
  if (!run.hasProperty(logInterp)) {
    issues[PropertyNames::LOG_INTERP] = "Log " + logInterp + " not found in the workspace sample logs.";
  }
  std::string logMatch = getPropertyValue(PropertyNames::LOG_MATCH);
  if (!run.hasProperty(logMatch)) {
    issues[PropertyNames::LOG_MATCH] = "Log " + logMatch + " not found in the workspace sample logs.";
  }
  if (!issues.empty()) {
    return issues;
  }

  // if the properties are present, make sure they are time series logs
  auto const *tspMatch = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(logMatch));
  if (!tspMatch) {
    issues[PropertyNames::LOG_MATCH] = "Log " + logMatch + " must be a numerical time series (TimeSeries<double>).";
  }
  auto const *tspInterp = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(logInterp));
  if (!tspInterp) {
    issues[PropertyNames::LOG_INTERP] = "Log " + logInterp + " must be a numerical time series (TimeSeries<double>).";
  } else {
    if (tspInterp->size() < static_cast<int>(Mantid::Kernel::spline::MIN_CSPLINE_POINTS)) {
      issues[PropertyNames::LOG_INTERP] = "Log " + logInterp +
                                          " has insufficient number of points: " + std::to_string(tspInterp->size()) +
                                          " < " + std::to_string(Mantid::Kernel::spline::MIN_CSPLINE_POINTS);
    }
  }

  return issues;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AddLogInterpolated::exec() {
  MatrixWorkspace_sptr ws = getProperty(PropertyNames::INPUT_WKSP);
  std::string logInterp = getPropertyValue(PropertyNames::LOG_INTERP);
  std::string logMatch = getPropertyValue(PropertyNames::LOG_MATCH);
  std::string newLogName = getPropertyValue(PropertyNames::NEW_LOG_NAME);
  if (newLogName.empty())
    newLogName = logInterp + "_interpolated";

  // retrieve the time series data
  Run &run = ws->mutableRun();
  auto *tspInterp = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(logInterp));
  std::vector<double> valuesInterp = tspInterp->valuesAsVector();
  std::vector<double> timesInterp = tspInterp->timesAsVectorSeconds();
  auto *tspMatch = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(logMatch));
  std::vector<double> newTimes = tspMatch->timesAsVectorSeconds();

  // perform interpolation
  auto tspOutput = std::make_unique<TimeSeriesProperty<double>>(newLogName);
  auto range = this->findInterpolationRange(timesInterp, newTimes);
  std::span<double const> newTimesInRange(newTimes.cbegin() + range.first, range.second - range.first);
  std::vector<DateAndTime> datetimes = tspMatch->timesAsVector();
  std::vector<DateAndTime> newDatetimesInRange(datetimes.cbegin() + range.first, datetimes.cbegin() + range.second);
  std::vector<double> newValues =
      Mantid::Kernel::CubicSpline<double, double>::getSplinedYValues(newTimesInRange, timesInterp, valuesInterp);
  tspOutput->addValues(newDatetimesInRange, newValues);

  run.addProperty(tspOutput.release(), true);

  g_log.notice() << "Added log named " << newLogName << " to " << ws->getName() << '\n';

  setProperty(PropertyNames::INPUT_WKSP, ws);
}

/** Find the region that has to be interpolated
 * @param xAxisIn : the x-axis of the original data
 * @param xAxisOut : the x-axis the interpolated data will have
 * @return : pair of indices for representing the interpolation range
 */
std::pair<size_t, size_t> AddLogInterpolated::findInterpolationRange(std::vector<double> const &xAxisIn,
                                                                     std::vector<double> const &xAxisOut) const {
  size_t firstIndex = 0;
  size_t lastIndex = xAxisOut.size();

  if (xAxisOut.empty() || xAxisIn.empty()) {
    // if either vector is empty, don't do anything else
  } else {
    if (xAxisOut.front() >= xAxisIn.back()) {
      lastIndex = firstIndex;
    } else if (xAxisOut.back() <= xAxisIn.front()) {
      firstIndex = lastIndex;
    } else {
      auto start =
          std::find_if(xAxisOut.cbegin(), xAxisOut.cend(), [&xAxisIn](double x) { return x >= xAxisIn.front(); });
      firstIndex = std::distance(xAxisOut.begin(), start);
      auto stop = std::find_if(start, xAxisOut.cend(), [&xAxisIn](double x) { return x > xAxisIn.back(); });
      lastIndex = std::distance(xAxisOut.begin(), stop);
    }
  }
  return std::make_pair(firstIndex, lastIndex);
}
} // namespace Mantid::Algorithms
