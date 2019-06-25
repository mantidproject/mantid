// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_DYNAMICFACTORY_H_
#define MANTID_KERNEL_DYNAMICFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/CaseInsensitiveMap.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Instantiator.h"
#include "MantidKernel/RegistrationHelper.h"

// Boost
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

// Poco
#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>

// std
#include <functional>
#include <iterator>
#include <vector>

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------
class Logger;

using CaseSensitiveStringComparator = std::less<std::string>;

/** @class DynamicFactory DynamicFactory.h Kernel/DynamicFactory.h

    The dynamic factory is a base dynamic factory for serving up objects in
   response
    to requests from other classes.

    @author Nick Draper, Tessella Support Services plc
    @date 10/10/2007
*/
template <class Base, class Comparator = CaseInsensitiveStringComparator>
class DynamicFactory {

public:
  /// Defines the whether notifications are dispatched
  enum NotificationStatus { Enabled, Disabled };
  /// Defines replacement behaviour
  enum SubscribeAction { ErrorIfExists, OverwriteCurrent };
  DynamicFactory(const DynamicFactory &) = delete;
  DynamicFactory &operator=(const DynamicFactory &) = delete;

  /**
   * Base class for dynamic factory notifications
   */
  class DynamicFactoryNotification : public Poco::Notification {};
  /**
   * A notification that the factory has been updated. This is
   * blind to the details.
   */
  class UpdateNotification : public DynamicFactoryNotification {};

  /**
   * Enable notifications
   */
  void enableNotifications() { m_notifyStatus = Enabled; }

  /**
   * Disable notifications
   */
  void disableNotifications() { m_notifyStatus = Disabled; }

  /// A typedef for the instantiator
  using AbstractFactory = AbstractInstantiator<Base>;
  /// Destroys the DynamicFactory and deletes the instantiators for
  /// all registered classes.
  virtual ~DynamicFactory() {}

  /// Creates a new instance of the class with the given name.
  /// The class must have been registered with subscribe() (typically done via a
  /// macro).
  /// If the class name is unknown, a NotFoundException is thrown.
  /// @param className :: the name of the class you wish to create
  /// @return a shared pointer ot the base class
  virtual boost::shared_ptr<Base> create(const std::string &className) const {
    auto it = _map.find(className);
    if (it != _map.end())
      return it->second->createInstance();
    else
      throw Exception::NotFoundError(
          "DynamicFactory: " + className + " is not registered.\n", className);
  }

  /// Creates a new instance of the class with the given name, which
  /// is not wrapped in a boost shared_ptr. This should be used with
  /// extreme care (or, better, not used)! The caller owns the returned
  /// instance.
  /// The class must have been registered with subscribe() (typically done via a
  /// macro).
  /// If the class name is unknown, a NotFoundException is thrown.
  /// @param className :: the name of the class you wish to create
  /// @return a pointer to the base class
  virtual Base *createUnwrapped(const std::string &className) const {
    auto it = _map.find(className);
    if (it != _map.end())
      return it->second->createUnwrappedInstance();
    else
      throw Exception::NotFoundError(
          "DynamicFactory: " + className + " is not registered.\n", className);
  }

  /// Registers the instantiator for the given class with the DynamicFactory.
  /// The DynamicFactory takes ownership of the instantiator and deletes
  /// it when it's no longer used.
  /// If the class has already been registered, an ExistsException is thrown
  /// and the instantiator is deleted.
  /// @param className :: the name of the class you wish to subscribe
  template <class C> void subscribe(const std::string &className) {
    subscribe(className, std::make_unique<Instantiator<C, Base>>());
  }

  /// Registers the instantiator for the given class with the DynamicFactory.
  /// The DynamicFactory takes ownership of the instantiator and deletes
  /// it when it's no longer used.
  /// If the class has already been registered, an ExistsException is thrown
  /// and the instantiator is deleted.
  /// @param className :: the name of the class you wish to subscribe
  /// @param pAbstractFactory :: A pointer to an abstractFactory for this class
  /// @param replace :: If ReplaceExisting then the given AbstractFactory
  /// replaces any existing
  ///                   factory with the same className, else throws
  ///                   std::runtime_error (default=ThrowOnExisting)
  void subscribe(const std::string &className,
                 std::unique_ptr<AbstractFactory> pAbstractFactory,
                 SubscribeAction replace = ErrorIfExists) {
    if (className.empty()) {
      throw std::invalid_argument("Cannot register empty class name");
    }

    auto it = _map.find(className);
    if (it == _map.end() || replace == OverwriteCurrent) {
      _map[className] = std::move(pAbstractFactory);
      sendUpdateNotificationIfEnabled();
    } else {
      throw std::runtime_error(className + " is already registered.\n");
    }
  }

  /// Unregisters the given class and deletes the instantiator
  /// for the class.
  /// Throws a NotFoundException if the class has not been registered.
  /// @param className :: the name of the class you wish to unsubscribe
  void unsubscribe(const std::string &className) {
    auto it = _map.find(className);
    if (!className.empty() && it != _map.end()) {
      _map.erase(it);
      sendUpdateNotificationIfEnabled();
    } else {
      throw Exception::NotFoundError(
          "DynamicFactory:" + className + " is not registered.\n", className);
    }
  }

  /// Returns true if the given class is currently registered.
  /// @param className :: the name of the class you wish to check
  /// @returns true is the class is subscribed
  bool exists(const std::string &className) const {
    return _map.find(className) != _map.end();
  }

  /// Returns the keys in the map
  /// @return A string vector of keys
  virtual const std::vector<std::string> getKeys() const {
    std::vector<std::string> names;
    names.reserve(_map.size());
    std::transform(
        _map.cbegin(), _map.cend(), std::back_inserter(names),
        [](const std::pair<const std::string, std::unique_ptr<AbstractFactory>>
               &mapPair) { return mapPair.first; });
    return names;
  }

  /// Sends notifications to observers. Observers can subscribe to
  /// notificationCenter
  /// using Poco::NotificationCenter::addObserver(...)
  /// @return nothing
  Poco::NotificationCenter notificationCenter;

protected:
  /// Protected constructor for base class
  DynamicFactory() : notificationCenter(), _map(), m_notifyStatus(Disabled) {}

private:
  /// Send an update notification if they are enabled
  void sendUpdateNotificationIfEnabled() {
    if (m_notifyStatus == Enabled)
      sendUpdateNotification();
  }
  /// Send an update notification
  void sendUpdateNotification() {
    notificationCenter.postNotification(new UpdateNotification);
  }

  /// A typedef for the map of registered classes
  using FactoryMap =
      std::map<std::string, std::unique_ptr<AbstractFactory>, Comparator>;
  /// The map holding the registered class names and their instantiators
  FactoryMap _map;
  /// Flag marking whether we should dispatch notifications
  NotificationStatus m_notifyStatus;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_DYNAMICFACTORY_H_*/
