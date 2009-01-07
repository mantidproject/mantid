#ifndef PARAMETERMAP_H_
#define PARAMETERMAP_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/Parameter.h"
#include "boost/shared_ptr.hpp"

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

class IComponent;

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
    void addTypeString(const IComponent* compName,const std::string& name, const std::string& value)
    {
        ParameterType<T> *param = new ParameterType<T>(name);
        param->fromString(value);
        m_map.insert(std::make_pair(compName,boost::shared_ptr<Parameter>(param)));
    }

    /// Templated method for adding a parameter providing its value of a particular type
    template<class T>
    void addType(const IComponent* compName,const std::string& name, const T& value)
    {
        ParameterType<T> *param = new ParameterType<T>(name);
        param->setValue(value);
        m_map.insert(std::make_pair(compName,boost::shared_ptr<Parameter>(param)));
    }

    /// Method is provided to add a parameter of any custom type which must be created with 'new' operator.
    /// ParameterMap takes the owneship of the parameter. e.g. AMap.add(new CustomParameter,"compName","name","value");
    void add(Parameter* param,const IComponent* compName,const std::string& name, const std::string& value)
    {
        param->fromString(value);
        m_map.insert(std::make_pair(compName,boost::shared_ptr<Parameter>(param)));
    }

    /// The same as above except that the caller is resposible for setting the parameter's value.
    /// e.g. CustomParameter* param = new CustomParameter;
    ///      param->setItsValue(value);
    ///      AMap.add(param,"compName","name");
    void add(Parameter* param,const IComponent* compName,const std::string& name)
    {
        m_map.insert(std::make_pair(compName,boost::shared_ptr<Parameter>(param)));
    }

    /// Concrete parameter adding methods.
    void addDouble(const IComponent* compName,const std::string& name, const std::string& value){addTypeString<double>(compName,name,value);}
    void addDouble(const IComponent* compName,const std::string& name, double value){addType(compName,name,value);}

    void addInt(const IComponent* compName,const std::string& name, const std::string& value){addTypeString<int>(compName,name,value);}
    void addInt(const IComponent* compName,const std::string& name, int value){addType(compName,name,value);}

    void addBool(const IComponent* compName,const std::string& name, const std::string& value){addTypeString<bool>(compName,name,value);}
    void addBool(const IComponent* compName,const std::string& name, bool value){addType(compName,name,value);}

    void addString(const IComponent* compName,const std::string& name, const std::string& value){addTypeString<std::string>(compName,name,value);}

    void addV3D(const IComponent* compName,const std::string& name, const std::string& value){addTypeString<V3D>(compName,name,value);}
    void addV3D(const IComponent* compName,const std::string& name, const V3D& value){addType(compName,name,value);}

    void addQuat(const IComponent* compName,const std::string& name, const Quat& value){addType(compName,name,value);}

    /// Return the value of a parameter as a string.
    std::string getString(const IComponent* compName,const std::string& name);

    Parameter* get(const IComponent* compName,const std::string& name)const;

    /// Templated get method
    template<class T>
    T getType(const IComponent* compName,const std::string& name)const
    {
        Parameter *param = get(compName,name);
        if (!param) return T();
        ParameterType<T> *tparam = dynamic_cast<ParameterType<T> *>(param);
        if (!tparam)
            throw std::runtime_error("Parameter "+name+" is not of type " + typeid(T).name());
        return tparam->value();
    }

    double getDouble(const IComponent* compName,const std::string& name)const{return getType<double>(compName,name);}
    int getInt(const IComponent* compName,const std::string& name)const{return getType<int>(compName,name);}
    bool getBool(const IComponent* compName,const std::string& name)const{return getType<bool>(compName,name);}
    V3D getV3D(const IComponent* compName,const std::string& name)const{return getType<V3D>(compName,name);}
    Quat getQuat(const IComponent* compName,const std::string& name)const{return getType<Quat>(compName,name);}

    /// Returns a vector with all parameter names for componenet compName
    std::vector<std::string> nameList(const IComponent* compName)const;

private:
  ///Assignment operator
  ParameterMap& operator=(const ParameterMap& rhs);
  /// insternal parameter map instance
  pmap m_map;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // Namespace Geometry

} // Namespace Mantid

#endif /*PARAMETERMAP_H_*/
