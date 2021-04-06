// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

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
*/
class MANTID_GEOMETRY_DLL Parameter {
public:
  /// Virtual destructor
  virtual ~Parameter() = default;

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
  /// set description:
  virtual void setDescription(const std::string &source) { m_description.assign(source); }
  /// get description
  virtual const std::string &getDescription() const { return m_description; }
  /// get short description
  virtual std::string getShortDescription() const;
  /// Equality operator
  bool operator==(const Parameter &rhs) const {
    if (this->name() == rhs.name() && this->type() == rhs.type() && this->asString() == rhs.asString())
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
  Parameter() : m_type(""), m_name(""), m_str_value(""), m_description("") {}

private:
  /// The type of the property
  std::string m_type;
  /// The name of the property
  std::string m_name;
  std::string m_str_value; ///< Parameter value as a string
  /// parameter's description -- string containing the description
  /// of this parameter
  std::string m_description;
};

/// Templated class for parameters of type \c Type
template <class Type> class DLLExport ParameterType : public Parameter {
public:
  /// Constructor
  ParameterType() : Parameter(), m_value() {}

  /// Returns the value of the property as a string
  std::string asString() const override;
  /// Set the value of the property via a string
  void fromString(const std::string &value) override;

  /// Returns the value of the parameter
  inline const Type &value() const { return m_value; }
  /// Get the value of the parameter
  inline const Type &operator()() const { return m_value; }

  Parameter *clone() const override { return new ParameterType(*this); }

private:
  friend class ParameterMap;
  friend class Parameter;
  /// Set the value of the parameter
  void setValue(const Type &value);
  /// Set the value of the parameter
  ParameterType &operator=(const Type &value);

private:
  Type m_value; ///< Value
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

/**
 * Set the value of the parameter from a string
 * @tparam T The type of the parameter
 * @param value :: A string representation of the parameter's value
 */
template <class Type> void ParameterType<Type>::fromString(const std::string &value) {
  std::istringstream istr(value);
  istr >> m_value;
}

/**
 * Specialization for a string.
 */
template <> inline void ParameterType<std::string>::fromString(const std::string &value) { m_value = value; }

/** Set the value of the parameter via the assignment operator
 * @tparam The parameter type
 * @param value :: The vlue of the parameter
 */
template <class Type> void ParameterType<Type>::setValue(const Type &value) { m_value = value; }

/** Set the value of the parameter via the assignment operator
 * @param value :: The value of the parameter
 * @returns A reference to the parameter
 */
template <class Type> ParameterType<Type> &ParameterType<Type>::operator=(const Type &value) {
  setValue(value);
  return *this;
}

/// Typedef for the shared pointer
using Parameter_sptr = std::shared_ptr<Parameter>;

} // namespace Geometry
} // namespace Mantid
