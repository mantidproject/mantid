#ifndef MANTID_KERNEL_REMOTEJOBMANAGERFACTORY_H_
#define MANTID_KERNEL_REMOTEJOBMANAGERFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/XMLInstantiator.h"

// Note: RemoteJobManagerFactory doesn't actually need RegistrationHelper
// We're including it here because it's referenced in the DECLARE_RJM macro
// below, and thus anyone that uses the macro will need RegistrationHelper
#include "MantidKernel/RegistrationHelper.h"

#include <boost/shared_ptr.hpp>

#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>


//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class RemoteJobManager;


namespace Mantid
{
namespace Kernel
{

/** The RemoteJobManagerFactory class is in charge of the creation of concrete
    instances of RemoteJobManager objects. Note that it does NOT inherit from
    DynamicFactory.  This is largely because RemoteJobManager objects use a
    chunk of XML to define various settings when they're created and
    DynamicFactory assumes an object's constructor takes no parameters.

    The class is implemented as a singleton class.
    
    @author Ross Miller, Oak Ridge National Laboratory
    @date 4/12/2012
    
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/


/* Used to register concrete classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 */
#define DECLARE_RJM(classname, typestring) \
  namespace { \
    Mantid::Kernel::RegistrationHelper register_alg_##classname( \
       ((Mantid::Kernel::RemoteJobManagerFactory::Instance().subscribe<classname>( typestring)) \
       , 0)); \
  }


class RemoteJobManagerFactoryImpl
{
public:

  /// Creates an instance of a RemoteJobManager from an XML description
  boost::shared_ptr<RemoteJobManager> create(const Poco::XML::Element* elem ) const
  {
    std::string type = elem->getAttribute("type");
    InstantiatorMap::const_iterator it = m_instantiatorMap.find( type);
    if (it == m_instantiatorMap.end())
    {
      throw std::runtime_error( std::string("No instantiator for RemoteJobManger of type ") + type);
    }

    return (*it).second->createInstance( elem);
  }

  /// RJM concrete classes call this to register with the factory
  template <class C>
  void subscribe( const std::string & managerType)
  {
    if (! exists( managerType))
    {
      boost::shared_ptr<XMLInstantiator<C, RemoteJobManager> > newI( new XMLInstantiator<C, RemoteJobManager>);
      m_instantiatorMap.insert( std::make_pair(managerType, newI));
    }
  }

  /// Unsubscribe the specified RemoteJobManager
  void unsubscribe(const std::string & managerType)
  {
    InstantiatorMap::iterator it = m_instantiatorMap.find( managerType);
    if (it != m_instantiatorMap.end())
    {
      m_instantiatorMap.erase( it);
    }
  }

  /// Does an manager of the given type exist
  bool exists(const std::string & managerType)
  {
    return (m_instantiatorMap.find(managerType) != m_instantiatorMap.end());
  }

 private:

  friend struct CreateUsingNew<RemoteJobManagerFactoryImpl>;

  /// Private Constructor for singleton class
  RemoteJobManagerFactoryImpl() { }
  /// Private copy constructor - NO COPY ALLOWED
  RemoteJobManagerFactoryImpl(const RemoteJobManagerFactoryImpl&);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  RemoteJobManagerFactoryImpl& operator = (const RemoteJobManagerFactoryImpl&);

  ///Private Destructor
  ~RemoteJobManagerFactoryImpl() { }

  // Holds the mapping of factory types to instantiators
  typedef std::map <std::string, boost::shared_ptr<XMLAbstractInstantiator<RemoteJobManager> > > InstantiatorMap;
  InstantiatorMap m_instantiatorMap;
};
  

///Forward declaration of a specialisation of SingletonHolder for RemoteJobManagerFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class MANTID_KERNEL_DLL Mantid::Kernel::SingletonHolder<RemoteJobManagerFactoryImpl>;
#endif /* _WIN32 */

typedef MANTID_KERNEL_DLL Mantid::Kernel::SingletonHolder<RemoteJobManagerFactoryImpl> RemoteJobManagerFactory;

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_REMOTEJOBMANAGERFACTORY_H_*/
