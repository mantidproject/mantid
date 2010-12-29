#ifndef MANTID_GEOMETRY_PARAMETER_H_
#define MANTID_GEOMETRY_PARAMETER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"
#include "boost/shared_ptr.hpp"
#include <string>
#include <typeinfo>
#include <vector>
#include <stdexcept>

namespace Mantid
{

namespace Kernel
{
template <class C, class Base>
class Instantiator;
}

namespace Geometry
{

/** @class Parameter Parameter.h Geometry/Parameter.h

    Base class for parameters of an instrument.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 2/12/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport Parameter
{
public:
    /// Constructor
    ///Parameter( const std::string& name):m_name(name){}
    /// Virtual destructor
    virtual ~Parameter(){}

    /// Parameter type
    const std::string& type() const{return m_type;};
	/// Parameter name
    const std::string& name() const{return m_name;};

	/// Returns the value of the property as a string
    virtual std::string asString() const{return m_str_value;}

	/// Set the value of the property via a string
    virtual void fromString( const std::string& value ){m_str_value = value;}

    /// Returns the parameter value of type T if the parameter has type ParameterType<T>
    template<class T>
    const T& value();

    /** Sets the value of type T to the parameter if it has type ParameterType<T>
        Throws an exception if the types don't match.
        @param t Value to set
     */ 
    template<class T>
    void set(const T& t);

protected:
    friend class ParameterFactory;
    /// Constructor
    Parameter():m_name(""){}
private:
  /// The type of the property
    std::string m_type;
  /// The name of the property
  std::string m_name;
  std::string m_str_value; ///< Parameter value as a string
};

/// Templated class for parameters of type \c Type
template<class Type>
class DLLExport ParameterType:public Parameter
{
public:
	/// Returns the value of the property as a string
    std::string asString() const
    {
        std::ostringstream str;
        str << m_value;
        return str.str();
    }

	/// Set the value of the property via a string
    void fromString( const std::string& value )
    {
        std::istringstream istr(value);
        istr >> m_value;
    }

    /// Returns the value of the parameter
    const Type& value()const{return m_value;}

    /// Set the value of the parameter
    void setValue(const Type& value){ m_value = value; }

    /// Set the value of the parameter
    ParameterType& operator=(const Type& value)
    {
        setValue( value );
        return *this;
    }

    /// Get the value of the parameter
    const Type& operator()()const{return m_value;}

protected:
    friend class Kernel::Instantiator<ParameterType<Type>,Parameter>;
    /// Constructor
    ParameterType():Parameter(){}
private:
    Type m_value;///< Value
};

/// Typedef for the shared pointer
typedef boost::shared_ptr<Parameter> Parameter_sptr;

template<class T>
const T& Parameter::value()
{
    ParameterType<T> *p = dynamic_cast<ParameterType<T>*>(this);
    if (!p) throw std::runtime_error("Wrong type of parameter.");
    return p->ParameterType<T>::value();
}

template<class T>
void Parameter::set(const T& t)
{
    ParameterType<T> *p = dynamic_cast<ParameterType<T>*>(this);
    if (!p) throw std::runtime_error("Wrong type of parameter.");
    p->ParameterType<T>::setValue(t);
}

/// Parameter of type int
typedef DLLExport ParameterType<int> ParameterInt;
/// Parameter of type double
typedef DLLExport ParameterType<double> ParameterDouble;
/// Parameter of type bool
typedef DLLExport ParameterType<bool> ParameterBool;
/// Parameter of type std::string
typedef DLLExport ParameterType<std::string> ParameterString;
/// Parameter of type V3D
typedef DLLExport ParameterType<V3D> ParameterV3D;
/// Parameter of type Quat
typedef DLLExport ParameterType<Quat> ParameterQuat;



} // namespace Geometry
} // namespace Mantid

#define DECLARE_PARAMETER(classname,classtype) \
	namespace { \
	Mantid::Kernel::RegistrationHelper register_par_##classname( \
    ((Mantid::Geometry::ParameterFactory::subscribe< Mantid::Geometry::ParameterType<classtype> >(#classname)) \
	, 0)); \
  }

#endif /*MANTID_GEOMETRY_PARAMETER_H_*/
