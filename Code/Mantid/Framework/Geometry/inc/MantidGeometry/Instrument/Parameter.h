#ifndef MANTID_GEOMETRY_PARAMETER_H_
#define MANTID_GEOMETRY_PARAMETER_H_

/* Register classes into the factory
 *
 */
#define DECLARE_PARAMETER(classname, classtype)                                \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_par_##classname(                 \
      ((Mantid::Geometry::ParameterFactory::subscribe<                         \
           Mantid::Geometry::ParameterType<classtype>>(#classname)),           \
       0));                                                                    \
  }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/RegistrationHelper.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif
#include <string>
#include <typeinfo>
#include <vector>
#include <stdexcept>

namespace Mantid {

namespace Kernel {
template <class C, class Base> class Instantiator;
}

namespace Geometry {
//--------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------
class ParameterMap;

/** @class Parameter Parameter.h Geometry/Parameter.h

  Base class for parameters of an instrument.

  @author Roman Tolchenov, Tessella Support Services plc
  @date 2/12/2008

  Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class MANTID_GEOMETRY_DLL Parameter {
public:
  /// Virtual destructor
  virtual ~Parameter() {}

  /// Parameter type
  const std::string &type() const { return m_type; }
  /// Parameter name
  const std::string &name() const { return m_name; }
  /// Parameter name
  const char *nameAsCString() const { return m_name.c_str(); }

  /// type-independent clone method;
  virtual Parameter *clone() const = 0;

  /// Returns the value of the property as a string
  virtual std::string asString() const { return m_str_value; }

  /// Set the value of the property via a string
  virtual void fromString(const std::string &value) { m_str_value = value; }

  /// Returns the parameter value of type T if the parameter has type
  /// ParameterType<T>
  template <class T> const T &value();

  /// Equality operator
  bool operator==(const Parameter &rhs) const {
    if (this->name() == rhs.name() && this->type() == rhs.type() &&
        this->asString() == rhs.asString())
      return true;
    else
      return false;
  }

protected:
  friend class ParameterMap;

  /** Sets the value of type T to the parameter if it has type ParameterType<T>
      Throws an exception if the types don't match.
      @param t :: Value to set
  */
  template <class T> void set(const T &t);

  friend class ParameterFactory;
  /// Constructor
  Parameter() : m_type(""), m_name(""), m_str_value("") {}

private:
  /// The type of the property
  std::string m_type;
  /// The name of the property
  std::string m_name;
  std::string m_str_value; ///< Parameter value as a string
};


/// Templated class for parameters of type \c Type
template <class Type> class MANTID_GEOMETRY_DLL ParameterType : public Parameter {
public:
  /// Returns the value of the property as a string
  std::string asString() const;
  /// Set the value of the property via a string
  void fromString(const std::string &value);

  /// Returns the value of the parameter
  inline const Type &value() const { return m_value; }
  /// Get the value of the parameter
  inline const Type &operator()() const { return m_value; }

  Parameter *clone() const { return new ParameterType(*this); }

private:
  friend class ParameterMap;
  friend class Parameter;
  /// Set the value of the parameter
  void setValue(const Type &value);
  /// Set the value of the parameter
  ParameterType &operator=(const Type &value);

protected:
  friend class Kernel::Instantiator<ParameterType<Type>, Parameter>;
  /// Constructor
  ParameterType() : Parameter(), m_value() {}

private:
  Type m_value; ///< Value
};

template <class Type> class MANTID_GEOMETRY_DLL ParameterWithHelp : public ParameterType<Type> {
public:

  Parameter *clone() const { return new ParameterWithHelp(*this); }

protected:
  /// Constructor
  ParameterWithHelp() : ParameterType<Type>(){}

private:
  /// The string containing full description of the property
  std::string m_description;
  /// The name of the property
  std::string m_name;


};

//--------------------------------------------------------------------------
// Template definitions - Parameter class
//--------------------------------------------------------------------------

/**
 * The value of the parameter
 * @tparam T The concrete type
 * @returns A const reference to the value of the parameter
 */
template <class T> const T &Parameter::value() {
  ParameterType<T> *p = dynamic_cast<ParameterType<T> *>(this);
  if (!p)
    throw std::runtime_error("Wrong type of parameter.");
  return p->ParameterType<T>::value();
}

/** Sets the value of the parameter
 * @tparam T The concrete type
 * @param t :: value of the parameter
 */
template <class T> void Parameter::set(const T &t) {
  ParameterType<T> *p = dynamic_cast<ParameterType<T> *>(this);
  if (!p)
    throw std::runtime_error("Wrong type of parameter.");
  p->ParameterType<T>::setValue(t);
}

//--------------------------------------------------------------------------
// Template definitions - ParameterType class
//--------------------------------------------------------------------------

/** Return the value of the parameter as a string
 * @tparam T The type of the parameter
 * @returns A string representation of the parameter
 */
template <class Type> std::string ParameterType<Type>::asString() const {
  std::ostringstream str;
  str << m_value;
  return str.str();
}

/**
 * Set the value of the parameter from a string
 * @tparam T The type of the parameter
 * @param value :: A string representation of the parameter's value
 */
template <class Type>
void ParameterType<Type>::fromString(const std::string &value) {
  std::istringstream istr(value);
  istr >> m_value;
}

/** Set the value of the parameter via the assignment operator
 * @tparam The parameter type
 * @param value :: The vlue of the parameter
 */
template <class Type> void ParameterType<Type>::setValue(const Type &value) {
  m_value = value;
}

/** Set the value of the parameter via the assignment operator
 * @param value :: The value of the parameter
 * @returns A reference to the parameter
 */
template <class Type>
ParameterType<Type> &ParameterType<Type>::operator=(const Type &value) {
  setValue(value);
  return *this;
}

/// Typedef for the shared pointer
typedef boost::shared_ptr<Parameter> Parameter_sptr;
/// Parameter of type int
typedef MANTID_GEOMETRY_DLL ParameterType<int> ParameterInt;
/// Parameter of type double
typedef MANTID_GEOMETRY_DLL ParameterType<double> ParameterDouble;
/// Parameter of type bool
typedef MANTID_GEOMETRY_DLL ParameterType<bool> ParameterBool;
/// Parameter of type std::string
typedef MANTID_GEOMETRY_DLL ParameterType<std::string> ParameterString;
/// Parameter of type V3D
typedef MANTID_GEOMETRY_DLL ParameterType<Kernel::V3D> ParameterV3D;
/// Parameter of type Quat
typedef MANTID_GEOMETRY_DLL ParameterType<Kernel::Quat> ParameterQuat;

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_PARAMETER_H_*/
