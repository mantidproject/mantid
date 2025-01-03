// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/AddLogDerivative.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Strings.h"

#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddLogDerivative)

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AddLogDerivative::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::InOut),
                  "An input/output workspace. The new log will be added to it.");
  declareProperty("LogName", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "The name that will identify the log entry to perform a derivative.\n"
                  "This log must be a numerical series (double).");
  declareProperty("Derivative", 1, std::make_shared<BoundedValidator<int>>(1, 10),
                  "How many derivatives to perform. Default 1.");
  declareProperty("NewLogName", "",
                  "Name of the newly created log. If not "
                  "specified, the string '_derivativeN' will "
                  "be appended to the original name");
}

//----------------------------------------------------------------------------------------------
/** Perform the N^th derivative of a log
 *
 * @param progress :: Progress indicator
 * @param input :: input TSP. Must have N+1 log entries
 * @param name :: name of the resulting log
 * @param numDerivatives :: number of times to perform derivative.
 * @return
 */
Mantid::Kernel::TimeSeriesProperty<double> *
AddLogDerivative::makeDerivative(API::Progress &progress, Mantid::Kernel::TimeSeriesProperty<double> *input,
                                 const std::string &name, int numDerivatives) {
  if (input->size() < numDerivatives + 1)
    throw std::runtime_error("Log " + input->name() + " only has " + Strings::toString(input->size()) +
                             " values. Need at least " + Strings::toString(numDerivatives + 1) +
                             " to make this derivative.");

  std::vector<double> values, dVal;
  std::vector<double> times, dTime;
  values = input->valuesAsVector();
  times = input->timesAsVectorSeconds();

  for (int deriv = 0; deriv < numDerivatives; deriv++) {
    dVal.clear();
    dTime.clear();
    double t0 = times[0];
    double y0 = values[0];
    for (size_t i = 0; i < times.size() - 1; i++) {
      double y1 = values[i + 1];
      double t1 = times[i + 1];
      if (t1 != t0) {
        // Avoid repeated time values giving infinite derivatives
        double dy = (y1 - y0) / (t1 - t0);
        double t = (t0 + t1) / 2.0;
        dVal.emplace_back(dy);
        dTime.emplace_back(t);
        // For the next time interval
        t0 = t1;
        y0 = y1;
      }
    }
    times = dTime;
    values = dVal;

    progress.report("Add Log Derivative");
  }

  if (times.empty())
    throw std::runtime_error("Log " + input->name() +
                             " did not have enough non-repeated time values to "
                             "make this derivative.");

  // Convert time in sec to DateAndTime
  DateAndTime start = input->nthTime(0);
  std::vector<DateAndTime> timeFull;
  timeFull.reserve(times.size());

  std::transform(times.begin(), times.end(), std::back_inserter(timeFull),
                 [&start](const double time) { return start + time; });

  // Create the TSP out of it
  auto out = new TimeSeriesProperty<double>(name);
  out->addValues(timeFull, values);
  return out;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AddLogDerivative::exec() {
  MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
  std::string LogName = getPropertyValue("LogName");
  std::string NewLogName = getPropertyValue("NewLogName");
  int Derivative = getProperty("Derivative");
  if (!ws)
    return;

  if (NewLogName.empty())
    NewLogName = LogName + "_derivative" + Strings::toString(Derivative);

  Run &run = ws->mutableRun();
  if (!run.hasProperty(LogName))
    throw std::invalid_argument("Log " + LogName + " not found in the workspace sample logs.");
  Property *prop = run.getProperty(LogName);
  if (!prop)
    throw std::invalid_argument("Log " + LogName + " not found in the workspace sample logs.");
  auto *tsp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
  if (!tsp)
    throw std::invalid_argument("Log " + LogName +
                                " is not a numerical series "
                                "(TimeSeriesProperty<double>"
                                ") so we can't perform its "
                                "derivative.");

  Progress progress(this, 0.0, 1.0, Derivative);

  // Perform derivative
  TimeSeriesProperty<double> *output = makeDerivative(progress, tsp, NewLogName, Derivative);
  // Add the log
  run.addProperty(output, true);

  g_log.notice() << "Added log named " << NewLogName << '\n';
}

} // namespace Mantid::Algorithms
