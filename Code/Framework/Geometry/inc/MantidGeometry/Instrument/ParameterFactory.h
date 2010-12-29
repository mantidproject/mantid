#ifndef MANTID_GEOMETRY_PARAMETERFACTORY_H_
#define MANTID_GEOMETRY_PARAMETERFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Instantiator.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Logger.h"
#include <vector>
#include <map>

#ifdef _WIN32
#if (IN_MANTID_GEOMETRY)
#define GEOMETRY_DLL_EXPORT DLLExport
#else
#define GEOMETRY_DLL_EXPORT DLLImport
#endif
#else
#define GEOMETRY_DLL_EXPORT
#endif

namespace Mantid
{
	
namespace Kernel
{
  class Logger;
}
	
namespace Geometry
{

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Parameter;

/** The ParameterFactory class creates parameters for the instrument ParameterMap. 
    
    @author Roman Tolchenov, Tessella plc
    @date 19/05/2009
    
    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class GEOMETRY_DLL_EXPORT ParameterFactory
{
public:
  template<class C>
  static void subscribe(const std::string& className);
  
  static boost::shared_ptr<Parameter> create(const std::string& className, const std::string& name);
  
private:
  /// Private default constructor
  ParameterFactory();
  /// Private copy constructor
  ParameterFactory(const ParameterFactory&);
  /// Private assignment operator
  ParameterFactory& operator=(const ParameterFactory&);
  
  /// A typedef for the instantiator
  typedef Kernel::AbstractInstantiator<Parameter> AbstractFactory;
  /// An inner class to specialise map such that it does a deep delete when s_map is destroyed
  class DLLExport FactoryMap : public std::map<std::string, AbstractFactory*>
  {
  public:
    /// Destructor. Deletes the AbstractInstantiator objects stored in the map.
    virtual ~FactoryMap()
    {
      for (iterator it = this->begin(); it!=this->end(); ++it) delete it->second;
    }
  };
  /// The map holding the registered class names and their instantiators
  static FactoryMap s_map;
};

/**  Templated method for parameter subscription
 *   @param className The parameter type name
 *   @tparam C The parameter type
 */
template<class C>
void ParameterFactory::subscribe(const std::string& className)
{
  typename FactoryMap::iterator it = s_map.find(className);
  if (!className.empty() && it == s_map.end())
  {
    s_map[className] = new Kernel::Instantiator<C, Parameter>;
  }
  else
  {
    throw std::runtime_error("Parameter type" + className + " is already registered.\n");
  }
}

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_PARAMETERFACTORY_H_*/
