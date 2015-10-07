#include "MantidAlgorithms/AddTimeSeriesLog.h"
#include "MantidKernel/DateTimeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Algorithms {
using namespace API;
using namespace Kernel;

namespace {
/**
 * Creates/updates the named log on the given run. The template type
 * specifies either the type of TimeSeriesProperty that is expected to
 * exist or the type that will be created
 * @param run A reference to the run object that stores the logs
 * @param name The name of the log that is to be either created or updated
 * @param time A time string in ISO format, passed to the DateAndTime
 * constructor
 * @param value The value at the given time
 */
template <typename T>
void createOrUpdateValue(API::Run &run, const std::string &name,
                         const std::string &time, const T value) {
  TimeSeriesProperty<T> *timeSeries(NULL);
  if (run.hasProperty(name)) {
    timeSeries = dynamic_cast<TimeSeriesProperty<T> *>(run.getLogData(name));
    if (!timeSeries)
      throw std::invalid_argument(
          "Log '" + name +
          "' already exists but the values are a different type.");
  } else {
    timeSeries = new TimeSeriesProperty<T>(name);
    run.addProperty(timeSeries);
  }
  timeSeries->addValue(time, value);
}
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddTimeSeriesLog)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string AddTimeSeriesLog::name() const { return "AddTimeSeriesLog"; }

/// Algorithm's version for identification. @see Algorithm::version
int AddTimeSeriesLog::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AddTimeSeriesLog::category() const {
  return "DataHandling\\Logs";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/**
 * Initialize the algorithm's properties.
 */
void AddTimeSeriesLog::init() {
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("Workspace", "", Direction::InOut),
      "In/out workspace that will store the new log information");

  declareProperty(
      "Name", "", boost::make_shared<MandatoryValidator<std::string>>(),
      "A string name for either a new time series log to be created "
      "or an existing name to update",
      Direction::Input);
  declareProperty(
      "Time", "", boost::make_shared<DateTimeValidator>(),
      "An ISO formatted date/time string specifying the timestamp for "
      "the given log value, e.g 2010-09-14T04:20:12",
      Direction::Input);
  auto nonEmtpyDbl = boost::make_shared<MandatoryValidator<double>>();
  declareProperty("Value", EMPTY_DBL(), nonEmtpyDbl,
                  "The value for the log at the given time", Direction::Input);

  auto optionsValidator = boost::make_shared<ListValidator<std::string>>();
  optionsValidator->addAllowedValue("double");
  optionsValidator->addAllowedValue("int");
  declareProperty("Type", "double", optionsValidator,
                  "An optional type for the given value. A double value with a "
                  "Type=int will have "
                  "the fractional part chopped off.",
                  Direction::Input);
  declareProperty(
      "DeleteExisting", false,
      "If true and the named log exists then the whole log is removed first.",
      Direction::Input);
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void AddTimeSeriesLog::exec() {
  MatrixWorkspace_sptr logWS = getProperty("Workspace");
  std::string name = getProperty("Name");

  const bool deleteExisting = getProperty("DeleteExisting");
  auto &run = logWS->mutableRun();
  if (deleteExisting && run.hasProperty(name))
    removeExisting(logWS, name);

  createOrUpdate(run, name);
}

/**
 * @param logWS The workspace containing the log
 * @param name The name of the log to delete
 */
void AddTimeSeriesLog::removeExisting(API::MatrixWorkspace_sptr &logWS,
                                      const std::string &name) {
  auto deleter = createChildAlgorithm("DeleteLog", -1, -1, false);
  deleter->setProperty("Workspace", logWS);
  deleter->setProperty("Name", name);
  deleter->executeAsChildAlg();
}

/**
 * @param run The run object that either has the log or will store it
 * @param name The name of the log to create/update
 */
void AddTimeSeriesLog::createOrUpdate(API::Run &run, const std::string &name) {
  std::string time = getProperty("Time");
  double valueAsDouble = getProperty("Value");
  std::string type = getProperty("Type");
  bool asInt = (type == "int");

  if (asInt) {
    createOrUpdateValue<int>(run, name, time, static_cast<int>(valueAsDouble));
  } else {
    createOrUpdateValue<double>(run, name, time, valueAsDouble);
  }
}

} // namespace Algorithms
} // namespace Mantid
