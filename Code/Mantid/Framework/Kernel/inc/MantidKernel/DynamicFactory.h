#ifndef MANTID_KERNEL_DYNAMICFACTORY_H_
#define MANTID_KERNEL_DYNAMICFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Instantiator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/RegistrationHelper.h"

// Boost
#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif

// Poco
#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>
#include <Poco/AutoPtr.h>

// std
#include <cstring>
#include <functional>
#include <map>
#include <vector>

namespace Mantid
{
namespace Kernel
{
  
  //----------------------------------------------------------------------------
  // Forward declarations
  //----------------------------------------------------------------------------
  class Logger; 
  
  typedef std::less<std::string> CaseSensitiveStringComparator;
  struct CaseInsensitiveStringComparator
  {
    bool operator() (const std::string & s1, const std::string & s2) const
    {
#ifdef _MSC_VER
	    return stricmp(s1.c_str(), s2.c_str()) < 0;
#else
	    return strcasecmp(s1.c_str(), s2.c_str()) < 0;
#endif
    }
  };

    
/** @class DynamicFactory DynamicFactory.h Kernel/DynamicFactory.h

    The dynamic factory is a base dynamic factory for serving up objects in response 
    to requests from other classes.
    
    @author Nick Draper, Tessella Support Services plc
    @date 10/10/2007
    
    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
template <class Base, class Comparator = CaseInsensitiveStringComparator >
class DynamicFactory
{

public:
  /// Defines the whether notifications are dispatched
  enum NotificationStatus {Enabled, Disabled};
  /// Defines replacement behaviour
  enum SubscribeAction {ErrorIfExists, OverwriteCurrent};

public:
  /**
   * Base class for dynamic factory notifications
   */
  class DynamicFactoryNotification : public Poco::Notification
  {
  };

  /**
   * A notification that the factory has been updated. This is
   * blind to the details.
   */
  class UpdateNotification : public DynamicFactoryNotification
  {
  };

  /**
   * Enable notifications
   */
  void enableNotifications()
  {
    m_notifyStatus = Enabled;
  }

  /**
   * Disable notifications
   */
  void disableNotifications()
  {
    m_notifyStatus = Disabled;
  }

public:
  /// A typedef for the instantiator
  typedef AbstractInstantiator<Base> AbstractFactory;
  
  /// Destroys the DynamicFactory and deletes the instantiators for 
  /// all registered classes.
  virtual ~DynamicFactory()
  {
    for (typename FactoryMap::iterator it = _map.begin(); it != _map.end(); ++it)
    {
      delete it->second;
    }
  }
  
  /// Creates a new instance of the class with the given name.
  /// The class must have been registered with subscribe() (typically done via a macro).
  /// If the class name is unknown, a NotFoundException is thrown.
  /// @param className :: the name of the class you wish to create
  /// @return a shared pointer ot the base class
  virtual boost::shared_ptr<Base> create(const std::string& className) const
  {   
    typename FactoryMap::const_iterator it = _map.find(className);
    if (it != _map.end())
      return it->second->createInstance();
    else
      throw Exception::NotFoundError("DynamicFactory: "+ className + " is not registered.\n", className);
  }

  /// Creates a new instance of the class with the given name, which
  /// is not wrapped in a boost shared_ptr. This should be used with
  /// extreme care (or, better, not used)! The caller owns the returned instance.
  /// The class must have been registered with subscribe() (typically done via a macro).
  /// If the class name is unknown, a NotFoundException is thrown.
  /// @param className :: the name of the class you wish to create
  /// @return a pointer to the base class
  virtual Base* createUnwrapped(const std::string& className) const
  {   
    typename FactoryMap::const_iterator it = _map.find(className);
    if (it != _map.end())
      return it->second->createUnwrappedInstance();
    else
      throw Exception::NotFoundError("DynamicFactory: "+ className + " is not registered.\n", className);
  }
  
  /// Registers the instantiator for the given class with the DynamicFactory.
  /// The DynamicFactory takes ownership of the instantiator and deletes
  /// it when it's no longer used.
  /// If the class has already been registered, an ExistsException is thrown
  /// and the instantiator is deleted.
  /// @param className :: the name of the class you wish to subscribe
  template <class C> 
  void subscribe(const std::string& className)
  {
    subscribe(className, new Instantiator<C, Base>);
  }


  /// Registers the instantiator for the given class with the DynamicFactory.
  /// The DynamicFactory takes ownership of the instantiator and deletes
  /// it when it's no longer used.
  /// If the class has already been registered, an ExistsException is thrown
  /// and the instantiator is deleted.
  /// @param className :: the name of the class you wish to subscribe
  /// @param pAbstractFactory :: A pointer to an abstractFactory for this class
  /// @param replace :: If ReplaceExisting then the given AbstractFactory replaces any existing
  ///                   factory with the same className, else throws std::runtime_error (default=ThrowOnExisting)
  void subscribe(const std::string& className, AbstractFactory* pAbstractFactory,
                 SubscribeAction replace=ErrorIfExists)
  {
    if(className.empty())
    {
      delete pAbstractFactory;
      throw std::invalid_argument("Cannot register empty class name");
    }

    typename FactoryMap::iterator it = _map.find(className);
    if (it == _map.end() || replace == OverwriteCurrent)
    {
      if (it != _map.end() && it->second)
        delete it->second;
      _map[className] = pAbstractFactory;
      sendUpdateNotificationIfEnabled();
    }
    else
    {
      delete pAbstractFactory;
      throw std::runtime_error(className + " is already registered.\n");
    }
  }
  
  /// Unregisters the given class and deletes the instantiator
  /// for the class.
  /// Throws a NotFoundException if the class has not been registered.
  /// @param className :: the name of the class you wish to unsubscribe
  void unsubscribe(const std::string& className)
  {
    typename FactoryMap::iterator it = _map.find(className);
    if (!className.empty() && it != _map.end())
    {
      delete it->second;
      _map.erase(it);
      sendUpdateNotificationIfEnabled();
    }
    else 
    {
      throw Exception::NotFoundError("DynamicFactory:"+ className + " is not registered.\n",className);
    }
  }
  
  /// Returns true if the given class is currently registered.
  /// @param className :: the name of the class you wish to check
  /// @returns true is the class is subscribed
  bool exists(const std::string& className) const
  {
    return _map.find(className) != _map.end();
  }
  
  ///Returns the keys in the map
  /// @return A string vector of keys 
  virtual const std::vector<std::string> getKeys() const
  {
    std::vector<std::string> names;
    names.reserve(_map.size());

    typename FactoryMap::const_iterator iter = _map.begin();
    for (; iter != _map.end(); ++iter)
    {
      names.push_back(iter->first);
    }

    return names;
  }

  /// Sends notifications to observers. Observers can subscribe to notificationCenter
  /// using Poco::NotificationCenter::addObserver(...)
  /// @return nothing
  Poco::NotificationCenter notificationCenter;
    
protected:
  /// Protected constructor for base class
  DynamicFactory() : notificationCenter(), _map(),
    m_notifyStatus(Disabled)
  {
  }  

private:
  /// Private copy constructor - NO COPY ALLOWED
  DynamicFactory(const DynamicFactory&);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  DynamicFactory& operator = (const DynamicFactory&);

  /// Send an update notification if they are enabled
  void sendUpdateNotificationIfEnabled()
  {
    if(m_notifyStatus == Enabled) sendUpdateNotification();
  }
  /// Send an update notification
  void sendUpdateNotification()
  {
    notificationCenter.postNotification(new UpdateNotification);
  }

  /// A typedef for the map of registered classes
  typedef std::map<std::string, AbstractFactory*,Comparator> FactoryMap;
  /// The map holding the registered class names and their instantiators
  FactoryMap _map;
  /// Flag marking whether we should dispatch notifications
  NotificationStatus m_notifyStatus;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_DYNAMICFACTORY_H_*/
