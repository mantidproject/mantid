#ifndef PARAMETERMAP_H_
#define PARAMETERMAP_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/Parameter.h"
#include "boost/shared_ptr.hpp"
#include "MantidGeometry/IComponent.h"

#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

#include <vector>
#include <typeinfo>

namespace Mantid
{
namespace Geometry
{

//class IComponent;

/** @class ParameterMap ParameterMap.h


    ParameterMap class. Holds the parameters of modified (parametrized) instrument
    components. ParameterMap has a number of 'add' methods for adding parameters of
    different types. 

    @author Roman Tolchenov, Tessella Support Services plc
    @date 2/12/2008

 Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
class DLLExport ParameterMap
{
public:
#ifndef HAS_UNORDERED_MAP_H
  /// Parameter map typedef
    typedef std::multimap<const IComponent*,boost::shared_ptr<Parameter> > pmap;
  /// Parameter map iterator typedef
  typedef std::multimap<const IComponent*,boost::shared_ptr<Parameter> >::const_iterator pmap_it;
#else
  /// Parameter map typedef
  typedef std::tr1::unordered_multimap<const IComponent*,boost::shared_ptr<Parameter> > pmap;
  /// Parameter map iterator typedef
  typedef std::tr1::unordered_multimap<const IComponent*,boost::shared_ptr<Parameter> >::const_iterator pmap_it;
#endif
    ///Constructor
    ParameterMap(){}
    ///virtual destructor
    virtual ~ParameterMap(){}
    /// Return the size of the map
    int size() const {return m_map.size();}
    ///Copy Contructor
    ParameterMap(const ParameterMap& copy);

    /// Templated method for adding a parameter providing its value as a string
    template<class T>
    void addTypeString(const IComponent* comp,const std::string& name, const std::string& value)
    {
        ParameterType<T> *param = new ParameterType<T>(name);
        param->fromString(value);
        m_map.insert(std::make_pair(comp,boost::shared_ptr<Parameter>(param)));
    }

    /// Templated method for adding a parameter providing its value of a particular type
    template<class T>
    void addType(const IComponent* comp,const std::string& name, const T& value)
    {
        ParameterType<T> *param = new ParameterType<T>(name);
        param->setValue(value);
        m_map.insert(std::make_pair(comp,boost::shared_ptr<Parameter>(param)));
    }

    /// Method is provided to add a parameter of any custom type which must be created with 'new' operator.
    /// ParameterMap takes the owneship of the parameter. e.g. AMap.add(new CustomParameter,"comp","name","value");
    void add(Parameter* param,const IComponent* comp,const std::string& name, const std::string& value)
    {
        param->fromString(value);
        m_map.insert(std::make_pair(comp,boost::shared_ptr<Parameter>(param)));
    }

    /// The same as above except that the caller is resposible for setting the parameter's value.
    /// e.g. CustomParameter* param = new CustomParameter;
    ///      param->setItsValue(value);
    ///      AMap.add(param,"comp","name");
    void add(Parameter* param,const IComponent* comp,const std::string& name)
    {
        m_map.insert(std::make_pair(comp,boost::shared_ptr<Parameter>(param)));
    }

    /// Concrete parameter adding methods.
    void addDouble(const IComponent* comp,const std::string& name, const std::string& value){addTypeString<double>(comp,name,value);}
    void addDouble(const IComponent* comp,const std::string& name, double value){addType(comp,name,value);}

    void addInt(const IComponent* comp,const std::string& name, const std::string& value){addTypeString<int>(comp,name,value);}
    void addInt(const IComponent* comp,const std::string& name, int value){addType(comp,name,value);}

    void addBool(const IComponent* comp,const std::string& name, const std::string& value){addTypeString<bool>(comp,name,value);}
    void addBool(const IComponent* comp,const std::string& name, bool value){addType(comp,name,value);}

    void addString(const IComponent* comp,const std::string& name, const std::string& value){addTypeString<std::string>(comp,name,value);}

    void addV3D(const IComponent* comp,const std::string& name, const std::string& value){addTypeString<V3D>(comp,name,value);}
    void addV3D(const IComponent* comp,const std::string& name, const V3D& value){addType(comp,name,value);}

    void addQuat(const IComponent* comp,const std::string& name, const Quat& value){addType(comp,name,value);}

    /// Return the value of a parameter as a string.
    std::string getString(const IComponent* comp,const std::string& name);

    boost::shared_ptr<Parameter> get(const IComponent* comp,const std::string& name)const;

    /// Get the values of a given parameter of all the components that have the name: compName
    template<class T>
    std::vector<T> getType(const std::string& compName,const std::string& name)const
    {
      std::vector<T> retval;

      pmap_it it;
      for (it = m_map.begin(); it != m_map.end(); ++it)
      {
        if ( compName.compare(((*it).first)->getName()) == 0 )  
        {
          Parameter_sptr param = get((*it).first,name);
          if (param)
            retval.push_back( param->value<T>() );
        }
      }
      return retval;
    }

    std::vector<double> getDouble(const std::string& compName,const std::string& name)const{return getType<double>(compName,name);}

    /// Returns a vector with all parameter names for componenet comp
    std::vector<std::string> nameList(const IComponent* comp)const;

private:
  ///Assignment operator
  ParameterMap& operator=(const ParameterMap& rhs);
  /// insternal parameter map instance
  pmap m_map;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

typedef boost::shared_ptr<Parameter> Parameter_sptr;

} // Namespace Geometry

} // Namespace Mantid

#endif /*PARAMETERMAP_H_*/
