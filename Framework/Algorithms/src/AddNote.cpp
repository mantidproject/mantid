// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/AddNote.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/DateTimeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "boost/date_time/local_time/posix_time_zone.hpp"

namespace Mantid::Algorithms {

using namespace API;
using namespace Kernel;
using Types::Core::DateAndTime;

namespace {
/**
 * Creates/updates the named log on the given run.
 * @param run A reference to the run object that stores the logs
 * @param name The name of the log that is to be either created or updated
 * @param time A time string in ISO format, passed to the DateAndTime
 * constructor
 * @param value The value at the given time
 */
void createOrUpdateValue(API::Run &run, const std::string &name, const std::string &time, const std::string &value) {
  TimeSeriesProperty<std::string> *timeSeries(nullptr);
  if (run.hasProperty(name)) {
    timeSeries = dynamic_cast<TimeSeriesProperty<std::string> *>(run.getLogData(name));
    if (!timeSeries)
      throw std::invalid_argument("Log '" + name + "' already exists but the values are a different type.");
  } else {
    timeSeries = new TimeSeriesProperty<std::string>(name);
    run.addProperty(timeSeries);
  }
  timeSeries->addValue(time, value);
}
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddNote)

//----------------------------------------------------------------------------------------------
AddNote::AddNote() { useAlgorithm("Comment", 1); }

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string AddNote::name() const { return "AddNote"; }

/// Algorithm's version for identification. @see Algorithm::version
int AddNote::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AddNote::category() const { return "DataHandling\\Logs"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string AddNote::summary() const { return "Adds a timestamped note to a workspace."; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AddNote::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("Workspace", "", Direction::InOut),
                  "An InOut workspace that will store the new log information");

  declareProperty("Name", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "A String name for either a new time series log to be "
                  "created or an existing name to update",
                  Direction::Input);

  auto dtv = std::make_shared<DateTimeValidator>();
  dtv->allowEmpty(true);

  declareProperty("Time", "", dtv,
                  "An ISO formatted date/time string specifying the timestamp for "
                  "the given log value, for example 2010-09-14T04:20:12 \n"
                  "If left blank, this will default to the current Date and Time",
                  Direction::Input);

  declareProperty("Value", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "A String value for the series log at the given time", Direction::Input);

  declareProperty("DeleteExisting", false, "If true and the named log exists then the whole log is removed first.",
                  Direction::Input);
}
//----------------------------------------------------------------------------------------------
/** Executes the algorithm.
 */
void AddNote::exec() {
  MatrixWorkspace_sptr logWS = getProperty("Workspace");
  std::string logName = getProperty("Name");
  const bool deleteExisting = getProperty("DeleteExisting");
  auto &run = logWS->mutableRun();
  if (deleteExisting && run.hasProperty(logName)) {
    removeExisting(logWS, logName);
  }
  createOrUpdate(run, logName);
}

/**
 * Removes an existing instance of the log from the Workspace
 * @param logWS The workspace containing the log
 * @param name The name of the log to delete
 */
void AddNote::removeExisting(API::MatrixWorkspace_sptr &logWS, const std::string &name) {
  auto deleter = createChildAlgorithm("DeleteLog", -1, -1, false);
  deleter->setProperty("Workspace", logWS);
  deleter->setProperty("Name", name);
  deleter->executeAsChildAlg();
}

/**
 * Obtains variables to use in createOrUpdateValue function
 * @param run The run object that either has the log or will store it
 * @param name The name of the log to create/update
 */
void AddNote::createOrUpdate(API::Run &run, const std::string &name) {
  std::string time = getProperty("Time");
  if (time.empty()) {
    namespace pt = boost::posix_time;
    auto dateTimeObj = DateAndTime(pt::second_clock::local_time());
    time = dateTimeObj.toISO8601String();
  }
  std::string value = getProperty("Value");

  createOrUpdateValue(run, name, time, value);
}

} // namespace Mantid::Algorithms
