// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_LOGMANAGER_H_
#define MANTID_API_LOGMANAGER_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Statistics.h"

#include <memory>
#include <vector>

namespace NeXus {
class File;
}

namespace Mantid {
namespace Types {
namespace Core {
class DateAndTime;
}
} // namespace Types
namespace Kernel {
template <class KEYTYPE, class VALUETYPE> class Cache;
template <typename TYPE> class TimeSeriesProperty;
class SplittingInterval;
using TimeSplitterType = std::vector<SplittingInterval>;
class PropertyManager;
} // namespace Kernel

namespace API {

/**
   This class contains the information about the log entries


   @author Martyn Gigg, Tessella plc
   @date 02/10/201
*/
class MANTID_API_DLL LogManager {
public:
  LogManager();
  LogManager(const LogManager &other);
  /// Destructor. Doesn't need to be virtual as long as nothing inherits from
  /// this class.
  virtual ~LogManager();
  LogManager &operator=(const LogManager &other);

  //-------------------------------------------------------------
  /// Set the run start and end
  void setStartAndEndTime(const Types::Core::DateAndTime &start,
                          const Types::Core::DateAndTime &end);
  /// Return the run start time
  const Types::Core::DateAndTime startTime() const;
  /// Return the run end time
  const Types::Core::DateAndTime endTime() const;
  //-------------------------------------------------------------

  /// Filter the logs by time
  virtual void filterByTime(const Types::Core::DateAndTime start,
                            const Types::Core::DateAndTime stop);
  /// Split the logs based on the given intervals
  virtual void splitByTime(Kernel::TimeSplitterType &splitter,
                           std::vector<LogManager *> outputs) const;
  /// Filter the run by the given boolean log
  void filterByLog(const Kernel::TimeSeriesProperty<bool> &filter);

  /// Return an approximate memory size for the object in bytes
  virtual size_t getMemorySize() const;

  /// Add data to the object in the form of a property
  /// @deprecated new code should use smart pointers
  void addProperty(Kernel::Property *prop, bool overwrite = false) {
    addProperty(std::unique_ptr<Kernel::Property>(prop), overwrite);
  };
  /// Add data to the object in the form of a property
  void addProperty(std::unique_ptr<Kernel::Property> prop,
                   bool overwrite = false);
  /// Add a property of given type
  template <class TYPE>
  void addProperty(const std::string &name, const TYPE &value,
                   bool overwrite = false);

  template <class TYPE>
  void addProperty(const std::string &name, const TYPE &value,
                   const std::string &units, bool overwrite = false);

  /// Does the property exist on the object
  bool hasProperty(const std::string &name) const;
  /// Remove a named property
  void removeProperty(const std::string &name, bool delProperty = true);
  const std::vector<Kernel::Property *> &getProperties() const;

  /// Returns a property as a time series property. It will throw if it is not
  /// valid
  template <typename T>
  Kernel::TimeSeriesProperty<T> *
  getTimeSeriesProperty(const std::string &name) const;
  /// Get the value of a property as the given TYPE. Throws if the type is not
  /// correct
  template <typename HeldType>
  HeldType getPropertyValueAsType(const std::string &name) const;
  /// Returns a property as a single double value from its name
  double getPropertyAsSingleValue(
      const std::string &name,
      Kernel::Math::StatisticType statistic = Kernel::Math::Mean) const;
  /// Returns a property as an integer value
  int getPropertyAsIntegerValue(const std::string &name) const;
  /// Returns the named property as a pointer
  Kernel::Property *getProperty(const std::string &name) const;

  /**
   * Add a log entry
   * @param p :: A pointer to the property containing the log entry
   * @deprecated new code should use smart pointers
   */
  void addLogData(Kernel::Property *p) {
    addLogData(std::unique_ptr<Kernel::Property>(p));
  }

  /**
   * Add a log entry
   * @param p :: A pointer to the property containing the log entry
   * @param overwrite :: Overwrite existing if requested
   */
  void addLogData(std::unique_ptr<Kernel::Property> p, bool overwrite = false) {
    addProperty(std::move(p), overwrite);
  }

  /**
   * Access a single log entry
   * @param name :: The name of the log entry to retrieve
   * @returns A pointer to a property containing the log entry
   */
  Kernel::Property *getLogData(const std::string &name) const {
    return getProperty(name);
  }
  /**
   * Access all log entries
   * @returns A list of all of the log entries
   */
  const std::vector<Kernel::Property *> &getLogData() const {
    return getProperties();
  }
  /**
   * Remove a named log entry
   * @param name :: The name of the entry to remove
   * @param delproperty :: If true, delete the log entry
   */
  void removeLogData(const std::string &name, const bool delproperty = true) {
    return removeProperty(name, delproperty);
  }
  /**
   * @param name :: The name of the property
   * @param statistic :: Defines how to calculate the single value from series
   * (default=Mean)
   * @return A log as a single value using the given statistic type
   */
  double getLogAsSingleValue(
      const std::string &name,
      Kernel::Math::StatisticType statistic = Kernel::Math::Mean) const {
    return getPropertyAsSingleValue(name, statistic);
  }

  /// Empty the values out of all TimeSeriesProperty logs
  void clearTimeSeriesLogs();
  /// Empty all but the last value out of all TimeSeriesProperty logs
  void clearOutdatedTimeSeriesLogValues();

  /// Save the run to a NeXus file with a given group name
  virtual void saveNexus(::NeXus::File *file, const std::string &group,
                         bool keepOpen = false) const;
  /// Load the run from a NeXus file with a given group name
  virtual void loadNexus(::NeXus::File *file, const std::string &group,
                         bool keepOpen = false);
  /// Clear the logs
  void clearLogs();

protected:
  /// Load the run from a NeXus file with a given group name
  void loadNexus(::NeXus::File *file,
                 const std::map<std::string, std::string> &entries);
  /// A pointer to a property manager
  std::unique_ptr<Kernel::PropertyManager> m_manager;
  /// Name of the log entry containing the proton charge when retrieved using
  /// getProtonCharge
  static const char *PROTON_CHARGE_LOG_NAME;

private:
  /// Cache for the retrieved single values
  std::unique_ptr<Kernel::Cache<
      std::pair<std::string, Kernel::Math::StatisticType>, double>>
      m_singleValueCache;
};
/// shared pointer to the logManager base class
using LogManager_sptr = boost::shared_ptr<LogManager>;
/// shared pointer to the logManager base class (const version)
using LogManager_const_sptr = boost::shared_ptr<const LogManager>;

/**
 * Add a property of a specified type (Simply creates a Kernel::Property of that
 * type
 * @param name :: The name of the type
 * @param value :: The value of the property
 * @param overwrite :: If true, a current value is overwritten. (Default: False)
 */
template <class TYPE>
void LogManager::addProperty(const std::string &name, const TYPE &value,
                             bool overwrite) {
  addProperty(std::make_unique<Kernel::PropertyWithValue<TYPE>>(name, value),
              overwrite);
}

/**
 * Add a property of a specified type (Simply creates a Kernel::Property of that
 * type)
 *  and set its units.
 * @param name :: The name of the type
 * @param value :: The value of the property
 * @param units :: a string giving the units of the property.
 * @param overwrite :: If true, a current value is overwritten. (Default: False)
 */
template <class TYPE>
void LogManager::addProperty(const std::string &name, const TYPE &value,
                             const std::string &units, bool overwrite) {
  auto newProp = std::make_unique<Kernel::PropertyWithValue<TYPE>>(name, value);
  newProp->setUnits(units);
  addProperty(std::move(newProp), overwrite);
}
} // namespace API
} // namespace Mantid

#endif // MANTID_API_LOGMANAGER_H_
