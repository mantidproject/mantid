// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/LogManager.h"
#include "MantidKernel/Cache.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyNexus.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <nexus/NeXusFile.hpp>
#include <numeric>

namespace Mantid::API {

using namespace Kernel;
using namespace Types::Core;

namespace {
/// static logger
Logger g_log("LogManager");

const std::string START_TIME_NAME("start_time");
const std::string END_TIME_NAME("end_time");

/// Templated method to convert property to double
template <typename T> bool convertSingleValue(const Property *property, double &value) {
  if (auto log = dynamic_cast<const PropertyWithValue<T> *>(property)) {
    value = static_cast<double>(*log);
    return true;
  } else {
    return false;
  }
}

bool convertSingleValueToDouble(const Property *property, double &value) {
  // Order these with double and int first, and less likely options later.
  // The first one to succeed short-circuits and the value is returned.
  // If all fail, returns false.
  return convertSingleValue<double>(property, value) || convertSingleValue<int32_t>(property, value) ||
         convertSingleValue<int64_t>(property, value) || convertSingleValue<uint32_t>(property, value) ||
         convertSingleValue<uint64_t>(property, value) || convertSingleValue<float>(property, value);
}

/// Templated method to convert time series property to single double
template <typename T>
bool convertTimeSeriesToDouble(const Property *property, double &value, const Math::StatisticType &function,
                               const Kernel::TimeROI *timeRoi = nullptr) {
  if (const auto *log = dynamic_cast<const ITimeSeriesProperty *>(property)) {
    value = log->extractStatistic(function, timeRoi);
    return true;
  } else {
    return false;
  }
}

/// Templated method to convert a property to a single double
template <typename T>
bool convertPropertyToDouble(const Property *property, double &value, const Math::StatisticType &function,
                             const Kernel::TimeROI *timeRoi = nullptr) {
  return convertSingleValue<T>(property, value) || convertTimeSeriesToDouble<T>(property, value, function, timeRoi);
}

/// Converts a property to a single double
bool convertPropertyToDouble(const Property *property, double &value, const Math::StatisticType &function,
                             const Kernel::TimeROI *timeRoi = nullptr) {
  // Order these with double and int first, and less likely options later.
  // The first one to succeed short-circuits and the value is returned.
  // If all fail, returns false.
  return convertPropertyToDouble<double>(property, value, function, timeRoi) ||
         convertPropertyToDouble<int32_t>(property, value, function, timeRoi) ||
         convertPropertyToDouble<int64_t>(property, value, function, timeRoi) ||
         convertPropertyToDouble<uint32_t>(property, value, function, timeRoi) ||
         convertPropertyToDouble<uint64_t>(property, value, function, timeRoi) ||
         convertPropertyToDouble<float>(property, value, function, timeRoi);
}
} // namespace

/// Name of the log entry containing the proton charge when retrieved using
/// getProtonCharge
const char *LogManager::PROTON_CHARGE_LOG_NAME = "gd_prtn_chrg";
//----------------------------------------------------------------------
// Public member functions
//----------------------------------------------------------------------

LogManager::LogManager()
    : m_manager(std::make_unique<Kernel::PropertyManager>()), m_timeroi(std::make_unique<Kernel::TimeROI>()),
      m_singleValueCache(
          std::make_unique<Kernel::Cache<std::pair<std::string, Kernel::Math::StatisticType>, double>>()) {}

LogManager::LogManager(const LogManager &other)
    : m_manager(std::make_unique<Kernel::PropertyManager>(*other.m_manager)),
      m_timeroi(std::make_unique<Kernel::TimeROI>(*other.m_timeroi)),
      m_singleValueCache(std::make_unique<Kernel::Cache<std::pair<std::string, Kernel::Math::StatisticType>, double>>(
          *other.m_singleValueCache)) {}

// Defined as default in source for forward declaration with std::unique_ptr.
LogManager::~LogManager() = default;

LogManager &LogManager::operator=(const LogManager &other) {
  *m_manager = *other.m_manager;
  *m_timeroi = *other.m_timeroi;
  m_singleValueCache = std::make_unique<Kernel::Cache<std::pair<std::string, Kernel::Math::StatisticType>, double>>(
      *other.m_singleValueCache);
  return *this;
}

/**
 * Set the run start and end
 * @param start :: The run start
 * @param end :: The run end
 */
void LogManager::setStartAndEndTime(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &end) {
  this->addProperty<std::string>(START_TIME_NAME, start.toISO8601String(), true);
  this->addProperty<std::string>(END_TIME_NAME, end.toISO8601String(), true);
}

/**
 * Find run end time as determined by the following priorities:
 * 1. "start_time" property
 * 2. "run_start" property
 * 3. first valid log time in "proton_charge" property
 *  @returns The start time of the run
 *  @throws std::runtime_error if start time cannot be determined
 */
const Types::Core::DateAndTime LogManager::startTime() const {
  if (hasProperty(START_TIME_NAME)) {
    try {
      DateAndTime start_time(getProperty(START_TIME_NAME)->value());
      if (start_time != DateAndTime::GPS_EPOCH) {
        return start_time;
      }
    } catch (std::invalid_argument &) { /*Swallow and move on*/
    }
  }

  const std::string run_start_prop("run_start");
  if (hasProperty(run_start_prop)) {
    try {
      DateAndTime start_time(getProperty(run_start_prop)->value());
      if (start_time != DateAndTime::GPS_EPOCH) {
        return start_time;
      }
    } catch (std::invalid_argument &) { /*Swallow and move on*/
    }
  }

  std::string errorMsg{"No valid start time has been set for this run."};
  if (hasValidProtonChargeLog(errorMsg))
    return getFirstPulseTime();

  throw std::runtime_error(errorMsg);
}

/** Find run end time as determined by the following priorities:
 * 1. "end_time" property
 * 2. "run_end" property
 * 3. last log time in "proton_charge" property
 *  @returns The end time of the run
 *  @throws std::runtime_error if end time cannot be determined
 */
const Types::Core::DateAndTime LogManager::endTime() const {
  if (hasProperty(END_TIME_NAME)) {
    try {
      return DateAndTime(getProperty(END_TIME_NAME)->value());
    } catch (std::invalid_argument &) { /*Swallow and move on*/
    }
  }

  const std::string run_end_prop("run_end");
  if (hasProperty(run_end_prop)) {
    try {
      return DateAndTime(getProperty(run_end_prop)->value());
    } catch (std::invalid_argument &) { /*Swallow and move on*/
    }
  }

  std::string errorMsg{"No valid end time has been set for this run."};
  if (hasValidProtonChargeLog(errorMsg))
    return getLastPulseTime();

  throw std::runtime_error(errorMsg);
}

bool LogManager::hasStartTime() const {
  std::string temp;
  return hasProperty(START_TIME_NAME) || hasProperty("run_start") || hasValidProtonChargeLog(temp);
}

bool LogManager::hasEndTime() const {
  std::string temp;
  return hasProperty(END_TIME_NAME) || hasProperty("run_end") || hasValidProtonChargeLog(temp);
}

/** Return the time of the first pulse received, by accessing the run's
 * sample logs to find the proton_charge.
 *
 * @return the time of the first valid pulse.
 * @throw runtime_error if the time of the first pulse is not available.
 */
const DateAndTime LogManager::getFirstPulseTime() const {
  TimeSeriesProperty<double> *log =
      getTimeSeriesProperty<double>("proton_charge"); // guaranteed to be a valid pointer, if the call succeeds
  if (log->realSize() == 0)
    throw std::runtime_error("First pulse time is not available. Log \"proton_charge\" is empty.");

  // NOTE, this method has been migrated from MatrixWorkspace class, where it had a comment:
  // "Pulse times before 1991 (up to 100) are skipped. This is to avoid
  // a DAS bug at SNS around Mar 2011 where the first pulse time is Jan 1, 1990."
  // There was no explanation why 100 was picked as the maximum number of times to skip.
  // In the refactored algorithm below we keep 100 as is.
  const size_t maxSkip{100};
  const DateAndTime reference("1991-01-01T00:00:00");
  const std::vector<DateAndTime> &times = log->timesAsVector();
  size_t index;
  const size_t maxIndex = std::min(static_cast<size_t>(log->realSize()), maxSkip);
  for (index = 0; index < maxIndex; index++) {
    if (times[index] >= reference)
      break;
  }

  return times[index];
}

/** Return the time of the last pulse received, by accessing the run's
 * sample logs to find the proton_charge
 *
 * @return the time of the last pulse.
 * @throw runtime_error if the time of the last pulse is not available.
 */
const DateAndTime LogManager::getLastPulseTime() const {
  TimeSeriesProperty<double> *log =
      getTimeSeriesProperty<double>("proton_charge"); // guaranteed to be a valid pointer, if the call succeeds
  if (log->realSize() == 0)
    throw std::runtime_error("Last pulse time is not available. Log \"proton_charge\" is empty.");
  return log->lastTime();
}

/** Check if the proton_charge log exists, is of valid type, and is not empty.
 *
 * @param error :: extended error message in case the check fails
 * @return true if a valid proton_charge log exists, false otherwise.
 */
bool LogManager::hasValidProtonChargeLog(std::string &error) const {
  const std::string log_name{"proton_charge"};
  if (!hasProperty(log_name)) {
    error += " Log " + log_name + " is not found.";
    return false;
  }

  Kernel::Property *prop = getProperty(log_name);
  TimeSeriesProperty<double> *log;
  if (!(log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(prop))) {
    error += " Log " + log_name + " is not a time series of floating-point values.";
    return false;
  }

  if (log->realSize() == 0) {
    error += " Log " + log_name + " is empty.";
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------------------------
/**
 * Filter out all TimeSeriesProperty log entries outside of the given absolute time range [start,stop),
 * except for the entries immediately before and after the time range.
 *
 * @param start :: Absolute start time.
 * @param stop :: Absolute stop time.
 */
void LogManager::filterByTime(const Types::Core::DateAndTime start, const Types::Core::DateAndTime stop) {
  this->setTimeROI(TimeROI(start, stop));
  this->removeDataOutsideTimeROI();
}

/**
 * Create a partial copy of this object such that every time series property is cloned according to the input TimeROI.
 * A partially cloned time series property should include all time values enclosed by the ROI regions,
 * each defined as [roi_begin,roi_end], plus the values immediately before and after an ROI region, if available.
 * Properties that are not time series will be cloned with no changes.
 * @param timeROI :: a series of time regions used to determine which time series values should be included in the copy.
 */
LogManager *LogManager::cloneInTimeROI(const Kernel::TimeROI &timeROI) {
  LogManager *newMgr = new LogManager();
  newMgr->m_manager = std::unique_ptr<PropertyManager>(m_manager->cloneInTimeROI(timeROI));

  // This LogManager object may have filtered out some data previously, in which case it would be holding the TimeROI
  // used. Therefore, the cloned object's TimeROI should be an intersection of the input TimeROI with the one being
  // held.
  TimeROI outputTimeROI(timeROI);
  outputTimeROI.update_or_replace_intersection(*m_timeroi);
  newMgr->m_timeroi = std::make_unique<Kernel::TimeROI>(outputTimeROI);

  return newMgr;
}

/**
 * Copy properties from another LogManager object. Filter copied time series properties according to the input TimeROI.
 * @param other :: another LogManager object.
 * @param timeROI :: a series of time regions used to determine which time series values should be included in the copy.
 */
void LogManager::copyAndFilterProperties(const LogManager &other, const Kernel::TimeROI &timeROI) {
  this->m_manager = std::unique_ptr<PropertyManager>(other.m_manager->cloneInTimeROI(timeROI));
  this->setTimeROI(timeROI);
  this->clearSingleValueCache();
}

/**
 * For time series properties, remove time values outside of this object's TimeROI.
 * Each TimeROI region is defined as [roi_begin,roi_end]. However, keep the values
 * immediately before and after each timeROI region, if available.
 */
void LogManager::removeDataOutsideTimeROI() {
  m_manager->removeDataOutsideTimeROI(*m_timeroi);
  this->clearSingleValueCache();
}

//----------------------------------------------------------------------------------------------
/**
 * Filter the run by the given boolean log. It replaces all time
 * series properties with filtered time series properties
 * @param filter :: A LogFilter instance to filter each log on
 * @param excludedFromFiltering :: A string list of logs that
 * will be excluded from filtering
 */
void LogManager::filterByLog(Mantid::Kernel::LogFilter *filter, const std::vector<std::string> &excludedFromFiltering) {
  // This will invalidate the cache
  this->clearSingleValueCache();
  m_manager->filterByProperty(filter, excludedFromFiltering);
}

//-----------------------------------------------------------------------------------------------
/**
 * Add data to the object in the form of a property
 * @param prop :: A pointer to a property whose ownership is transferred to
 * this
 * object
 * @param overwrite :: If true, a current value is overwritten. (Default:
 * False)
 */
void LogManager::addProperty(std::unique_ptr<Kernel::Property> prop, bool overwrite) {
  // Make an exception for the proton charge
  // and overwrite its value as we don't want to store the proton charge in two
  // separate locations
  // Similar we don't want more than one run_title
  std::string name = prop->name();
  if (hasProperty(name) && (overwrite || prop->name() == PROTON_CHARGE_LOG_NAME || prop->name() == "run_title")) {
    removeProperty(name);
  }
  m_manager->declareProperty(std::move(prop), "");
}

//-----------------------------------------------------------------------------------------------
/**
 * Returns true if the named property exists
 * @param name :: The name of the property
 * @return True if the property exists, false otherwise
 */
bool LogManager::hasProperty(const std::string &name) const { return m_manager->existsProperty(name); }

//-----------------------------------------------------------------------------------------------
/**
 * Remove a named property
 * @param name :: The name of the property
 * @param delProperty :: If true the property is deleted (default=true)
 */

void LogManager::removeProperty(const std::string &name, bool delProperty) {
  // Remove any cached entries for this log. Need to make this more general
  for (unsigned int stat = 0; stat < 7; ++stat) {
    m_singleValueCache->removeCache(std::make_pair(name, static_cast<Math::StatisticType>(stat)));
  }
  m_manager->removeProperty(name, delProperty);
}

/**
 * Return all of the current properties
 * @returns A vector of the current list of properties
 */
const std::vector<Kernel::Property *> &LogManager::getProperties() const { return m_manager->getProperties(); }

//-----------------------------------------------------------------------------------------------
/** Return the total memory used by the run object, in bytes.
 */
size_t LogManager::getMemorySize() const {
  size_t total{m_timeroi->getMemorySize()};

  for (const auto &p : m_manager->getProperties()) {
    if (p) {
      // cppcheck-suppress useStlAlgorithm
      total += p->getMemorySize() + sizeof(Property *); // cppcheck-suppress useStlAlgorithm
    }
  }

  return total;
}

/**
 * Returns a property as a time series property. It will throw if it is not
 * valid or the property does not exist
 * @param name The name of a time-series property
 * @return A pointer to the time-series property
 * @throw invalid_argument if the named property is not a time-series property
 */
template <typename T> Kernel::TimeSeriesProperty<T> *LogManager::getTimeSeriesProperty(const std::string &name) const {
  Kernel::Property *prop = getProperty(name);
  if (auto *tsp = dynamic_cast<Kernel::TimeSeriesProperty<T> *>(prop)) {
    return tsp;
  } else {
    throw std::invalid_argument("LogManager::getTimeSeriesProperty - '" + name + "' is not a TimeSeriesProperty");
  }
}

/**
 * Returns the time dependent standard deviation
 * @param name :: The name of the property
 * @return A single double value
 */
double LogManager::getTimeAveragedStd(const std::string &name) const {
  return getStatistics(name).time_standard_deviation;
}

/**
 * Returns the time averaged value
 * @param name :: The name of the property
 * @return A single double value
 */
double LogManager::getTimeAveragedValue(const std::string &name) const { return getStatistics(name).time_mean; }

/**
 * Returns various statistics computations for a given property. The time filter, if not-empty, is applied when
 * computing.
 * The returned statistics will all be NAN when statistics cannot be computed, such as for a string property.
 * @param name :: The name of the property.
 * @return A TimeSeriesPropertyStatistics object containing values for Minimum, Maximum, Mean, Median,
 * standard deviation, time weighted average, and time weighted standard deviation.
 */
Kernel::TimeSeriesPropertyStatistics LogManager::getStatistics(const std::string &name) const {
  const Kernel::Property *prop = getProperty(name);

  // statistics from a TimeSeriesProperty object
  if (auto *timeSeriesProp = dynamic_cast<const Kernel::ITimeSeriesProperty *>(prop))
    return timeSeriesProp->getStatistics(m_timeroi.get());

  // statistics from a PropertyWithValue object
  double value;
  if (Mantid::API::convertSingleValueToDouble(prop, value))
    return TimeSeriesPropertyStatistics(value);

  // return values set to NAN, signaling no statistics can be obtained
  TimeSeriesPropertyStatistics invalid;
  invalid.setAllToNan();
  return invalid;
}

/**
 * Get the value of a property as the requested type. Throws if the type is not
 * correct
 * @param name :: The name of the property
 * @return The value of as the requested type
 */
template <typename HeldType> HeldType LogManager::getPropertyValueAsType(const std::string &name) const {
  Kernel::Property *prop = getProperty(name);
  if (auto *valueProp = dynamic_cast<Kernel::PropertyWithValue<HeldType> *>(prop)) {
    return (*valueProp)();
  } else {
    throw std::invalid_argument("Run::getPropertyValueAsType - '" + name + "' is not of the requested type");
  }
}

/**
 * Returns a property as a single double value from its name @see
 * getPropertyAsSingleValue
 * @param name :: The name of the property
 * @param statistic :: The statistic to use to calculate the single value
 * (default=Mean) @see StatisticType
 * @return A single double value
 */
double LogManager::getPropertyAsSingleValue(const std::string &name, Kernel::Math::StatisticType statistic) const {
  double singleValue(0.0);
  const auto key = std::make_pair(name, statistic);
  if (!m_singleValueCache->getCache(key, singleValue)) {
    const Property *log = getProperty(name);
    const TimeROI &filter = this->getTimeROI();
    if (!convertPropertyToDouble(log, singleValue, statistic, &filter)) {
      if (const auto stringLog = dynamic_cast<const PropertyWithValue<std::string> *>(log)) {
        // Try to lexically cast string to a double
        try {
          singleValue = std::stod(stringLog->value());
        } catch (const std::invalid_argument &) {
          throw std::invalid_argument("Run::getPropertyAsSingleValue - Property \"" + name +
                                      "\" cannot be converted to a numeric value.");
        }
      } else {
        throw std::invalid_argument("Run::getPropertyAsSingleValue - Property \"" + name +
                                    "\" is not a single numeric value or numeric time series.");
      }
    }
    // Put it in the cache
    m_singleValueCache->setCache(key, singleValue);
  }
  return singleValue;
}

/**
 * Returns a property as a n integer, if the underlying value is an integer.
 * Throws otherwise.
 * @param name :: The name of the property
 * @return A single integer value
 * @throws std::invalid_argument if property is not an integer type
 */
int LogManager::getPropertyAsIntegerValue(const std::string &name) const {
  int singleValue(0);
  double discard(0);

  Property *prop = getProperty(name);

  if (convertSingleValue<int32_t>(prop, discard) || convertSingleValue<int64_t>(prop, discard) ||
      convertSingleValue<uint32_t>(prop, discard) || convertSingleValue<uint64_t>(prop, discard)) {
    singleValue = std::stoi(prop->value());
  } else {
    throw std::invalid_argument("Run::getPropertyAsIntegerValue - Property \"" + name +
                                "\" cannot be converted to an integer value.");
  }

  return singleValue;
}

/**
 * Get a pointer to a property by name
 * @param name :: The name of a property, throws an Exception::NotFoundError if
 * it does not exist
 * @return A pointer to the named property
 */
Kernel::Property *LogManager::getProperty(const std::string &name) const { return m_manager->getProperty(name); }

/** Clear out the contents of all logs of type TimeSeriesProperty.
 *  Single-value properties will be left unchanged.
 *
 *  The method has been fully implemented here instead of as a pass-through to
 *  PropertyManager to limit its visibility to Run clients.
 */
void LogManager::clearTimeSeriesLogs() {
  auto &props = getProperties();

  // Loop over the set of properties, identifying those that are time-series
  // properties
  // and then clearing them out.
  for (auto prop : props) {
    if (auto tsp = dynamic_cast<ITimeSeriesProperty *>(prop)) {
      tsp->clear();
    }
  }
}

/** Clears out all but the last entry of all logs of type TimeSeriesProperty
 *  Check the documentation/definition of TimeSeriesProperty::clearOutdated for
 *  the definition of 'last entry'.
 */
void LogManager::clearOutdatedTimeSeriesLogValues() {
  auto &props = getProperties();
  for (auto prop : props) {
    if (auto tsp = dynamic_cast<ITimeSeriesProperty *>(prop)) {
      tsp->clearOutdated();
    }
  }
}

const Kernel::TimeROI &LogManager::getTimeROI() const { return *(m_timeroi.get()); }

void LogManager::setTimeROI(const Kernel::TimeROI &timeroi) {
  m_timeroi->replaceROI(timeroi);
  this->clearSingleValueCache();
}

//--------------------------------------------------------------------------------------------
/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to create
 * @param keepOpen :: do not close group on exit to allow overloading and child
 * classes writing to the same group
 */
void LogManager::saveNexus(::NeXus::File *file, const std::string &group, bool keepOpen) const {
  file->makeGroup(group, "NXgroup", true);
  file->putAttr("version", 1);

  // Save all the properties as NXlog
  std::vector<Property *> props = m_manager->getProperties();
  for (auto &prop : props) {
    try {
      prop->saveProperty(file);
    } catch (std::invalid_argument &exc) {
      g_log.warning(exc.what());
    }
  }
  // save the timeROI to the nexus file
  if (!(m_timeroi->useAll()))
    m_timeroi->saveNexus(file);

  if (!keepOpen)
    file->closeGroup();
}

//--------------------------------------------------------------------------------------------
/** Load the object from an open NeXus file. Not used.
 * @param file :: open NeXus file
 * @param group :: name of the group to open. Pass an empty string to NOT open a
 * group
 * @param fileInfo :: The corresponding Nexus HDF5 file descriptor
 * @param prefix :: The prefix of the provided file
 * @param keepOpen :: do not close group on exit to allow overloading and child
 * classes reading from the same group
 * load any NXlog in the current open group.
 */
void LogManager::loadNexus(::NeXus::File * /*file*/, const std::string & /*group*/,
                           const Mantid::Kernel::NexusHDF5Descriptor & /*fileInfo*/, const std::string & /*prefix*/,
                           bool /*keepOpen*/) {
  throw std::runtime_error("LogManager::loadNexus should not be used");
}

//--------------------------------------------------------------------------------------------
/** Load the object from an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to open. Pass an empty string to NOT open a
 * group
 * @param keepOpen :: do not close group on exit to allow overloading and child
 * classes reading from the same group
 * load any NXlog in the current open group.
 */
void LogManager::loadNexus(::NeXus::File *file, const std::string &group, bool keepOpen) {
  if (!group.empty()) {
    file->openGroup(group, "NXgroup");
  }
  std::map<std::string, std::string> entries;
  file->getEntries(entries);
  LogManager::loadNexus(file, entries);

  if (!(group.empty() || keepOpen)) {
    file->closeGroup();
  }
}

void LogManager::loadNexus(::NeXus::File *file, const Mantid::Kernel::NexusHDF5Descriptor &fileInfo,
                           const std::string &prefix) {

  // Only load from NXlog entries
  const auto &allEntries = fileInfo.getAllEntries();
  auto itNxLogEntries = allEntries.find("NXlog");
  const auto nxLogEntries = (itNxLogEntries != allEntries.end()) ? itNxLogEntries->second : std::set<std::string>{};

  const auto levels = std::count(prefix.begin(), prefix.end(), '/');

  auto itLower = nxLogEntries.lower_bound(prefix);

  if (itLower == nxLogEntries.end()) {
    return;
  }
  if (itLower->compare(0, prefix.size(), prefix) != 0) {
    return;
  }

  for (auto it = itLower; it != nxLogEntries.end() && it->compare(0, prefix.size(), prefix) == 0; ++it) {
    // only next level entries
    const std::string &absoluteEntryName = *it;
    if (std::count(absoluteEntryName.begin(), absoluteEntryName.end(), '/') != levels + 1) {
      continue;
    }
    const std::string nameClass = absoluteEntryName.substr(absoluteEntryName.find_last_of('/') + 1);

    auto prop = PropertyNexus::loadProperty(file, nameClass, fileInfo, prefix);
    if (prop) {
      // get TimeROI
      if (prop->name() == Kernel::TimeROI::NAME) {
        auto boolProp = dynamic_cast<TimeSeriesProperty<bool> *>(prop.get());
        if (boolProp) {
          m_timeroi->replaceROI(boolProp);
        } else {
          throw std::runtime_error("Kernel_TimeROI is not a TimeSeriesPropertyBool");
        }
      } else {
        // everything else gets added to the list of properties
        m_manager->declareOrReplaceProperty(std::move(prop));
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
/** Load the object from an open NeXus file. Avoid multiple expensive calls to
 * getEntries().
 * @param file :: open NeXus file
 * @param entries :: The entries available in the current place in the file.
 * load any NXlog in the current open group.
 */
void LogManager::loadNexus(::NeXus::File *file, const std::map<std::string, std::string> &entries) {

  for (const auto &name_class : entries) {
    // NXLog types are the main one.
    if (name_class.second == "NXlog") {
      auto prop = PropertyNexus::loadProperty(file, name_class.first);
      if (prop) {
        // get TimeROI
        if (prop->name() == Kernel::TimeROI::NAME) {
          auto boolProp = dynamic_cast<TimeSeriesProperty<bool> *>(prop.get());
          if (boolProp) {
            m_timeroi->replaceROI(boolProp);
          } else {
            throw std::runtime_error("Kernel_TimeROI is not a TimeSeriesPropertyBool");
          }
        } else {
          // everything else gets added to the list of properties
          m_manager->declareOrReplaceProperty(std::move(prop));
        }
      }
    }
  }
}

/**
 * Clear the logs.
 */
void LogManager::clearLogs() { m_manager->clear(); }

void LogManager::clearSingleValueCache() { m_singleValueCache->clear(); }

/// Gets the correct log name for the matching invalid values log for a given
/// log name
std::string LogManager::getInvalidValuesFilterLogName(const std::string &logName) {
  return PropertyManager::getInvalidValuesFilterLogName(logName);
}

/// returns true if the log has a matching invalid values log filter
bool LogManager::hasInvalidValuesFilter(const std::string &logName) const {
  return hasProperty(getInvalidValuesFilterLogName(logName));
}

/// returns the invalid values log if the log has a matching invalid values log filter
Kernel::TimeSeriesProperty<bool> *LogManager::getInvalidValuesFilter(const std::string &logName) const {
  try {
    auto log = getLogData(getInvalidValuesFilterLogName(logName));
    if (auto tsp = dynamic_cast<TimeSeriesProperty<bool> *>(log)) {
      return tsp;
    }
  } catch (Exception::NotFoundError &) {
    // do nothing, just drop through tto the return line below
  }
  return nullptr;
}

bool LogManager::operator==(const LogManager &other) const {
  return (*m_manager == *(other.m_manager)) && (*m_timeroi == *(other.m_timeroi));
}

bool LogManager::operator!=(const LogManager &other) const {
  return (*m_timeroi != *(other.m_timeroi)) || (*m_manager != *(other.m_manager));
}

//-----------------------------------------------------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------------------------------------------------

/** @cond */
/// Macro to instantiate concrete template members
#define INSTANTIATE(TYPE)                                                                                              \
  template MANTID_API_DLL Kernel::TimeSeriesProperty<TYPE> *LogManager::getTimeSeriesProperty(const std::string &)     \
      const;                                                                                                           \
  template MANTID_API_DLL TYPE LogManager::getPropertyValueAsType(const std::string &) const;

INSTANTIATE(double)
INSTANTIATE(int32_t)
INSTANTIATE(int64_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(std::string)
INSTANTIATE(bool)

template MANTID_API_DLL uint16_t LogManager::getPropertyValueAsType(const std::string &) const;
template MANTID_API_DLL std::vector<double> LogManager::getPropertyValueAsType(const std::string &) const;
template MANTID_API_DLL std::vector<size_t> LogManager::getPropertyValueAsType(const std::string &) const;
template MANTID_API_DLL std::vector<int> LogManager::getPropertyValueAsType(const std::string &) const;
template MANTID_API_DLL std::vector<long> LogManager::getPropertyValueAsType(const std::string &) const;
/** @endcond */

} // namespace Mantid::API
