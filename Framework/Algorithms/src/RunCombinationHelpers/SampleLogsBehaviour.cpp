// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/RunCombinationHelpers/SampleLogsBehaviour.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/FloatingPointComparison.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace API;

namespace {
std::string generateDifferenceMessage(const std::string &item, const std::string &wsName, const std::string &wsValue,
                                      const std::string &firstValue) {
  std::stringstream stringstream;
  stringstream << "Item \"" << item << "\" has different values in workspaces! Found: " << wsValue << " in workspace "
               << wsName << " but the value in the first workspace was: " << firstValue << "." << std::endl;
  return stringstream.str();
}
} // namespace

// Names and docs from the properties allowing to override the default (IPF
// controlled) merging behaviour.
// These are common between e.g. MergeRuns and ConjoinXRuns.
const std::string SampleLogsBehaviour::TIME_SERIES_PROP = "SampleLogsTimeSeries";
const std::string SampleLogsBehaviour::TIME_SERIES_DOC = "A comma separated list of the sample logs to merge into a "
                                                         "time series. The initial times are taken as the start times "
                                                         "for the run. Sample logs must be numeric.";
const std::string SampleLogsBehaviour::LIST_PROP = "SampleLogsList";
const std::string SampleLogsBehaviour::LIST_DOC = "A comma separated list of the sample logs to merge into a "
                                                  "list.";
const std::string SampleLogsBehaviour::WARN_PROP = "SampleLogsWarn";
const std::string SampleLogsBehaviour::WARN_DOC = "A comma separated list of the sample "
                                                  "logs to generate a warning if "
                                                  "different when merging.";
const std::string SampleLogsBehaviour::WARN_TOL_PROP = "SampleLogsWarnTolerances";
const std::string SampleLogsBehaviour::WARN_TOL_DOC = "The tolerances for warning if sample logs are different. "
                                                      "Can either be empty for a comparison of the strings, a "
                                                      "single value for all warn sample logs, or a comma "
                                                      "separated list of values (must be the same length as "
                                                      "SampleLogsWarn).";
const std::string SampleLogsBehaviour::FAIL_PROP = "SampleLogsFail";
const std::string SampleLogsBehaviour::FAIL_DOC = "The sample logs to fail if different "
                                                  "when merging. If there is a "
                                                  "difference the run is skipped.";
const std::string SampleLogsBehaviour::FAIL_TOL_PROP = "SampleLogsFailTolerances";
const std::string SampleLogsBehaviour::FAIL_TOL_DOC = "The tolerances for failing if sample logs are different. "
                                                      "Can either be empty for a comparison of the strings, a "
                                                      "single value for all fail sample logs, or a comma "
                                                      "separated list of values (must be the same length as "
                                                      "SampleLogsFail).";
const std::string SampleLogsBehaviour::SUM_PROP = "SampleLogsSum";
const std::string SampleLogsBehaviour::SUM_DOC = "A comma separated list of the sample "
                                                 "logs to sum into a single entry.  "
                                                 "Sample logs must be numeric.";

/**
 * Create and initialise an object that is responsbile for keeping track of the
 * merge types, and performing the merge or warning/error for sample logs when
 * calling MergeRuns.
 *
 * @param ws the base workspace that the other workspaces are merged into
 * @param logger the logger from the parent algorithm
 * @param logEntries the sample log names to merge given by the user which
 * override names given by IPF parameters
 * @param parName the parameter names which specify the sample log sames to
 * merge given be the IPF
 */
SampleLogsBehaviour::SampleLogsBehaviour(const MatrixWorkspace_sptr &ws, Logger &logger,
                                         const SampleLogNames &logEntries, ParameterName parName)
    : parameterNames(std::move(parName)), m_logger(logger) {
  setSampleMap(m_logMap, MergeLogType::Sum, logEntries.sampleLogsSum, *ws);
  setSampleMap(m_logMap, MergeLogType::TimeSeries, logEntries.sampleLogsTimeSeries, *ws);
  setSampleMap(m_logMap, MergeLogType::List, logEntries.sampleLogsList, *ws);
  setSampleMap(m_logMap, MergeLogType::Warn, logEntries.sampleLogsWarn, *ws, logEntries.sampleLogsWarnTolerances);
  setSampleMap(m_logMap, MergeLogType::Fail, logEntries.sampleLogsFail, *ws, logEntries.sampleLogsFailTolerances);

  SampleLogsMap instrumentMap;
  this->createSampleLogsMapsFromInstrumentParams(instrumentMap, *ws);

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
void SampleLogsBehaviour::createSampleLogsMapsFromInstrumentParams(SampleLogsMap &map, MatrixWorkspace &ws) {
  std::string params = ws.getInstrument()->getParameterAsString(parameterNames.SUM_MERGE, false);
  setSampleMap(map, MergeLogType::Sum, params, ws, "", true);

  params = ws.getInstrument()->getParameterAsString(parameterNames.TIME_SERIES_MERGE, false);
  setSampleMap(map, MergeLogType::TimeSeries, params, ws, "", true);

  params = ws.getInstrument()->getParameterAsString(parameterNames.LIST_MERGE, false);
  setSampleMap(map, MergeLogType::List, params, ws, "", true);

  params = ws.getInstrument()->getParameterAsString(parameterNames.WARN_MERGE, false);
  std::string paramsTolerances;
  paramsTolerances = ws.getInstrument()->getParameterAsString(parameterNames.WARN_MERGE_TOLERANCES, false);
  setSampleMap(map, MergeLogType::Warn, params, ws, paramsTolerances, true);

  params = ws.getInstrument()->getParameterAsString(parameterNames.FAIL_MERGE, false);
  paramsTolerances = ws.getInstrument()->getParameterAsString(parameterNames.FAIL_MERGE_TOLERANCES, false);
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
 * @param skipIfInPrimaryMap whether to skip if in the member variable map
 * (optional, default false)
 */
void SampleLogsBehaviour::setSampleMap(SampleLogsMap &map, const MergeLogType &mergeType, const std::string &params,
                                       MatrixWorkspace &ws, const std::string &paramsTolerances,
                                       bool skipIfInPrimaryMap) {

  StringTokenizer tokenizer(params, ",", StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY);
  StringTokenizer tokenizerTolerances(paramsTolerances, ",",
                                      StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY);

  auto tolerancesStringVector = tokenizerTolerances.asVector();

  std::vector<double> tolerancesVector = createTolerancesVector(tokenizer.asVector().size(), tolerancesStringVector);

  auto i = tokenizer.begin();
  auto j = tolerancesVector.begin();

  for (; i != tokenizer.end() && j != tolerancesVector.end(); ++i, ++j) {
    auto item = *i;
    auto tolerance = *j;

    // Check 1: Does the key exist in the primary map? If so ignore it and
    // continue.
    if (skipIfInPrimaryMap && (m_logMap.count(SampleLogsKey(item, mergeType)) != 0)) {
      continue;
    }

    // Check 2: If the key (sample log name) already exists in this map throw an
    // error.
    if (map.count(SampleLogsKey(item, mergeType)) != 0) {
      throw std::invalid_argument("Error when making list of merge items, sample log \"" + item +
                                  "\" defined more than once!");
    }

    // Check 3: If the sample log is one that should not be combined with
    // others, check other incompatible sample logs do not exist too.
    std::set<MergeLogType> uncombinableLogs = {MergeLogType::Sum, MergeLogType::TimeSeries, MergeLogType::List};
    if (uncombinableLogs.count(mergeType) != 0) {
      bool skipLog = false;

      uncombinableLogs.erase(mergeType);
      for (auto &logType : uncombinableLogs) {
        if (map.count(SampleLogsKey(item, logType)) > 0)
          throw std::invalid_argument("Error when making list of merge items, sample log " + item +
                                      " being used for two incompatible merge types!");
        if (skipIfInPrimaryMap && m_logMap.count(SampleLogsKey(item, logType)) > 0)
          skipLog = true;
      }

      if (skipLog)
        continue;
    }

    // Check 4: Does the sample log exist? If not log an error but continue.
    std::shared_ptr<Property> prop;
    try {
      prop = std::shared_ptr<Property>(ws.getLog(item)->clone());
    } catch (std::invalid_argument &) {
      m_logger.error() << "Could not merge sample log \"" << item
                       << "\", does not exist in workspace! This sample log will be ignored." << std::endl;
      continue;
    }

    // Check 5: Can the property be converted to a double? If not, and sum or
    // time series case, log an error but continue.
    bool isNumeric;
    double value = 0.0;
    isNumeric = setNumericValue(item, ws, value);
    if (!isNumeric && (mergeType == MergeLogType::Sum || mergeType == MergeLogType::TimeSeries)) {
      m_logger.error() << item
                       << " could not be converted to a numeric type. "
                          "This sample log will be ignored.\n"
                       << std::endl;
      continue;
    }

    // For a TimeSeries or a List we need to add a new property to the workspace
    if (mergeType == MergeLogType::TimeSeries) {
      prop = addPropertyForTimeSeries(item, value, ws);
    } else if (mergeType == MergeLogType::List) {
      prop = addPropertyForList(item, prop->value(), ws);
    }

    // Finally add the key-value pair to the map
    map[SampleLogsKey(item, mergeType)] = {prop, tolerance, isNumeric};
  }
}

/**
 * Creates a vector of tolerances with the same size as the number of sample
 * logs for the merge type. If the number of names and tolerances is the same
 * the vector is filled with the tolerances for each name. If no tolerances were
 * specified all tolerances are set to -1, and if one tolerance is given all
 * tolerances are set to that value.
 *
 * @param numberNames the number of sample log names
 * @param tolerances a vector containing strings with the tolerances
 * @return a vector of doubles of size numberNames
 */
std::vector<double> SampleLogsBehaviour::createTolerancesVector(size_t numberNames,
                                                                const std::vector<std::string> &tolerances) {
  size_t numberTolerances = tolerances.size();

  std::vector<double> tolerancesVector(numberNames);

  if (numberNames == numberTolerances && numberTolerances > 1) {
    try {
      std::transform(tolerances.begin(), tolerances.end(), tolerancesVector.begin(),
                     [](const std::string &val) { return std::stod(val); });
    } catch (std::invalid_argument &) {
      throw std::invalid_argument("Error when creating tolerances vector. "
                                  "Please ensure each comma separated value is "
                                  "numeric.");
    } catch (std::out_of_range &) {
      throw std::out_of_range("Error when creating tolerances vector. Please "
                              "ensure each comma separated value is within "
                              "double precision range.");
    }
    if (std::any_of(tolerancesVector.cbegin(), tolerancesVector.cend(), [](const auto value) { return value < 0.; })) {
      throw std::out_of_range("Error when creating tolerances vector. Please "
                              "ensure all tolerance values are positive.");
    }
  } else if (tolerances.empty()) {
    std::fill(tolerancesVector.begin(), tolerancesVector.end(), -1.0);
  } else if (numberTolerances == 1) {
    double value;
    try {
      value = std::stod(tolerances.front());
      std::fill(tolerancesVector.begin(), tolerancesVector.end(), value);
    } catch (std::invalid_argument &) {
      throw std::invalid_argument("The single tolerance value requested can "
                                  "not be converted to a number. Please ensure "
                                  "it is a single number, or a comma separated "
                                  "list of numbers.");
    } catch (std::out_of_range &) {
      throw std::out_of_range("The single tolerance value requested can not be "
                              "converted to a double. Please ensure tolerance "
                              "is within double precision range.");
    }
    if (value < 0)
      throw std::out_of_range("The single tolerance value requested is "
                              "negative. Please ensure it is positive.");
  } else {
    throw std::invalid_argument("Invalid length of tolerances, found " + std::to_string(numberTolerances) +
                                " tolerance values but " + std::to_string(numberNames) + " names.");
  }

  return tolerancesVector;
}

/**
 * Adds a property to the workspace provided for a TimeSeries merge type.
 *
 * @param item the name of the sample log to merge as a time series
 * @param value the numeric value of the sample log in the first workspace
 * @param ws the first workspace in the merge
 * @return a shared pointer to the added property
 */
std::shared_ptr<Property> SampleLogsBehaviour::addPropertyForTimeSeries(const std::string &item, const double value,
                                                                        MatrixWorkspace &ws) {
  std::shared_ptr<Property> returnProp;
  const std::string originalUnit = ws.getLog(item)->units();
  try {
    // See if property exists as a TimeSeriesLog already - merging an output of
    // MergeRuns
    ws.run().getTimeSeriesProperty<double>(item);
    returnProp.reset(ws.getLog(item)->clone());
  } catch (std::invalid_argument &) {
    // Property does not already exist, so add it setting the first entry
    std::unique_ptr<Kernel::TimeSeriesProperty<double>> timeSeriesProp(new TimeSeriesProperty<double>(item));
    std::string startTime = ws.run().startTime().toISO8601String();

    timeSeriesProp->addValue(startTime, value);
    ws.mutableRun().addProperty(std::move(timeSeriesProp), true);

    returnProp.reset(ws.getLog(item)->clone());
  }
  ws.getLog(item)->setUnits(originalUnit); // we lost the unit of the workspace
  return returnProp;
}

/**
 * Adds a property to the workspace provided for a List merge type.
 *
 * @param item the name of the sample log to merge as a list
 * @param value the string value of the sample log in the first workspace
 * @param ws the first workspace in the merge
 * @return a shared pointer to the added property
 */
std::shared_ptr<Property> SampleLogsBehaviour::addPropertyForList(const std::string &item, const std::string &value,
                                                                  MatrixWorkspace &ws) {
  std::shared_ptr<Property> returnProp;

  const std::string originalUnit = ws.getLog(item)->units();
  // See if property exists already - merging an output of the calling algorithm
  returnProp.reset(ws.getLog(item)->clone());
  if (returnProp->type() != "string") {
    ws.mutableRun().addProperty(item, value, true);
    returnProp.reset(ws.getLog(item)->clone());
  }
  ws.getLog(item)->setUnits(originalUnit); // we lost the unit of the workspace
  return returnProp;
}

/**
 * Tries to set the numeric value of a property.
 *
 * @param item the name of the sample log
 * @param ws the first workspace in the merge
 * @param value the value of the sample log (if it could be set)
 * @return true if the sample log could be converted to a double, false
 * otherwise
 */
bool SampleLogsBehaviour::setNumericValue(const std::string &item, const MatrixWorkspace &ws, double &value) {
  bool isNumeric;

  try {
    value = ws.getLogAsSingleValue(item);
    isNumeric = true;
  } catch (std::invalid_argument &) {
    isNumeric = false;
  }

  return isNumeric;
}

/**
 * Updates the sample logs according to the requested behaviour.
 *
 * @param addeeWS the workspace being merged
 * @param outWS the workspace the others are merged into
 */
void SampleLogsBehaviour::mergeSampleLogs(const MatrixWorkspace_sptr &addeeWS, const MatrixWorkspace_sptr &outWS) {
  for (const auto &item : m_logMap) {
    const std::string &logName = item.first.first;

    const Property *addeeWSProperty = addeeWS->getLog(logName);
    const std::string originalUnit = addeeWS->getLog(logName)->units();

    double addeeWSNumericValue = 0.;
    double outWSNumericValue = 0.;

    try {
      addeeWSNumericValue = addeeWS->getLogAsSingleValue(logName);
      outWSNumericValue = outWS->getLogAsSingleValue(logName);
    } catch (std::invalid_argument &) {
      if (item.second.isNumeric) {
        throw std::invalid_argument(logName + " could not be converted to a numeric type");
      }
    }

    switch (item.first.second) {
    case MergeLogType::Sum: {
      this->updateSumProperty(addeeWSNumericValue, outWSNumericValue, *outWS, logName);
      break;
    }
    case MergeLogType::TimeSeries: {
      this->updateTimeSeriesProperty(*addeeWS, *outWS, logName);
      break;
    }
    case MergeLogType::List: {
      this->updateListProperty(*addeeWS, *outWS, logName);
      break;
    }
    case MergeLogType::Warn:
      this->checkWarnProperty(*addeeWS, addeeWSProperty, item.second, addeeWSNumericValue, outWSNumericValue, logName);
      break;
    case MergeLogType::Fail:
      this->checkErrorProperty(*addeeWS, addeeWSProperty, item.second, addeeWSNumericValue, outWSNumericValue, logName);
      break;
    }
    outWS->getLog(logName)->setUnits(originalUnit);
  }
}

/**
 * Perform the update for a sum property, adding a new value to the existing
 *one. Skipped if the time series log entry is in the addeeWS.
 *
 * @param addeeWSNumericValue the sample log value from the workspace being
 *merged
 * @param outWSNumericValue the sample log value for the merged workspace
 * @param outWS the workspace the others are merged into
 * @param name the name of the property
 */
void SampleLogsBehaviour::updateSumProperty(double addeeWSNumericValue, double outWSNumericValue,
                                            MatrixWorkspace &outWS, const std::string &name) {
  outWS.mutableRun().addProperty(name, addeeWSNumericValue + outWSNumericValue, true);
}

/**
 * Perform the update for a time series property, adding a new value to the
 *existing time series property.
 *
 * @param addeeWS the workspace being merged
 * @param outWS the workspace the others are merged into
 * @param name the name of the property
 */
void SampleLogsBehaviour::updateTimeSeriesProperty(const MatrixWorkspace &addeeWS, const MatrixWorkspace &outWS,
                                                   const std::string &name) {
  auto timeSeriesProp = outWS.run().getTimeSeriesProperty<double>(name);
  try {
    const auto addeeTimeSeries = addeeWS.run().getTimeSeriesProperty<double>(name);
    timeSeriesProp->merge(addeeTimeSeries);
  } catch (std::invalid_argument &) {
    Types::Core::DateAndTime startTime = addeeWS.run().startTime();
    double value = addeeWS.run().getLogAsSingleValue(name);
    timeSeriesProp->addValue(startTime, value);
  }
  // Remove this to supress a warning, we will put it back after adding the
  // workspaces in MergeRuns
  const Property *addeeWSProperty = addeeWS.run().getProperty(name);
  m_addeeLogMap.emplace_back(std::shared_ptr<Property>(addeeWSProperty->clone()));
}

/**
 * Perform the update for a list property, appending a new value to the existing
 *string. If the list log entry is in the addeeWS the list log entry is merged
 *instead.
 *
 * @param addeeWS the workspace being merged
 * @param outWS the workspace the others are merged into
 * @param name the name of the property
 */
void SampleLogsBehaviour::updateListProperty(const MatrixWorkspace &addeeWS, MatrixWorkspace &outWS,
                                             const std::string &name) {
  const std::string addeeWSVal = addeeWS.getLog(name)->value();
  const std::string outWSVal = outWS.run().getProperty(name)->value();
  outWS.mutableRun().addProperty(name, outWSVal + ", " + addeeWSVal, true);
}

/**
 * Performs the check to see if a warning should be generated because logs are
 * different. Performs a numeric comparison if a tolerance is set and the log is
 * a number, else performs a string comparison.
 *
 * @param addeeWS the workspace being merged
 * @param addeeWSProperty the property value of the workspace being merged
 * @param behaviour the merge behaviour struct, containing information about the
 *merge
 * @param addeeWSNumericValue a double for the addeeWS value (0 if it is not
 *numeric)
 * @param outWSNumericValue a double for the outWS value (0 if it is not
 *numeric)
 * @param name the name of the sample log to check
 */
void SampleLogsBehaviour::checkWarnProperty(const MatrixWorkspace &addeeWS, const Property *addeeWSProperty,
                                            const SampleLogBehaviour &behaviour, const double addeeWSNumericValue,
                                            const double outWSNumericValue, const std::string &name) {

  if (!isWithinTolerance(behaviour, addeeWSNumericValue, outWSNumericValue) &&
      !stringPropertiesMatch(behaviour, addeeWSProperty)) {
    m_logger.warning() << generateDifferenceMessage(name, addeeWS.getName(), addeeWSProperty->value(),
                                                    behaviour.property->value());
  }
}

/**
 ** Performs the check to see if an error should be generated because logs are
 * different. Performs a numeric comparison if a tolerance is set and the log is
 * a number, else performs a string comparison.
 *
 * @param addeeWS the workspace being merged
 * @param addeeWSProperty the property value of the workspace being merged
 * @param behaviour the merge behaviour struct, containing information about the
 *merge
 * @param addeeWSNumericValue a double for the addeeWS value (0 if it is not
 *numeric)
 * @param outWSNumericValue a double for the outWS value (0 if it is not
 *numeric)
 * @param name the name of the sample log to check
 */
void SampleLogsBehaviour::checkErrorProperty(const MatrixWorkspace &addeeWS, const Property *addeeWSProperty,
                                             const SampleLogBehaviour &behaviour, const double addeeWSNumericValue,
                                             const double outWSNumericValue, const std::string &name) {

  if (!isWithinTolerance(behaviour, addeeWSNumericValue, outWSNumericValue) &&
      !stringPropertiesMatch(behaviour, addeeWSProperty)) {
    throw std::invalid_argument(
        generateDifferenceMessage(name, addeeWS.getName(), addeeWSProperty->value(), behaviour.property->value()));
  }
}

/**
 * Check if a sample log value in the addee workspace is numeric and within
 * tolerances.
 *
 * @param behaviour the SampleLogBehaviour item to check
 * @param addeeWSNumericValue the value in the workspace being added
 * @param outWSNumericValue the value in the first workspace
 * @return true if the sample log is numeric and within any tolerance set, or if
 * strings match, false otherwise
 */
bool SampleLogsBehaviour::isWithinTolerance(const SampleLogBehaviour &behaviour, const double addeeWSNumericValue,
                                            const double outWSNumericValue) {
  if (behaviour.isNumeric && behaviour.tolerance > 0.0) {
    return Kernel::withinAbsoluteDifference(addeeWSNumericValue, outWSNumericValue, behaviour.tolerance);
  }

  return false;
}

/**
 * Check if a sample log value in the addee workspace matches one in the first
 * workspace.
 *
 * @param behaviour the SampleLogBehaviour item to check
 * @param addeeWSProperty a pointer to the property in the workspace being added
 * @return true if the sample logs match, false otherwise
 */
bool SampleLogsBehaviour::stringPropertiesMatch(const SampleLogBehaviour &behaviour, const Property *addeeWSProperty) {
  return behaviour.property->value() == addeeWSProperty->value();
}

/**
 * Set the values in the map to be the same as those in the output workspace.
 *
 * @param outWS the merged workspace
 */
void SampleLogsBehaviour::setUpdatedSampleLogs(const MatrixWorkspace_sptr &outWS) {
  for (auto &item : m_logMap) {
    std::string propertyToReset = item.first.first;

    if (item.first.second == MergeLogType::Warn || item.first.second == MergeLogType::Fail) {
      continue;
    }

    const Property *outWSProperty = outWS->run().getProperty(propertyToReset);
    item.second.property = std::shared_ptr<Property>(outWSProperty->clone());
  }
}

/**
 * When doing a time series merge we need to remove, then add back the sample
 *log in the addee workspace to supress a warning about it not being a
 *TimeSeriesProperty. Here we remove the original property.
 *
 * @param addeeWS the workspace being merged
 */
void SampleLogsBehaviour::removeSampleLogsFromWorkspace(const MatrixWorkspace_sptr &addeeWS) {
  for (const auto &prop : m_addeeLogMap) {
    const auto &propName = prop->name();
    addeeWS->mutableRun().removeProperty(propName);
  }
}

/**
 * When doing a time series merge we need to remove, then add back the sample
 *log in the addee workspace to supress a warning about it not being a
 *TimeSeriesProperty. Here we add back the original property, as the original
 *workspace should remain unchanged.
 *
 * @param addeeWS the workspace being merged
 */
void SampleLogsBehaviour::readdSampleLogToWorkspace(const MatrixWorkspace_sptr &addeeWS) {
  for (const auto &item : m_addeeLogMap) {
    auto property = std::unique_ptr<Kernel::Property>(item->clone());
    addeeWS->mutableRun().addProperty(std::move(property));
  }
  m_addeeLogMap.clear();
}

/**
 * Resets the sample logs in the workspace to the values in the map.
 *
 * @param ws the merged workspace to reset the sample logs for
 */
void SampleLogsBehaviour::resetSampleLogs(const MatrixWorkspace_sptr &ws) {
  for (auto const &item : m_logMap) {
    std::string const &propertyToReset = item.first.first;

    if (item.first.second == MergeLogType::TimeSeries) {
      auto property = std::unique_ptr<Kernel::Property>(item.second.property->clone());
      ws->mutableRun().addProperty(std::move(property), true);
    } else if (item.first.second == MergeLogType::Sum || item.first.second == MergeLogType::List) {
      ws->mutableRun().getProperty(propertyToReset)->setValue(item.second.property->value());
    }
  }
}

} // namespace Mantid::Algorithms
