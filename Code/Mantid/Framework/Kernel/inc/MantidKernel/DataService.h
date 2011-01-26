#ifndef MANTID_KERNEL_DATASERVICE_H_
#define MANTID_KERNEL_DATASERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <boost/shared_ptr.hpp>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <algorithm>

#include <Poco/NotificationCenter.h>
#include <Poco/Notification.h>

#include "MantidKernel/System.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/MemoryManager.h"

namespace Mantid
{
namespace Kernel
{
/** DataService is the base class for storing DataObjects. It stores instances of DataObjects
    (Workspace, Instrument, MappingTables,...). This is a templated class, implemented as a
    singleton. For simplicity and naming conventions, specialized classes must be constructed. The specialized
    classes (see example MantidAPI/InstrumentDataService.h) must simply :
    1) call the BaseClass constructor with the Name of the service
    2) Support the SingletonHolder templated class.
    This is the primary data service that  the users will interact with either through writing scripts or directly
    through the API. It is implemented as a singleton class.

    @author Laurent C Chapon, ISIS, Rutherford Appleton Laboratory
    @date 30/05/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
template <typename T>
class DLLExport DataService
{
public:

    /// Base class for DataService notifications
    class DataServiceNotification: public Poco::Notification
    {
    public:
        /// Constructor
        DataServiceNotification(const std::string& name, const boost::shared_ptr<T> obj):Poco::Notification(),m_name(name),m_object(obj){}
        /// Returns the const pointer to the object concerned or 0 if it is a general notification
        const boost::shared_ptr<T> object()const{return m_object;}
        /// Returns the name of the object
        std::string object_name()const{return m_name;}
    private:
        std::string m_name;///< object's name
        boost::shared_ptr<T> m_object;///< shared pointer to the object
    };

    /// AddNotification is sent after an new object is added to the data service.
    /// name() and object() return name and pointer to the new object.
    class AddNotification: public DataServiceNotification
    {
    public:
        AddNotification(const std::string& name,const boost::shared_ptr<T> obj):DataServiceNotification(name,obj){}///< Constructor
    };


    /// BeforeReplaceNotification is sent before an object is replaced in the addOrReplace() function.
    class BeforeReplaceNotification: public DataServiceNotification
    {
    public:
        /** Constructor.

            @param name The name of the replaced object
            @param obj  The pointer to the old object
            @param new_obj The pointer to the new object

            Both old and new objects are guaranteed to exist when an observer receives the notification.
        */
        BeforeReplaceNotification(const std::string& name, const boost::shared_ptr<T> obj,const boost::shared_ptr<T> new_obj):DataServiceNotification(name,obj),m_new_object(new_obj){}
        const boost::shared_ptr<T> new_object()const{return m_new_object;}///< Returns the pointer to the new object.
    private:
        boost::shared_ptr<T> m_new_object;///< shared pointer to the object
    };

    /// AfterReplaceNotification is sent after an object is replaced in the addOrReplace() function.
    class AfterReplaceNotification: public DataServiceNotification
    {
    public:
        /** Constructor.

            @param name The name of the replaced object
            @param new_obj The pointer to the new object

            Only new objects are guaranteed to exist when an observer receives the notification.
        */
        AfterReplaceNotification(const std::string& name, const boost::shared_ptr<T> new_obj):DataServiceNotification(name,new_obj) {}
    };

    /// DeleteNotification is sent after an object is deleted from the data service.
    /// name() and object() return name and pointer to the object being deleted.
    /// The object still exists when the notification is received by an observer.
    class DeleteNotification: public DataServiceNotification
    {
    public:
      /// Constructor
      DeleteNotification(const std::string& name,const boost::shared_ptr<T> obj):DataServiceNotification(name,obj){}
    };

    /// Clear notification is sent when the service is cleared
    class ClearNotification: public DataServiceNotification
    {
    public:
      ///Constructor
      ClearNotification() :  DataServiceNotification("", boost::shared_ptr<T>()) {}
    };
    /// Rename notification is sent from Renameworkspaces algorithm after a workspace is renamed
    class RenameNotification: public DataServiceNotification
    {
    public:
        /// Constructor
      RenameNotification(const std::string& name,const std::string& newwsname):DataServiceNotification(name,boost::shared_ptr<T>()),m_outwsname(newwsname){}

      ///name of the new workspace
      const std::string& new_objectname()const{return m_outwsname;}
    private:
      std::string m_outwsname; ///< output workspace name
    };

  /// Add an object to the service
  virtual void add( const std::string& name, const boost::shared_ptr<T>& Tobject)
  {
    // Don't permit an empty name for the workspace
    if (name.empty())
    {
      std::string error=" add Data Object with empty name";
      g_log.error(error);
      throw std::runtime_error(error);
    }

    // At the moment, you can't overwrite a workspace (i.e. pass in a name
    // that's already in the map with a pointer to a different workspace).
    // Also, there's nothing to stop the same workspace from being added
    // more than once with different names.
    if ( ! datamap.insert(typename svcmap::value_type(name, Tobject)).second)
    {
      std::string error=" add : Unable to insert Data Object : '"+name+"'";
      g_log.error(error);
      throw std::runtime_error(error);
    }
    else
    {
    	std::string information=" add Data Object '"+name+"' successful";
    	g_log.information(information);
        notificationCenter.postNotification(new AddNotification(name,Tobject));
    }
    return;
  }

  /// Add or replace an object
  virtual void addOrReplace( const std::string& name, const boost::shared_ptr<T>& Tobject)
  {
    //find if the Tobject already exists
    svc_it it = datamap.find(name);
    if (it!=datamap.end())
    {
      g_log.information("Data Object '"+ name +"' replaced in data service.");
      notificationCenter.postNotification(new BeforeReplaceNotification(name,it->second,Tobject));
      datamap[name] = Tobject;
      notificationCenter.postNotification(new AfterReplaceNotification(name,Tobject));
    }
    else
    {
      add(name,Tobject);
    }
    return;
  }

  /// Remove an object
  void remove( const std::string& name)
  {
    svc_it it = datamap.find(name);
    if (it==datamap.end())
    {
      g_log.warning(" remove '" + name + "' cannot be found");
      return;
    }

    notificationCenter.postNotification(new DeleteNotification(name,it->second));
    g_log.information("Data Object '"+ name +"' deleted from data service.");
    datamap.erase(it);
    API::MemoryManager::Instance().releaseFreeMemory();
    return;
  }

  /// Empty the service
  void clear()
  {
    datamap.clear();
    notificationCenter.postNotification(new ClearNotification());
    g_log.debug() << typeid(this).name() << " cleared.";
  }

  /// Get a shared pointer to a stored data object
  boost::shared_ptr<T> retrieve( const std::string& name) const
  {
    svc_constit it = datamap.find(name);
    if (it!=datamap.end())
    {
      return it->second;
    }
    else
    {
      throw Kernel::Exception::NotFoundError("Data Object",name);
    }
  }

  /// Check to see if a data object exists in the store
  bool doesExist(const std::string& name) const
  {
	  svc_constit it=datamap.find(name);
	  if (it!=datamap.end())
		  	return true;
	  return false;
  }

  /// Return the number of objects stored by the data service
  int size() const
  {
    return datamap.size();
  }

  /// Get a vector of the names of the data objects stored by the service
  std::set<std::string> getObjectNames() const
  {
    std::set<std::string> names;
    svc_constit it;
    for( it = datamap.begin(); it != datamap.end(); ++it)
    {
      names.insert(it->first);
    }
    return names;
  }

  /// Sends notifications to observers. Observers can subscribe to notificationCenter
  /// using Poco::NotificationCenter::addObserver(...)
  ///@return nothing
  Poco::NotificationCenter notificationCenter;

protected:
  /// Protected constructor (singleton)
  DataService(const std::string& name):svc_name(name),g_log(Kernel::Logger::get(svc_name)){}
  virtual ~DataService(){}

private:
	/// Private, unimplemented copy constructor
	DataService(const DataService&);
	/// Private, unimplemented copy assignment operator
	DataService& operator=(const DataService&);

  /// DataService name. This is set only at construction. DataService name should be provided when construction of derived classes
  std::string svc_name;
  
  /// functor for the less binary function that compares strings alphabetically without taking account of case
  struct ci_less : std::binary_function<std::string, std::string, bool>
  {
    
    /// functor for case-independent less binary function for characters
    struct nocase_compare : public std::binary_function<unsigned char,unsigned char,bool> 
    {
      /// case-independent less binary function for characters
      bool operator() (const unsigned char &c1, const unsigned char &c2) const 
      {
        return tolower (c1) < tolower (c2);
      }
    };
    /** case-independent (ci) less binary function for strings
    * Compares strings alphabetically
    * @param s1 first string for comparison
    * @param s2 second string for comparison
    * @return true if s1 < s2
    */
    bool operator() (const std::string &s1, const std::string &s2) const
    {
      return lexicographical_compare 
        (s1.begin(), s1.end(), s2.begin(), s2.end(), nocase_compare());
    }
  }; // end of ci_less

  /// Typedef for the map holding the names of and pointers to the data objects
  typedef std::map<std::string,  boost::shared_ptr<T>, ci_less > svcmap;
	/// Iterator for the data store map
	typedef typename svcmap::iterator svc_it;
  /// Const iterator for the data store map
	typedef typename svcmap::const_iterator svc_constit;
	/// Map of objects in the data service
	svcmap datamap;

  /// Static reference to the logger for this DataService
  Logger& g_log;

}; // End Class Data service

} // Namespace Kernel
} // Namespace Mantid

#endif /*MANTID_KERNEL_DATASERVICE_H_*/
