#ifndef MANTID_KERNEL_DYNAMICFACTORY_H_
#define MANTID_KERNEL_DYNAMICFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Instantiator.h"
#include "MantidKernel/Exception.h"

// Boost
#include "boost/shared_ptr.hpp"

// Poco
#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>
#include <Poco/AutoPtr.h>

// std
#include <map>
#include <iostream>
#include <vector>

namespace Mantid
{
namespace Kernel
{
class Logger;	
    
/** @class DynamicFactory DynamicFactory.h Kernel/DynamicFactory.h

    The dynamic factory is a base dynamic factory for serving up objects in response to requests from other classes.
    
    @author Nick Draper, Tessella Support Services plc
    @date 10/10/2007
    
    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <class Base>
class DynamicFactory
{
public:
  /**
   * Base class for dynamic factory notifications
   */
  class DynamicFactoryNotification : public Poco::Notification
  {
  };

  /**
   * A notification that the factory has been updated
   */
  class UpdateNotification : public DynamicFactoryNotification
  {
  };

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
  /// The class must have been registered with registerClass.
  /// If the class name is unknown, a NotFoundException is thrown.
  /// @param className the name of the class you wish to create
  /// @return a shared pointer ot the base class
  virtual boost::shared_ptr<Base> create(const std::string& className) const
  {   
    typename FactoryMap::const_iterator it = _map.find(className);
    if (it != _map.end())
      return it->second->createInstance();
    else
      throw Exception::NotFoundError("DynamicFactory:"+ className + " is not registered.\n", className);
  }

  /// Creates a new instance of the class with the given name, which
  /// is not wrapped in a boost shared_ptr. This should be used with
  /// extreme care.
  /// The class must have been registered with registerClass.
  /// If the class name is unknown, a NotFoundException is thrown.
  /// @param className the name of the class you wish to create
  /// @return a pointer to the base class
  virtual Base* createUnwrapped(const std::string& className) const
  {   
    typename FactoryMap::const_iterator it = _map.find(className);
    if (it != _map.end())
      return it->second->createUnwrappedInstance();
    else
      throw Exception::NotFoundError("DynamicFactory:"+ className + " is not registered.\n", className);
  }
  
  /// Registers the instantiator for the given class with the DynamicFactory.
  /// The DynamicFactory takes ownership of the instantiator and deletes
  /// it when it's no longer used.
  /// If the class has already been registered, an ExistsException is thrown
  /// and the instantiator is deleted.
  /// @param className the name of the class you wish to subscribe
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
  /// @param className the name of the class you wish to subscribe
  /// @param pAbstractFactory A pointer to an abstractFactory for this class
  void subscribe(const std::string& className, AbstractFactory* pAbstractFactory)
  {
    typename FactoryMap::iterator it = _map.find(className);
    if (!className.empty() && it == _map.end())
    {
      _map[className] = pAbstractFactory;
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
  /// @param className the name of the class you wish to unsubscribe
  void unsubscribe(const std::string& className)
  {
    typename FactoryMap::iterator it = _map.find(className);
    if (!className.empty() && it != _map.end())
    {
      delete it->second;
      _map.erase(it);
    }
    else 
    {
      throw Exception::NotFoundError("DynamicFactory:"+ className + " is not registered.\n",className);
    }
  }
  
  /// Returns true if the given class is currently registered.
  /// @param className the name of the class you wish to check
  /// @returns true is the class is subscribed
  bool exists(const std::string& className) const
  {
    return _map.find(className) != _map.end();
  }
  
  ///Returns the keys in the map
  /// \returns A string vector of keys 
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
  DynamicFactory() : notificationCenter()
  {
  }  

private:
  /// Private copy constructor - NO COPY ALLOWED
  DynamicFactory(const DynamicFactory&);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  DynamicFactory& operator = (const DynamicFactory&);

  /// A typedef for the map of registered classes
  typedef std::map<std::string, AbstractFactory*> FactoryMap;
  /// The map holding the registered class names and their instantiators
  FactoryMap _map;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_DYNAMICFACTORY_H_*/
