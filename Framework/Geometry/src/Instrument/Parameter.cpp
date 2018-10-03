// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/RegistrationHelper.h"
#include "MantidKernel/V3D.h"
#include <sstream>

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

namespace Mantid {
namespace Geometry {

/** Return the value of the parameter as a string
 * @tparam T The type of the parameter
 * @returns A string representation of the parameter
 */
template <class Type> std::string ParameterType<Type>::asString() const {
  std::ostringstream str;
  str << m_value;
  return str.str();
}

/// Specialization for doubles
template <> std::string ParameterType<double>::asString() const {
  std::ostringstream str;
  str << std::setprecision(std::numeric_limits<double>::digits10);
  str << m_value;
  return str.str();
}

/// Specialization for V3D
template <> std::string ParameterType<Kernel::V3D>::asString() const {
  std::ostringstream str;
  str << std::setprecision(std::numeric_limits<double>::digits10);
  str << m_value;
  return str.str();
}

// Initialize the static map
ParameterFactory::FactoryMap ParameterFactory::s_map;

/**@return short description of the property (e.g. tooltip).
   The short description defined as all in the full description
   up to double LF symbol.
   (Python agreement on a short description of a function or method)

   If no double LF symbol found in the description string, full string
   is returned.
*/
std::string Parameter::getShortDescription() const {
  size_t pos = m_description.find('.');
  if (pos == std::string::npos) {
    return std::string(m_description);
  } else {
    if (pos > 0) {
      return m_description.substr(0, pos + 1);
    } else {
      return std::string("");
    }
  }
}

/**  Creates an instance of a parameter
 *   @param className :: The parameter registered type name
 *   @param name :: The parameter name
 *   @return A pointer to the created parameter
 *   @throw runtime_error if the type has not been registered
 */
boost::shared_ptr<Parameter>
ParameterFactory::create(const std::string &className,
                         const std::string &name) {
  boost::shared_ptr<Parameter> p;
  FactoryMap::const_iterator it = s_map.find(className);
  if (it != s_map.end())
    p = it->second->createInstance();
  else
    throw std::runtime_error("ParameterFactory:" + className +
                             " is not registered.\n");
  p->m_name = name;
  p->m_type = className;
  return p;
}

} // Namespace Geometry
} // Namespace Mantid

DECLARE_PARAMETER(int, int)
DECLARE_PARAMETER(double, double)
DECLARE_PARAMETER(bool, bool)
DECLARE_PARAMETER(string, std::string)
DECLARE_PARAMETER(V3D, Mantid::Kernel::V3D)
DECLARE_PARAMETER(Quat, Mantid::Kernel::Quat)
DECLARE_PARAMETER(fitting, Mantid::Geometry::FitParameter)
