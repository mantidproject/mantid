#include <MantidAlgorithms/MergeRuns/SampleLogsBehaviour.h>
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;

namespace {
std::string generateDifferenceMessage(const std::string &item,
                                      const std::string &wsName,
                                      const std::string &wsValue,
                                      const std::string &firstValue) {
  std::stringstream stringstream;
  stringstream << "Item \"" << item
               << "\" has different values in workspaces! Found: " << wsValue
               << " in workspace " << wsName
               << " but value in first workspace value was: " << firstValue
               << "." << std::endl;
  return stringstream.str();
}
}

const std::string SampleLogsBehaviour::TIME_SERIES_MERGE =
    "sample_logs_time_series";
const std::string SampleLogsBehaviour::LIST_MERGE = "sample_logs_list";
const std::string SampleLogsBehaviour::WARN_MERGE = "sample_logs_warn";
const std::string SampleLogsBehaviour::FAIL_MERGE = "sample_logs_fail";
const std::string SampleLogsBehaviour::WARN_MERGE_TOLERANCES =
    "sample_logs_warn_tolerances";
const std::string SampleLogsBehaviour::FAIL_MERGE_TOLERANCES =
    "sample_logs_fail_tolerances";

const std::string SampleLogsBehaviour::TIME_SERIES_SUFFIX = "_time_series";
const std::string SampleLogsBehaviour::LIST_SUFFIX = "_list";

/**
 * Create and initialise an object that is responsbile for keeping track of the
 * merge types, and performing the merge or warning/error for sample logs when
 * calling MergeRuns.
 *
 * @param ws the base workspace that the other workspaces are merged into
 * @param logger the logger from the parent algorithm
 * @param sampleLogsTimeSeries a string with a comma separated list of the logs
 * for the time series merge
 * @param sampleLogsList a string with a comma separated list of the logs for a
 * list merge
 * @param sampleLogsWarn a string with a comma separated list of the logs for
 * warning when different on merging
 * @param sampleLogsWarnTolerances a string with a single value or comma
 * separated list of values for the warning tolerances
 * @param sampleLogsFail a string with a comma separated list of the logs for
 * throwing an error when different on merging
 * @param sampleLogsFailTolerances a string with a single value or comma
 * separated list of values for the error tolerances
 * @return An instance of SampleLogsBehaviour initialised with the merge types
 * from the IPF and parent algorithm
 */
SampleLogsBehaviour::SampleLogsBehaviour(
    const MatrixWorkspace_sptr &ws, Logger &logger,
    std::string sampleLogsTimeSeries, std::string sampleLogsList,
    std::string sampleLogsWarn, std::string sampleLogsWarnTolerances,
    std::string sampleLogsFail, std::string sampleLogsFailTolerances)
    : m_logger(logger) {
  setSampleMap(m_logMap, MergeLogType::TimeSeries, sampleLogsTimeSeries, ws,
               "");
  setSampleMap(m_logMap, MergeLogType::List, sampleLogsList, ws, "");
  setSampleMap(m_logMap, MergeLogType::Warn, sampleLogsWarn, ws,
               sampleLogsWarnTolerances);
  setSampleMap(m_logMap, MergeLogType::Fail, sampleLogsFail, ws,
               sampleLogsFailTolerances);

  SampleLogsMap instrumentMap;
  this->createSampleLogsMapsFromInstrumentParams(instrumentMap, ws);

  // This adds the parameters from the instrument to the main map, with any
  // duplicates left as the versions in the MergeRuns arguments.
  m_logMap.insert(instrumentMap.begin(), instrumentMap.end());
}

/**
 * Extracts all of the settings from the instrument parameters, and adds them to
 * a map of sample log behaviours.
 *
 * @param map the map to add the merge behaviours to
 * @param ws the workspace with the instrument and initial map
 */
void SampleLogsBehaviour::createSampleLogsMapsFromInstrumentParams(
    SampleLogsMap &map, const MatrixWorkspace_sptr &ws) {
  std::string params =
      ws->getInstrument()->getParameterAsString(TIME_SERIES_MERGE, false);
  setSampleMap(map, MergeLogType::TimeSeries, params, ws, "", true);

  params = ws->getInstrument()->getParameterAsString(LIST_MERGE, false);
  setSampleMap(map, MergeLogType::List, params, ws, "", true);

  params = ws->getInstrument()->getParameterAsString(WARN_MERGE, false);
  std::string paramsTolerances;
  paramsTolerances =
      ws->getInstrument()->getParameterAsString(WARN_MERGE_TOLERANCES, false);
  setSampleMap(map, MergeLogType::Warn, params, ws, paramsTolerances, true);

  params = ws->getInstrument()->getParameterAsString(FAIL_MERGE, false);
  paramsTolerances =
      ws->getInstrument()->getParameterAsString(FAIL_MERGE_TOLERANCES, false);
  setSampleMap(map, MergeLogType::Fail, params, ws, paramsTolerances, true);
}

/**
 * This method updates the map with the sample log behaviour, and adds the new
 * property to the workspace if required. if skipIfInPrimaryMap is true sample
 * logs in the primary map are ignored. Throws std::invalid_argument if a sample
 * log is not found.
 *
 * @param map the map to add the merge behaviours to
 * @param mergeType an enum for the type of merge to perform
 * @param params a string containing a comma separated list of the sample logs
 * to merge for this behaviour
 * @param ws the base workspace that the other workspaces are merged into
 * @param paramsTolerances a string containing a comma spearated list of the
 * tolerances for this merge behaviour (optional)
 * @param skipIfInPrimaryMap wether to skip if in the member variable map
 * (optional, default false)
 */
void SampleLogsBehaviour::setSampleMap(SampleLogsMap &map,
                                       const MergeLogType &mergeType,
                                       const std::string &params,
                                       const API::MatrixWorkspace_sptr &ws,
                                       const std::string paramsTolerances,
                                       bool skipIfInPrimaryMap) {

  StringTokenizer tokenizer(params, ",", StringTokenizer::TOK_TRIM |
                                             StringTokenizer::TOK_IGNORE_EMPTY);
  StringTokenizer tokenizerTolerances(paramsTolerances, ",",
                                      StringTokenizer::TOK_TRIM |
                                          StringTokenizer::TOK_IGNORE_EMPTY);

  auto tolerancesStringVector = tokenizerTolerances.asVector();

  std::vector<double> tolerancesVector = createTolerancesVector(
      tokenizer.asVector().size(), tolerancesStringVector, paramsTolerances);

  StringTokenizer::Iterator i = tokenizer.begin();
  std::vector<double>::iterator j = tolerancesVector.begin();

  for (; i != tokenizer.end() && j != tolerancesVector.end(); ++i, ++j) {
    auto item = *i;
    auto tolerance = *j;

    if (skipIfInPrimaryMap && (m_logMap.count(item) != 0)) {
      continue;
    }

    if (map.count(item) != 0) {
      throw std::invalid_argument(
          "Error when making list of merge items, sample log \"" + item +
          "\" defined more than once!");
    }

    std::shared_ptr<Property> prop;
    try {
      prop = std::shared_ptr<Property>(ws->getLog(item)->clone());
    } catch (std::invalid_argument &) {
      m_logger.warning() << "Could not merge sample log \"" << item
                         << "\", does not exist in workspace!" << std::endl;
      continue;
    }

    bool isNumeric;
    double value = 0.0;
    try {
      value = ws->getLogAsSingleValue(item);
      isNumeric = true;
    } catch (std::invalid_argument &) {
      isNumeric = false;
      if (mergeType == MergeLogType::TimeSeries) {
        m_logger.error() << item << " could not be converted to a numeric type"
                         << std::endl;
        continue;
      }
    }

    if (mergeType == MergeLogType::TimeSeries) {
      try {
        // See if property exists already - merging an output of MergeRuns
        prop = std::shared_ptr<Property>(
            ws->getLog(item + TIME_SERIES_SUFFIX)->clone());
      } catch (std::invalid_argument &) {
        // Property does not already exist
        std::unique_ptr<Kernel::TimeSeriesProperty<double>> timeSeriesProp(
            new TimeSeriesProperty<double>(item + TIME_SERIES_SUFFIX));
        std::string startTime = ws->mutableRun().startTime().toISO8601String();
        timeSeriesProp->addValue(startTime, value);
        ws->mutableRun().addLogData(
            std::unique_ptr<Kernel::Property>(std::move(timeSeriesProp)));
        prop = std::shared_ptr<Property>(
            ws->getLog(item + TIME_SERIES_SUFFIX)->clone());
      }
    } else if (mergeType == MergeLogType::List) {
      try {
        // See if property exists already - merging an output of MergeRuns
        prop =
            std::shared_ptr<Property>(ws->getLog(item + LIST_SUFFIX)->clone());
      } catch (std::invalid_argument &) {
        ws->mutableRun().addProperty(item + LIST_SUFFIX, prop->value());
        prop =
            std::shared_ptr<Property>(ws->getLog(item + LIST_SUFFIX)->clone());
      }
    }

    map[item] = {mergeType, prop, tolerance, isNumeric};
  }
}

/**
 * Creates a vector of tolernaces with the same size as the number of sample
 * logs for the merge type. If the number of names and tolerances is the same
 * the vector is filled with the tolerances for each name. If no tolerances were
 * specified all tolerances are set to -1, and if one tolerance is given all
 * tolerances are set to that value.
 *
 * @param numberNames the number of sample log names
 * @param tolerances a vector containing strings with the tolerances
 * @param paramsTolerances a string containing nothing, or tolerances to use
 * @return a vector of doubles of size numberNames
 */
std::vector<double> SampleLogsBehaviour::createTolerancesVector(
    size_t numberNames, const std::vector<std::string> &tolerances,
    const std::string paramsTolerances) {
  size_t numberTolerances = tolerances.size();

  std::vector<double> tolerancesVector(numberNames);

  if (numberNames == numberTolerances) {
    std::transform(tolerances.begin(), tolerances.end(),
                   tolerancesVector.begin(),
                   [](const std::string &val) { return std::stod(val); });
  } else if (paramsTolerances.empty()) {
    std::fill(tolerancesVector.begin(), tolerancesVector.end(), -1.0);
  } else if (numberTolerances == 1) {
    double value = std::stod(tolerances.front());
    std::fill(tolerancesVector.begin(), tolerancesVector.end(), value);
  } else {
    throw std::invalid_argument("Invalid length of tolerances, found " +
                                std::to_string(numberTolerances) +
                                " tolerance values but " +
                                std::to_string(numberNames) + " names.");
  }

  return tolerancesVector;
}

/**
 * Updates the sample logs according to the requested behaviour.
 *
 * @param addeeWS the workspace being merged
 * @param outWS the workspace the others are merged into
 */
void SampleLogsBehaviour::mergeSampleLogs(
    const MatrixWorkspace_sptr &addeeWS,
    const API::MatrixWorkspace_sptr &outWS) {
  for (auto item : m_logMap) {
    Property *addeeWSProperty = addeeWS->getLog(item.first);

    double addeeWSNumber = 0;
    double outWSNumber = 0;

    try {
      addeeWSNumber = addeeWS->getLogAsSingleValue(item.first);
      outWSNumber = outWS->getLogAsSingleValue(item.first);
    } catch (std::invalid_argument &) {
      if (item.second.isNumeric) {
        throw std::invalid_argument(
            item.first + " could not be converted to a numeric type");
      }
    }

    switch (item.second.type) {
    case MergeLogType::TimeSeries: {
      this->updateTimeSeriesProperty(addeeWS, outWS, item.first);
      break;
    }
    case MergeLogType::List: {
      this->updateListProperty(addeeWS, outWS, addeeWSProperty, item.first);
      break;
    }
    case MergeLogType::Warn:
      this->checkWarnProperty(addeeWS, addeeWSProperty, item.second,
                              addeeWSNumber, outWSNumber, item.first);
      break;
    case MergeLogType::Fail:
      this->checkErrorProperty(addeeWS, addeeWSProperty, item.second,
                               addeeWSNumber, outWSNumber, item.first);
      break;
    }
  }
}

/**
 * Perform the update for a time series property, adding a new value to the
 *existing time series property. Skipped if the time series log entry is in the
 *addeeWS.
 *
 * @param addeeWS the workspace being merged
 * @param outWS the workspace the others are merged into
 * @param name the name of the property
 */
void SampleLogsBehaviour::updateTimeSeriesProperty(
    const MatrixWorkspace_sptr &addeeWS, const MatrixWorkspace_sptr &outWS,
    const std::string name) {
  try {
    // If this already exists we do not need to do anything, Time Series Logs
    // are combined when adding workspaces.
    addeeWS->getLog(name + TIME_SERIES_SUFFIX);
  } catch (std::invalid_argument &) {
    auto timeSeriesProp = outWS->mutableRun().getTimeSeriesProperty<double>(
        name + TIME_SERIES_SUFFIX);
    Kernel::DateAndTime startTime = addeeWS->mutableRun().startTime();
    double value = addeeWS->mutableRun().getLogAsSingleValue(name);
    timeSeriesProp->addValue(startTime, value);
  }
}

/**
 * Perform the update for a list property, appending a new value to the existing
 *string. If the list log entry is in the addeeWS the list log entry is merged
 *instead.
 *
 * @param addeeWS the workspace being merged
 * @param outWS the workspace the others are merged into
 * @param addeeWSProperty the relevant property in addeeWS
 * @param name the name of the property
 */
void SampleLogsBehaviour::updateListProperty(
    const MatrixWorkspace_sptr &addeeWS, const MatrixWorkspace_sptr &outWS,
    Property *addeeWSProperty, const std::string name) {
  try {
    // If this already exists we combine the two strings.
    auto propertyAddeeWS = addeeWS->getLog(name + LIST_SUFFIX);
    auto propertyOutWS = outWS->mutableRun().getProperty(name + LIST_SUFFIX);
    propertyOutWS->setValue(propertyOutWS->value() + ", " +
                            propertyAddeeWS->value());
  } catch (std::invalid_argument &) {
    auto property = outWS->mutableRun().getProperty(name + LIST_SUFFIX);
    property->setValue(property->value() + ", " + addeeWSProperty->value());
  }
}

/**
 * Performs the check to see if a warning should be generated because logs are
 *different. Performs a numeric comparison if a tolerance is set and the log is
 *a number, else performs a string comparison.
 *
 * @param addeeWS the workspace being merged
 * @param addeeWSProperty the property value of the workspace being merged
 * @param behaviour the merge behaviour struct, containing information about the
 *merge
 * @param addeeWSNumber a double for the addeeWS value (0 if it is not numeric)
 * @param outWSNumber a double for the outWS value (0 if it is not numeric)
 * @param name the name of the sample log to check
 */
void SampleLogsBehaviour::checkWarnProperty(const MatrixWorkspace_sptr &addeeWS,
                                            Property *addeeWSProperty,
                                            const SampleLogBehaviour &behaviour,
                                            const double addeeWSNumber,
                                            const double outWSNumber,
                                            const std::string name) {
  if (behaviour.isNumeric && behaviour.tolerance > 0.0) {
    if (std::abs(addeeWSNumber - outWSNumber) > behaviour.tolerance) {
      m_logger.warning() << generateDifferenceMessage(
          name, addeeWS->name(), addeeWSProperty->value(),
          behaviour.property->value());
    }
  } else {
    if (behaviour.property->value().compare(addeeWSProperty->value()) != 0) {
      m_logger.warning() << generateDifferenceMessage(
          name, addeeWS->name(), addeeWSProperty->value(),
          behaviour.property->value());
    }
  }
}

/**
 ** Performs the check to see if an error should be generated because logs are
 *different. Performs a numeric comparison if a tolerance is set and the log is
 *a number, else performs a string comparison.
 *
 * @param addeeWS the workspace being merged
 * @param addeeWSProperty the property value of the workspace being merged
 * @param behaviour the merge behaviour struct, containing information about the
 *merge
 * @param addeeWSNumber a double for the addeeWS value (0 if it is not numeric)
 * @param outWSNumber a double for the outWS value (0 if it is not numeric)
 * @param name the name of the sample log to check
 */
void SampleLogsBehaviour::checkErrorProperty(
    const MatrixWorkspace_sptr &addeeWS, Property *addeeWSProperty,
    const SampleLogBehaviour &behaviour, const double addeeWSNumber,
    const double outWSNumber, const std::string name) {
  if (behaviour.isNumeric && behaviour.tolerance > 0.0) {
    if (std::abs(addeeWSNumber - outWSNumber) > behaviour.tolerance) {
      throw std::invalid_argument(generateDifferenceMessage(
          name, addeeWS->name(), addeeWSProperty->value(),
          behaviour.property->value()));
    }
  } else {
    if (behaviour.property->value().compare(addeeWSProperty->value()) != 0) {
      throw std::invalid_argument(generateDifferenceMessage(
          name, addeeWS->name(), addeeWSProperty->value(),
          behaviour.property->value()));
    }
  }
}

/**
 * Set the values in the map to be the same as those in the output workspace.
 *
 * @param ws the merged workspace
 */
void SampleLogsBehaviour::setUpdatedSampleLogs(
    const API::MatrixWorkspace_sptr &ws) {
  for (auto item : m_logMap) {
    std::string propertyToReset = item.first;

    if (item.second.type == MergeLogType::TimeSeries) {
      propertyToReset = item.first + TIME_SERIES_SUFFIX;
    } else if (item.second.type == MergeLogType::List) {
      propertyToReset = item.first + LIST_SUFFIX;
    } else {
      return;
    }

    Property *outWSProperty = ws->mutableRun().getProperty(propertyToReset);
    item.second.property = std::shared_ptr<Property>(outWSProperty->clone());
  }
}

/**
 * Resets the sample logs in the workspace to the values in the map.
 *
 * @param ws the merged workspace to reset the sample logs for
 */
void SampleLogsBehaviour::resetSampleLogs(const API::MatrixWorkspace_sptr &ws) {
  for (auto item : m_logMap) {
    std::string propertyToReset = item.first;

    if (item.second.type == MergeLogType::TimeSeries) {
      propertyToReset = item.first + TIME_SERIES_SUFFIX;
      auto property =
          std::unique_ptr<Kernel::Property>(item.second.property->clone());
      ws->mutableRun().addProperty(std::move(property), true);
    } else if (item.second.type == MergeLogType::List) {
      propertyToReset = item.first + LIST_SUFFIX;
      ws->mutableRun()
          .getProperty(propertyToReset)
          ->setValue(item.second.property->value());
    } else {
      return;
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
