#ifndef MANTID_KERNEL_DATASERVICE_H_
#define MANTID_KERNEL_DATASERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#endif
#include <Poco/NotificationCenter.h>
#include <Poco/Notification.h>
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"

#include <mutex>

#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include "Strings.h"
#endif

namespace Mantid {
namespace Kernel {

/// Flag for whether to sort items before returning
enum class DataServiceSort { Sorted, Unsorted };
/**
 * Flag for whether to include hidden items when returning,
 * Auto queries the class to determine this behavior
 */
enum class DataServiceHidden { Auto, Include, Exclude };

// Case-insensitive comparison functor for std::map
struct CaseInsensitiveCmp {
  bool operator()(const std::string &lhs, const std::string &rhs) const {
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
  }
};

/** DataService stores instances of a given type.
    This is a templated class, designed to be implemented as a
    singleton. For simplicity and naming conventions, specialized classes must
   be constructed. The specialized
    classes must simply:
    1) call the BaseClass constructor with the Name of the service
    2) Support the SingletonHolder templated class.
    This is the primary data service that  the users will interact with either
   through writing scripts or directly
    through the API. It is implemented as a singleton class.

    Copyright &copy; 2008-2013 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
template <typename T> class DLLExport DataService {
private:
  /// Typedef for the map holding the names of and pointers to the data objects
  using svcmap =
      std::map<std::string, boost::shared_ptr<T>, CaseInsensitiveCmp>;
  /// Iterator for the data store map
  using svc_it = typename svcmap::iterator;
  /// Const iterator for the data store map
  using svc_constit = typename svcmap::const_iterator;

public:
  /// Class for named object notifications
  class NamedObjectNotification : public Poco::Notification {
  public:
    NamedObjectNotification(const std::string &name)
        : Poco::Notification(), m_name(name) {}

    /// Returns the name of the object
    std::string objectName() const { return m_name; }

  private:
    std::string m_name; ///< object's name
  };

  /// Base class for DataService notifications that also stores a pointer to the
  /// object
  class DataServiceNotification : public NamedObjectNotification {
  public:
    /// Constructor
    DataServiceNotification(const std::string &name,
                            const boost::shared_ptr<T> obj)
        : NamedObjectNotification(name), m_object(obj) {}
    /// Returns the const pointer to the object concerned or 0 if it is a
    /// general notification
    const boost::shared_ptr<T> object() const { return m_object; }

  private:
    boost::shared_ptr<T> m_object; ///< shared pointer to the object
  };

  /// AddNotification is sent after an new object is added to the data service.
  /// name() and object() return name and pointer to the new object.
  class AddNotification : public DataServiceNotification {
  public:
    AddNotification(const std::string &name, const boost::shared_ptr<T> obj)
        : DataServiceNotification(name, obj) {} ///< Constructor
  };

  /// BeforeReplaceNotification is sent before an object is replaced in the
  /// addOrReplace() function.
  class BeforeReplaceNotification : public DataServiceNotification {
  public:
    /** Constructor.

        @param name :: The name of the replaced object
        @param obj ::  The pointer to the old object
        @param newObj :: The pointer to the new object

        Both old and new objects are guaranteed to exist when an observer
       receives the notification.
    */
    BeforeReplaceNotification(const std::string &name,
                              const boost::shared_ptr<T> obj,
                              const boost::shared_ptr<T> newObj)
        : DataServiceNotification(name, obj), m_newObject(newObj),
          m_oldObject(obj) {}
    const boost::shared_ptr<T> newObject() const {
      return m_newObject;
    } ///< Returns the pointer to the new object.
    const boost::shared_ptr<T> oldObject() const { return m_oldObject; }

  private:
    boost::shared_ptr<T> m_newObject; ///< shared pointer to the object
    boost::shared_ptr<T> m_oldObject;
  };

  /// AfterReplaceNotification is sent after an object is replaced in the
  /// addOrReplace() function.
  class AfterReplaceNotification : public DataServiceNotification {
  public:
    /** Constructor.
      * @param name :: The name of the replaced object
      *  @param newObj :: The pointer to the new object
      *  Only new objects are guaranteed to exist when an observer receives the
     * notification.
    */
    AfterReplaceNotification(const std::string &name,
                             const boost::shared_ptr<T> newObj)
        : DataServiceNotification(name, newObj) {}
  };

  /// PreDeleteNotification is sent before an object is deleted from the data
  /// service.
  /// name() and object() return name and pointer to the object being deleted.
  /// The object still exists when the notification is received by an observer.
  class PreDeleteNotification : public DataServiceNotification {
  public:
    /// Constructor
    PreDeleteNotification(const std::string &name,
                          const boost::shared_ptr<T> obj)
        : DataServiceNotification(name, obj) {}
  };

  /// PostDeleteNotification is sent after an object is deleted from the data
  /// service.
  /// name() returns the name of the object being deleted.
  /// The object no longer exists when the notification is received by an
  /// observer.
  class PostDeleteNotification : public NamedObjectNotification {
  public:
    /// Constructor
    PostDeleteNotification(const std::string &name)
        : NamedObjectNotification(name) {}
  };

  /// Clear notification is sent when the service is cleared
  class ClearNotification : public NamedObjectNotification {
  public:
    /// Constructor
    ClearNotification() : NamedObjectNotification("") {}
  };
  /// Rename notification is sent when the rename method is called
  class RenameNotification : public NamedObjectNotification {
  public:
    /// Constructor
    RenameNotification(const std::string &name, const std::string &newName)
        : NamedObjectNotification(name), m_newName(newName) {}

    /// New name for the object
    const std::string &newObjectName() const { return m_newName; }

  private:
    std::string m_newName; ///< New object name
  };

  //--------------------------------------------------------------------------
  /** Add an object to the service
   * @param name :: name of the object
   * @param Tobject :: shared pointer to object to add
   * @throw std::runtime_error if name is empty
   * @throw std::runtime_error if name exists in the map
   * @throw std::runtime_error if a null pointer is passed for the object
   */
  virtual void add(const std::string &name,
                   const boost::shared_ptr<T> &Tobject) {
    checkForEmptyName(name);
    checkForNullPointer(Tobject);

    bool success = false;
    {
      // Make DataService access thread-safe
      std::lock_guard<std::recursive_mutex> lock(m_mutex);
      // At the moment, you can't overwrite an object (i.e. pass in a name
      // that's already in the map with a pointer to a different object).
      // Also, there's nothing to stop the same object from being added
      // more than once with different names.
      success = datamap.insert(std::make_pair(name, Tobject)).second;
    }
    if (!success) {
      std::string error =
          " add : Unable to insert Data Object : '" + name + "'";
      g_log.error(error);
      throw std::runtime_error(error);
    } else {
      g_log.debug() << "Add Data Object " << name << " successful\n";
      notificationCenter.postNotification(new AddNotification(name, Tobject));
    }
  }

  //--------------------------------------------------------------------------
  /** Add or replace an object to the service.
   * Does NOT throw if the name was already used.
   *
   * @param name :: name of the object
   * @param Tobject :: shared pointer to object to add
   * @throw std::runtime_error if name is empty
   */
  virtual void addOrReplace(const std::string &name,
                            const boost::shared_ptr<T> &Tobject) {
    checkForNullPointer(Tobject);

    // Make DataService access thread-safe
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    // find if the Tobject already exists
    auto it = datamap.find(name);
    if (it != datamap.end()) {
      lock.unlock();
      g_log.debug("Data Object '" + name + "' replaced in data service.\n");

      notificationCenter.postNotification(
          new BeforeReplaceNotification(name, it->second, Tobject));

      lock.lock();
      it->second = Tobject;
      lock.unlock();

      notificationCenter.postNotification(
          new AfterReplaceNotification(name, Tobject));
    } else {
      // Avoid double-locking
      lock.unlock();
      DataService::add(name, Tobject);
    }
  }

  //--------------------------------------------------------------------------
  /** Remove an object from the service.
   * @param name :: name of the object */
  void remove(const std::string &name) {
    // Make DataService access thread-safe
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    auto it = datamap.find(name);
    if (it == datamap.end()) {
      lock.unlock();
      g_log.debug(" remove '" + name + "' cannot be found");
      return;
    }
    // The map is shared across threads so the item is erased from the map
    // before unlocking the mutex and is held in a local stack variable.
    // This protects it from being modified by another thread.
    auto data = std::move(it->second);
    datamap.erase(it);

    // Do NOT use "it" iterator after this point. Other threads may modify the
    // map
    lock.unlock();
    notificationCenter.postNotification(new PreDeleteNotification(name, data));
    data.reset(); // DataService now has no references to the object
    g_log.information("Data Object '" + name + "' deleted from data service.");
    notificationCenter.postNotification(new PostDeleteNotification(name));
  }

  //--------------------------------------------------------------------------
  /** Rename an object within the service.
   * @param oldName :: The old name of the object
   * @param newName :: The new name of the object
   */
  void rename(const std::string &oldName, const std::string &newName) {
    checkForEmptyName(newName);

    if (oldName == newName) {
      g_log.warning("Rename: The existing name matches the new name");
      return;
    }

    // Make DataService access thread-safe
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    auto existingNameIter = datamap.find(oldName);
    if (existingNameIter == datamap.end()) {
      lock.unlock();
      g_log.warning(" rename '" + oldName + "' cannot be found");
      return;
    }

    auto existingNameObject = std::move(existingNameIter->second);
    auto targetNameIter = datamap.find(newName);

    // If we are overriding send a notification for observers
    if (targetNameIter != datamap.end()) {
      auto targetNameObject = targetNameIter->second;
      // As we are renaming the existing name turns into the new name
      notificationCenter.postNotification(new BeforeReplaceNotification(
          newName, targetNameObject, existingNameObject));
    }

    datamap.erase(existingNameIter);

    if (targetNameIter != datamap.end()) {
      targetNameIter->second = std::move(existingNameObject);
      notificationCenter.postNotification(
          new AfterReplaceNotification(newName, targetNameIter->second));
    } else {
      if (!(datamap.emplace(newName, std::move(existingNameObject)).second)) {
        // should never happen
        lock.unlock();
        std::string error =
            " add : Unable to insert Data Object : '" + newName + "'";
        g_log.error(error);
        throw std::runtime_error(error);
      }
    }
    lock.unlock();
    g_log.information("Data Object '" + oldName + "' renamed to '" + newName +
                      "'");
    notificationCenter.postNotification(
        new RenameNotification(oldName, newName));
  }

  //--------------------------------------------------------------------------
  /// Empty the service
  void clear() {
    {
      // Make DataService access thread-safe
      std::lock_guard<std::recursive_mutex> lock(m_mutex);
      datamap.clear();
    }
    notificationCenter.postNotification(new ClearNotification());
    g_log.debug() << typeid(this).name() << " cleared.\n";
  }

  /// Prepare for shutdown
  virtual void shutdown() { clear(); }

  //--------------------------------------------------------------------------
  /** Get a shared pointer to a stored data object
   * @param name :: name of the object */
  boost::shared_ptr<T> retrieve(const std::string &name) const {
    // Make DataService access thread-safe
    std::lock_guard<std::recursive_mutex> _lock(m_mutex);

    auto it = datamap.find(name);
    if (it != datamap.end()) {
      return it->second;
    } else {
      throw Kernel::Exception::NotFoundError(
          "Unable to find Data Object type with name '" + name +
              "': data service ",
          name);
    }
  }

  /// Checks all elements within the specified vector exist in the ADS
  bool doAllWsExist(const std::vector<std::string> &listOfNames) {
    for (const auto &wsName : listOfNames) {
      if (!doesExist(wsName))
        return false;
    }
    return true;
  }

  /// Check to see if a data object exists in the store
  bool doesExist(const std::string &name) const {
    // Make DataService access thread-safe
    std::lock_guard<std::recursive_mutex> _lock(m_mutex);
    auto it = datamap.find(name);
    return it != datamap.end();
  }

  /// Return the number of objects stored by the data service
  size_t size() const {
    std::lock_guard<std::recursive_mutex> _lock(m_mutex);

    if (showingHiddenObjects()) {
      return datamap.size();
    } else {
      size_t count = 0;
      for (auto &it : datamap) {
        if (!isHiddenDataServiceObject(it.first))
          ++count;
      }
      return count;
    }
  }

  /**
   * Returns a vector of strings containing all object names in the ADS
   * @param sortState Whether to sort the output before returning. Defaults
   * to unsorted
   * @param hiddenState Whether to include hidden objects, Defaults to
   * Auto which checks the current configuration to determine behavior.
   * @return A vector of strings containing object names in the ADS
   */
  std::vector<std::string> getObjectNames(
      DataServiceSort sortState = DataServiceSort::Unsorted,
      DataServiceHidden hiddenState = DataServiceHidden::Auto) const {

    std::vector<std::string> foundNames;

    // First test if auto flag is set whether to include hidden
    if (hiddenState == DataServiceHidden::Auto) {
      if (showingHiddenObjects()) {
        hiddenState = DataServiceHidden::Include;
      } else {
        hiddenState = DataServiceHidden::Exclude;
      }
    }

    // Use the scoping of an if to handle our lock for duration
    if (hiddenState == DataServiceHidden::Include) {
      // Getting hidden items
      std::lock_guard<std::recursive_mutex> _lock(m_mutex);
      foundNames.reserve(datamap.size());
      for (const auto &item : datamap) {
        foundNames.push_back(item.first);
      }
      // Lock released at end of scope here
    } else {
      std::lock_guard<std::recursive_mutex> _lock(m_mutex);
      foundNames.reserve(datamap.size());
      for (const auto &item : datamap) {
        if (!isHiddenDataServiceObject(item.first)) {
          // This item is not hidden add it
          foundNames.push_back(item.first);
        }
      }
      // Lock released at end of scope here
    }

    // Now sort if told to
    if (sortState == DataServiceSort::Sorted) {
      std::sort(foundNames.begin(), foundNames.end());
    }

    return foundNames;
  }

  /// Get a vector of the pointers to the data objects stored by the service
  std::vector<boost::shared_ptr<T>>
  getObjects(DataServiceHidden includeHidden = DataServiceHidden::Auto) const {
    std::lock_guard<std::recursive_mutex> _lock(m_mutex);

    const bool alwaysIncludeHidden =
        includeHidden == DataServiceHidden::Include;
    const bool usingAuto =
        includeHidden == DataServiceHidden::Auto && showingHiddenObjects();

    const bool showingHidden = alwaysIncludeHidden || usingAuto;

    std::vector<boost::shared_ptr<T>> objects;
    objects.reserve(datamap.size());
    for (const auto &it : datamap) {
      if (showingHidden || !isHiddenDataServiceObject(it.first)) {
        objects.push_back(it.second);
      }
    }
    return objects;
  }

  inline static std::string prefixToHide() { return "__"; }

  inline static bool isHiddenDataServiceObject(const std::string &name) {
    return boost::starts_with(name, prefixToHide());
  }

  static bool showingHiddenObjects() {
    int showingHiddenFlag;
    const int success = ConfigService::Instance().getValue(
        "MantidOptions.InvisibleWorkspaces", showingHiddenFlag);
    if (success == 1 && showingHiddenFlag == 1) {
      return true;
    } else {
      return false;
    }
  }

  /// Sends notifications to observers. Observers can subscribe to
  /// notificationCenter
  /// using Poco::NotificationCenter::addObserver(...)
  ///@return nothing
  Poco::NotificationCenter notificationCenter;
  /// Deleted copy constructor
  DataService(const DataService &) = delete;
  /// Deleted copy assignment operator
  DataService &operator=(const DataService &) = delete;

protected:
  /// Protected constructor (singleton)
  DataService(const std::string &name) : svcName(name), g_log(svcName) {}
  virtual ~DataService() = default;

private:
  void checkForEmptyName(const std::string &name) {
    if (name.empty()) {
      const std::string error = "Add Data Object with empty name";
      g_log.debug() << error << '\n';
      throw std::runtime_error(error);
    }
  }

  void checkForNullPointer(const boost::shared_ptr<T> &Tobject) {
    if (!Tobject) {
      const std::string error = "Attempt to add empty shared pointer";
      g_log.debug() << error << '\n';
      throw std::runtime_error(error);
    }
  }

  /// DataService name. This is set only at construction. DataService name
  /// should be provided when construction of derived classes
  const std::string svcName;
  /// Map of objects in the data service
  svcmap datamap;
  /// Recursive mutex to avoid simultaneous access or notifications
  mutable std::recursive_mutex m_mutex;
  /// Logger for this DataService
  Logger g_log;
}; // End Class Data service

} // Namespace Kernel
} // Namespace Mantid

#endif /*MANTID_KERNEL_DATASERVICE_H_*/
