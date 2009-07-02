#ifndef MANTID_GEOMETRY_PARAMETERFACTORY_H_
#define MANTID_GEOMETRY_PARAMETERFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
//#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/Instantiator.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Logger.h"

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

/** The AlgorithmFactory class creates parameters for the instrument ParameterMap. 
    It inherits most of its implementation from the Dynamic Factory base class.
    It is implemented as a singleton class.
    
    @author Roman Tolchenov, Tessella plc
    @date 19/05/2009
    
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/
//class DLLExport ParameterFactoryImpl : public Kernel::DynamicFactory<Parameter>
//  {
//  public:
//      /// Creates a new instance of parameter of type className
//      Parameter* create(const std::string& className, const std::string& name) const;
//      template<class C>
//      void subscribe(const std::string& className);
//  private:
//	friend struct Mantid::Kernel::CreateUsingNew<ParameterFactoryImpl>;
//
//	/// Private Constructor for singleton class
//    ParameterFactoryImpl(): Kernel::DynamicFactory<Parameter>(), g_log(Kernel::Logger::get("ParameterFactory")){}
//	/// Private copy constructor - NO COPY ALLOWED
//	ParameterFactoryImpl(const ParameterFactoryImpl&);
//	/// Private assignment operator - NO ASSIGNMENT ALLOWED
//	ParameterFactoryImpl& operator = (const ParameterFactoryImpl&);
//	///Private Destructor
//    virtual ~ParameterFactoryImpl(){}
//	///static reference to the logger class
//	Kernel::Logger& g_log;
//  
//  };
//  
//	///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
//#ifdef _WIN32
//// this breaks new namespace declaraion rules; need to find a better fix
//	template class DLLExport Mantid::Kernel::SingletonHolder<ParameterFactoryImpl>;
//#endif /* _WIN32 */
//	typedef DLLExport Mantid::Kernel::SingletonHolder<ParameterFactoryImpl> ParameterFactory;
//	

    class DLLExport ParameterFactory
    {
    public:
        /// A typedef for the instantiator
        typedef Kernel::AbstractInstantiator<Parameter> AbstractFactory;
        /// A typedef for the map of registered classes
        typedef std::map<std::string, AbstractFactory*> FactoryMap;
        template<class C>
        static void subscribe(const std::string& className);
        static Parameter* create(const std::string& className, const std::string& name);
    private:
        /// Private default constructor
        ParameterFactory();
        /// Private copy constructor
        ParameterFactory(const ParameterFactory&);
        /// Private assignment operator
        ParameterFactory& operator=(const ParameterFactory&);
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
