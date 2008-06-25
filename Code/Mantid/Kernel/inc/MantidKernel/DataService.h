#ifndef MANTID_KERNEL_DATASERVICE_
#define MANTID_KERNEL_DATASERVICE_

#include <boost/shared_ptr.hpp>
#include <string>
#include <map>
#include <vector>

#include "MantidKernel/System.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace Kernel
{
/**  DataService is the base class for storing DataObjects. It stores instances of DataObjects
    (Workspace, Instrument, MappingTables,...). This is a templated class, implemented as a
    singleton. For simplicity and naming conventions, specialized classes must be constructed. The specialized 
    classes (see example MantidAPI/InstrumentDataService.h) must simply :
    1) call the BaseClass constructor with the Name of the service 
    2) Support the SingletonHolder templated class.
    This is the primary data service that  the users will interact with either through writing scripts or directly
    through the API. It is implemented as a singleton class.
    
    @author Laurent C Chapon, ISIS, Rutherford Appleton Laboratory
    @date 30/05/2008
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
template <typename T> class DLLExport DataService
{
public:
  /// Add an object to the service
  void add( const std::string& name, const boost::shared_ptr<T>& Tobject)
  {
	  std::cerr << "Adding " << name << std::endl;
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
    }
    return;
  }
  /// Add or replace an object
  void addOrReplace( const std::string& name, const boost::shared_ptr<T>& Tobject)
  {
	  std::cerr << "Adding or replacing" << name << std::endl;
	  
    //find if the Tobject already exists
	svc_it it = datamap.find(name);
    if (it!=datamap.end())
     datamap[name] = Tobject;
    else
      add(name,Tobject);
    return;
  }
  /// Remove an object 
  void remove( const std::string& name)
  {

	  svc_it it = datamap.find(name);
	   if (it==datamap.end())
	   {
	     g_log.warning(" remove '" + name + "' cannot be found");
	     std::cerr << "Could not find " << name << std::endl;
	     return;
	   }   
	   
	   std::cerr << "RefCount = " << it->second.use_count() << std::endl;
	   
	   datamap.erase(it);
	   return;
  }
  /// Empty the service
  void clear()
  {
	  datamap.clear();
  }
  boost::shared_ptr<T> retrieve( const std::string& name)
  {
	  svc_constit it = datamap.find(name);
	   if (it!=datamap.end())
	     return it->second;
	   else
	   {
		   g_log.error(" Data Object '"+name+"' not found");
		   throw Kernel::Exception::NotFoundError("Data Object",name);
	   }
  }
  bool doesExist(const std::string& name) const
  {
	  svc_constit it=datamap.find(name);
	  if (it!=datamap.end())
		  	return true;
	  return false;
  }
  int size() const
  {
	  return datamap.size();
  }
  std::vector<std::string> getObjectNames() const
  {
	  int n=size();
	  std::vector<std::string> names(n);
	  if (n==0) 
		  return names;
	  svc_constit it;   
	  int i=0;
	  for( it = datamap.begin(); it != datamap.end(); ++it) 
	  			names[i++]=it->first;
	  return names;
  }
protected:
	/// DataService name. This is set only at construction. 
	/// DataService name should be provided when construction of derived classes 
	std::string svc_name;
	/// Referenc to the logger for this DataService
	Logger& g_log;
	/// Protected constructor (singleton)
	DataService(const std::string& name):svc_name(name),g_log(Kernel::Logger::get(svc_name)){}
	// Not implemented
	DataService(const DataService&);
	// Not Implemented
	DataService& operator=(const DataService&);
	~DataService(){}
	
	// 
	typedef std::map<std::string,boost::shared_ptr<T> > svcmap;
	typedef typename svcmap::iterator svc_it;  
	typedef typename svcmap::const_iterator svc_constit; 
	/// Map of objects in the data service
	svcmap datamap; 
	
}; // End Class Data service

} // Namespace Kernel
} // Namespace Mantid
#endif /*DATASERVICE_*/
