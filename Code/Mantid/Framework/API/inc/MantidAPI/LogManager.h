#ifndef MANTID_API_LOGMANAGER_H_
#define MANTID_API_LOGMANAGER_H_

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Cache.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/Statistics.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidKernel/Matrix.h"
#include <nexus/NeXusFile.hpp>
#include <vector>

namespace Mantid {
namespace Kernel {
template <typename TYPE> class TimeSeriesProperty;
}

namespace API {

/**
   This class contains the information about the log entries


   @author Martyn Gigg, Tessella plc
   @date 02/10/201

   Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

   This file is part of Mantid.

   Mantid is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Mantid is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   File change history is stored at: <https://github.com/mantidproject/mantid>.
   Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL LogManager {
public:
  /// Default constructor
  LogManager();
  /// Destructor. Doesn't need to be virtual as long as nothing inherits from
  /// this class.
  virtual ~LogManager();
  /// Copy constructor
  LogManager(const LogManager &copy);
  /// Assignment operator
  const LogManager &operator=(const LogManager &rhs);

  //-------------------------------------------------------------
  /// Set the run start and end
  void setStartAndEndTime(const Kernel::DateAndTime &start,
                          const Kernel::DateAndTime &end);
  /// Return the run start time
  const Kernel::DateAndTime startTime() const;
  /// Return the run end time
  const Kernel::DateAndTime endTime() const;
  //-------------------------------------------------------------

  /// Filter the logs by time
  virtual void filterByTime(const Kernel::DateAndTime start,
                            const Kernel::DateAndTime stop);
  /// Split the logs based on the given intervals
  virtual void splitByTime(Kernel::TimeSplitterType &splitter,
                           std::vector<LogManager *> outputs) const;
  /// Filter the run by the given boolean log
  void filterByLog(const Kernel::TimeSeriesProperty<bool> &filter);

  /// Return an approximate memory size for the object in bytes
  virtual size_t getMemorySize() const;

  /// Add data to the object in the form of a property
  void addProperty(Kernel::Property *prop, bool overwrite = false);
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
  void removeProperty(const std::string &name, bool delproperty = true);
  /**
   * Return all of the current properties
   * @returns A vector of the current list of properties
   */
  inline const std::vector<Kernel::Property *> &getProperties() const {
    return m_manager.getProperties();
  }
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
  /// Returns the named property as a pointer
  Kernel::Property *getProperty(const std::string &name) const;

  /**
   * Add a log entry
   * @param p :: A pointer to the property containing the log entry
   */
  void addLogData(Kernel::Property *p) { addProperty(p); }
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
  /// A pointer to a property manager
  Kernel::PropertyManager m_manager;
  /// Name of the log entry containing the proton charge when retrieved using
  /// getProtonCharge
  static const char *PROTON_CHARGE_LOG_NAME;

private:
  /// Cache type for single value logs
  typedef Kernel::Cache<std::pair<std::string, Kernel::Math::StatisticType>,
                        double> SingleValueCache;
  /// Cache for the retrieved single values
  mutable SingleValueCache m_singleValueCache;
};
/// shared pointer to the logManager base class
typedef boost::shared_ptr<LogManager> LogManager_sptr;
/// shared pointer to the logManager base class (const version)
typedef boost::shared_ptr<const LogManager> LogManager_const_sptr;

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
  addProperty(new Kernel::PropertyWithValue<TYPE>(name, value), overwrite);
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
  Kernel::Property *newProp = new Kernel::PropertyWithValue<TYPE>(name, value);
  newProp->setUnits(units);
  addProperty(newProp, overwrite);
}
}
}

#endif // MANTID_API_LOGMANAGER_H_
