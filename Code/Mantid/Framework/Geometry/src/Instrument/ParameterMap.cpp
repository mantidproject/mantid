#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/NearestNeighbours.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidGeometry/Instrument.h"
#include <cstring>
#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace Geometry {
using Kernel::V3D;
using Kernel::Quat;

namespace {
// names of common parameter types
const std::string POS_PARAM_NAME = "pos";
const std::string POSX_PARAM_NAME = "x";
const std::string POSY_PARAM_NAME = "y";
const std::string POSZ_PARAM_NAME = "z";

const std::string ROT_PARAM_NAME = "rot";
const std::string ROTX_PARAM_NAME = "rotx";
const std::string ROTY_PARAM_NAME = "roty";
const std::string ROTZ_PARAM_NAME = "rotz";

const std::string DOUBLE_PARAM_NAME = "double";
const std::string INT_PARAM_NAME = "int";
const std::string BOOL_PARAM_NAME = "bool";
const std::string STRING_PARAM_NAME = "string";
const std::string V3D_PARAM_NAME = "V3D";
const std::string QUAT_PARAM_NAME = "Quat";

// static logger reference
Kernel::Logger g_log("ParameterMap");
}
//--------------------------------------------------------------------------
// Public method
//--------------------------------------------------------------------------
/**
 * Default constructor
 */
ParameterMap::ParameterMap() : m_parameterFileNames(), m_map() {}

/**
* Return string to be inserted into the parameter map
*/
// Position
const std::string &ParameterMap::pos() { return POS_PARAM_NAME; }

const std::string &ParameterMap::posx() { return POSX_PARAM_NAME; }

const std::string &ParameterMap::posy() { return POSY_PARAM_NAME; }

const std::string &ParameterMap::posz() { return POSZ_PARAM_NAME; }

// Rotation
const std::string &ParameterMap::rot() { return ROT_PARAM_NAME; }

const std::string &ParameterMap::rotx() { return ROTX_PARAM_NAME; }

const std::string &ParameterMap::roty() { return ROTY_PARAM_NAME; }

const std::string &ParameterMap::rotz() { return ROTZ_PARAM_NAME; }

// Other types
const std::string &ParameterMap::pDouble() { return DOUBLE_PARAM_NAME; }

const std::string &ParameterMap::pInt() { return INT_PARAM_NAME; }

const std::string &ParameterMap::pBool() { return BOOL_PARAM_NAME; }

const std::string &ParameterMap::pString() { return STRING_PARAM_NAME; }

const std::string &ParameterMap::pV3D() { return V3D_PARAM_NAME; }

const std::string &ParameterMap::pQuat() { return QUAT_PARAM_NAME; }

/**
 * Compares the values in this object with that given for inequality
 * The order of the values in the map is not important.
 * @param rhs A reference to a ParameterMap object to compare it to
 * @return true if the objects are considered not equal, false otherwise
 */
bool ParameterMap::operator!=(const ParameterMap &rhs) const {
  return !(this->operator==(rhs));
}

/**
 * Compares the values in this object with that given for equality
 * The order of the values in the map is not important.
 * @param rhs A reference to a ParameterMap object to compare it to
 * @return true if the objects are considered equal, false otherwise
 */
bool ParameterMap::operator==(const ParameterMap &rhs) const {
  if (this == &rhs)
    return true; // True for the same object

  // Quick size check
  if (this->size() != rhs.size())
    return false;

  // The map is unordered and the key is only valid at runtime. The
  // asString method turns the ComponentIDs to full-qualified name identifiers
  // so we will use the same approach to compare them

  pmap_cit thisEnd = this->m_map.end();
  pmap_cit rhsEnd = rhs.m_map.end();
  for (pmap_cit thisIt = this->m_map.begin(); thisIt != thisEnd; ++thisIt) {
    const IComponent *comp = static_cast<IComponent *>(thisIt->first);
    const std::string fullName = comp->getFullName();
    const auto &param = thisIt->second;
    bool match(false);
    for (pmap_cit rhsIt = rhs.m_map.begin(); rhsIt != rhsEnd; ++rhsIt) {
      const IComponent *rhsComp = static_cast<IComponent *>(rhsIt->first);
      const std::string rhsFullName = rhsComp->getFullName();
      if (fullName == rhsFullName && (*param) == (*rhsIt->second)) {
        match = true;
        break;
      }
    }
    if (!match)
      return false;
  }
  return true;
}

/** Get the component description by name
   *  @param compName :: The name of the component
   *  @param name :: The name of the parameter
   *  @return :: the description for the first parameter found and
   *  having non-empty description,
   *  or empty string if no description found.
*/
const std::string ParameterMap::getDescription(const std::string &compName,
                                const std::string &name) const{
    pmap_cit it;
    std::string result("");
    for (it = m_map.begin(); it != m_map.end(); ++it) {
      if (compName.compare(((const IComponent *)(*it).first)->getName()) == 0) {
        boost::shared_ptr<Parameter> param =
            get((const IComponent *)(*it).first, name);
        if (param){
          result = param->getDescription();
          if(!result.empty())
            return result;
        }
      }
    }
    return result;
}
/** Get the component tooltip by name
   *  @param compName :: The name of the component
   *  @param name :: The name of the parameter
   *  @return :: the tooltip (short description) for the first parameter 
   *  found and having non-empty description,
   *  or empty string if no description found.
*/
const std::string ParameterMap::getTooltip(const std::string &compName,
                                const std::string &name) const{
    pmap_cit it;
    std::string result("");
    for (it = m_map.begin(); it != m_map.end(); ++it) {
      if (compName.compare(((const IComponent *)(*it).first)->getName()) == 0) {
        boost::shared_ptr<Parameter> param =
            get((const IComponent *)(*it).first, name);
        if (param){
          result = param->getTooltip();
          if(!result.empty())
            return result;
        }
      }
    }
    return result;
}
/**
 * Output information that helps understanding the mismatch between two
 * parameter maps.
 * To loop through the difference between two very large parameter map can take
 * time, in which
 * you can a hit to what causes the difference faster setting firstDiffOnly to
 * true
 * @param rhs A reference to a ParameterMap object to compare it to
 * @param firstDiffOnly If true return only first difference found
 * @return diff as a string
 */
const std::string ParameterMap::diff(const ParameterMap &rhs,
                                     const bool &firstDiffOnly) const {
  if (this == &rhs)
    return std::string(""); // True for the same object

  // Quick size check
  if (this->size() != rhs.size()) {
    return std::string("Number of parameters does not match: ") +
           boost::lexical_cast<std::string>(this->size()) + " not equal to " +
           boost::lexical_cast<std::string>(rhs.size());
  }

  // Run this same loops as in operator==
  // The map is unordered and the key is only valid at runtime. The
  // asString method turns the ComponentIDs to full-qualified name identifiers
  // so we will use the same approach to compare them

  std::stringstream strOutput;
  pmap_cit thisEnd = this->m_map.end();
  pmap_cit rhsEnd = rhs.m_map.end();
  for (pmap_cit thisIt = this->m_map.begin(); thisIt != thisEnd; ++thisIt) {
    const IComponent *comp = static_cast<IComponent *>(thisIt->first);
    const std::string fullName = comp->getFullName();
    const auto &param = thisIt->second;
    bool match(false);
    for (pmap_cit rhsIt = rhs.m_map.begin(); rhsIt != rhsEnd; ++rhsIt) {
      const IComponent *rhsComp = static_cast<IComponent *>(rhsIt->first);
      const std::string rhsFullName = rhsComp->getFullName();
      if (fullName == rhsFullName && (*param) == (*rhsIt->second)) {
        match = true;
        break;
      }
    }

    if (!match) {
      // output some information that helps with understanding the mismatch
      strOutput << "Parameter mismatch LHS=RHS for LHS parameter in component "
                   "with name: " << fullName
                << ". Parameter name is: " << (*param).name()
                << " and value: " << (*param).asString() << std::endl;
      bool componentWithSameNameRHS = false;
      bool parameterWithSameNameRHS = false;
      for (pmap_cit rhsIt = rhs.m_map.begin(); rhsIt != rhsEnd; ++rhsIt) {
        const IComponent *rhsComp = static_cast<IComponent *>(rhsIt->first);
        const std::string rhsFullName = rhsComp->getFullName();
        if (fullName == rhsFullName) {
          componentWithSameNameRHS = true;
          if ((*param).name() == (*rhsIt->second).name()) {
            parameterWithSameNameRHS = true;
            strOutput << "RHS param with same name has value: "
                      << (*rhsIt->second).asString() << std::endl;
          }
        }
      }
      if (!componentWithSameNameRHS) {
        strOutput << "No matching RHS component name" << std::endl;
      }
      if (componentWithSameNameRHS && !parameterWithSameNameRHS) {
        strOutput << "Found matching RHS component name but not parameter name"
                  << std::endl;
      }
      if (firstDiffOnly)
        return strOutput.str();
    }
  }
  return strOutput.str();
}

/**
 * Clear any parameters with the given name
 * @param name :: The name of the parameter
 */
void ParameterMap::clearParametersByName(const std::string &name) {
  // Key is component ID so have to search through whole lot
  for (pmap_it itr = m_map.begin(); itr != m_map.end();) {
    if (itr->second->name() == name) {
      m_map.erase(itr++);
    } else {
      ++itr;
    }
  }
  // Check if the caches need invalidating
  if (name == pos() || name == rot())
    clearPositionSensitiveCaches();
}

/**
 * Clear any parameters with the given name for a specified component
 * @param name :: The name of the parameter
 * @param comp :: The component to clear parameters from
 */
void ParameterMap::clearParametersByName(const std::string &name,
                                         const IComponent *comp) {
  if (!m_map.empty()) {
    const ComponentID id = comp->getComponentID();
    pmap_it it_found = m_map.find(id);
    if (it_found != m_map.end()) {
      if (it_found->second->name() == name) {
        m_map.erase(it_found++);
      } else {
        ++it_found;
      }
    }

    // Check if the caches need invalidating
    if (name == pos() || name == rot())
      clearPositionSensitiveCaches();
  }
}

/**
 * Add a value into the map
 * @param type :: A string denoting the type, e.g. double, string, fitting
 * @param comp :: A pointer to the component that this parameter is attached to
 * @param name :: The name of the parameter
 * @param value :: The parameter's value
 */
void ParameterMap::add(const std::string &type, const IComponent *comp,
                       const std::string &name, const std::string &value,
                       const std::string * const pDescription) {
  auto param = ParameterFactory::create(type, name);
  param->fromString(value);
  this->add(comp, param,pDescription);
}

/** Method for adding/replacing a parameter providing shared pointer to it.
* @param comp :: A pointer to the component that this parameter is attached to
* @param par  :: a shared pointer to existing parameter. The ParameterMap stores
* share pointer and increment ref count to it
* @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
* description. If provided, the contents of the string is copied to the parameters
* memory
*/
void ParameterMap::add(const IComponent *comp,
                       const boost::shared_ptr<Parameter> &par,
                       const std::string * const pDescription) {
  // can not add null pointer
  if (!par)
    return;
  if (pDescription)
    par->setDescription(*pDescription);

  PARALLEL_CRITICAL(m_mapAccess) {
    auto existing_par = positionOf(comp, par->name().c_str(), "");
    // As this is only an add method it should really throw if it already
    // exists.
    // However, this is old behavior and many things rely on this actually be
    // an
    // add/replace-style function
    if (existing_par != m_map.end()) {
      existing_par->second = par;
    } else {
      m_map.insert(std::make_pair(comp->getComponentID(), par));
    }
  }
}

/** Create or adjust "pos" parameter for a component
 * Assumed that name either equals "x", "y" or "z" otherwise this
 * method will not add or modify "pos" parameter
 * @param comp :: Component
 * @param name :: name of the parameter
 * @param value :: value
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameters
 * memory
  */
void ParameterMap::addPositionCoordinate(const IComponent *comp,
                                         const std::string &name,
                                         const double value,
                                         const std::string * const pDescription) {
  Parameter_sptr param = get(comp, pos());
  V3D position;
  if (param) {
    // so "pos" already defined
    position = param->value<V3D>();
  } else {
    // so "pos" is not defined - therefore get position from component
    position = comp->getPos();
  }

  // adjust position

  if (name.compare(posx()) == 0)
    position.setX(value);
  else if (name.compare(posy()) == 0)
    position.setY(value);
  else if (name.compare(posz()) == 0)
    position.setZ(value);
  else {
    g_log.warning() << "addPositionCoordinate() called with unrecognized "
                       "coordinate symbol: " << name;
  // set description if one is provided
  if(pDescription){
    param->setDescription(*pDescription);
  }
    return;
  }

  // clear the position cache
  clearPositionSensitiveCaches();
  // finally add or update "pos" parameter
  addV3D(comp, pos(), position);
}

/** Create or adjust "rot" parameter for a component
 * Assumed that name either equals "rotx", "roty" or "rotz" otherwise this
 * method will not add/modify "rot" parameter
 * @param comp :: Component
 * @param name :: Parameter name
 * @param deg :: Parameter value in degrees
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameter's
 * memory
*/
void ParameterMap::addRotationParam(const IComponent *comp,
                                    const std::string &name, const double deg,
                                    const std::string * const pDescription) {
  Parameter_sptr paramRotX = get(comp, rotx());
  Parameter_sptr paramRotY = get(comp, roty());
  Parameter_sptr paramRotZ = get(comp, rotz());
  double rotX, rotY, rotZ;

  if (paramRotX)
    rotX = paramRotX->value<double>();
  else
    rotX = 0.0;

  if (paramRotY)
    rotY = paramRotY->value<double>();
  else
    rotY = 0.0;

  if (paramRotZ)
    rotZ = paramRotZ->value<double>();
  else
    rotZ = 0.0;

  // adjust rotation
  Quat quat;
  if (name.compare(rotx()) == 0) {
    addDouble(comp, rotx(), deg);
    quat = Quat(deg, V3D(1, 0, 0)) * Quat(rotY, V3D(0, 1, 0)) *
           Quat(rotZ, V3D(0, 0, 1));
  } else if (name.compare(roty()) == 0) {
    addDouble(comp, roty(), deg);
    quat = Quat(rotX, V3D(1, 0, 0)) * Quat(deg, V3D(0, 1, 0)) *
           Quat(rotZ, V3D(0, 0, 1));
  } else if (name.compare(rotz()) == 0) {
    addDouble(comp, rotz(), deg);
    quat = Quat(rotX, V3D(1, 0, 0)) * Quat(rotY, V3D(0, 1, 0)) *
           Quat(deg, V3D(0, 0, 1));
  } else {
    g_log.warning()
        << "addRotationParam() called with unrecognized coordinate symbol: "
        << name;
    return;
  }

  // clear the position cache
  clearPositionSensitiveCaches();

  // finally add or update "pos" parameter
  addQuat(comp, rot(), quat);
}

/**
 * Adds a double value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a string
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameter's
 * memory
*/
void ParameterMap::addDouble(const IComponent *comp, const std::string &name,
                             const std::string &value,const std::string * const pDescription) {
  add(pDouble(), comp, name, value,pDescription);
}

/**
 * Adds a double value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a double
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameter's
 * memory
*/
void ParameterMap::addDouble(const IComponent *comp, const std::string &name,
                             double value,const std::string * const pDescription) {
  add(pDouble(), comp, name, value,pDescription);
}

/**
 * Adds an int value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a string
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameters
 * memory
*/
void ParameterMap::addInt(const IComponent *comp, const std::string &name,
                          const std::string &value,const std::string * const pDescription) {
  add(pInt(), comp, name, value,pDescription);
}

/**
 * Adds an int value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as an int
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameters
 * memory
 */
void ParameterMap::addInt(const IComponent *comp, const std::string &name,
                          int value,const std::string * const pDescription) {
  add(pInt(), comp, name, value,pDescription);
}

/**
 * Adds a bool value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a string
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameters
 * memory
 */
void ParameterMap::addBool(const IComponent *comp, const std::string &name,
                           const std::string &value,const std::string * const pDescription) {
  add(pBool(), comp, name, value,pDescription);
}
/**
 * Adds a bool value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a bool
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameter's
 * memory
*/
void ParameterMap::addBool(const IComponent *comp, const std::string &name,
                           bool value,const std::string * const pDescription) {
  add(pBool(), comp, name, value,pDescription);
}

/**
 * Adds a std::string value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameter's
 * memory
*/
void ParameterMap::addString(const IComponent *comp, const std::string &name,
                             const std::string &value,const std::string * const pDescription) {
  add<std::string>(pString(), comp, name, value,pDescription);
}

/**
 * Adds a V3D value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a string
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameter's
 * memory
 */
void ParameterMap::addV3D(const IComponent *comp, const std::string &name,
                          const std::string &value,const std::string * const pDescription) {
  add(pV3D(), comp, name, value,pDescription);
  clearPositionSensitiveCaches();
}

/**
 * Adds a V3D value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a V3D
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameter's
 * memory
*/
void ParameterMap::addV3D(const IComponent *comp, const std::string &name,
                          const V3D &value,const std::string * const pDescription) {
  add(pV3D(), comp, name, value,pDescription);
  clearPositionSensitiveCaches();
}

/**
 * Adds a Quat value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a Quat
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's
 * description. If provided, the contents of the string is copied to the parameter's
 * memory
*/
void ParameterMap::addQuat(const IComponent *comp, const std::string &name,
                           const Quat &value,const std::string * const pDescription) {
  add(pQuat(), comp, name, value,pDescription);
  clearPositionSensitiveCaches();
}

/**
 * Does the named parameter exist for the given component and given type
 * @param comp :: The component to be searched
 * @param name :: The name of the parameter
 * @param type :: The type of the component as a string
 * @returns A boolean indicating if the map contains the named parameter. If the
 * type is given then
 * this must also match
 */
bool ParameterMap::contains(const IComponent *comp, const std::string &name,
                            const std::string &type) const {
  if (m_map.empty())
    return false;
  return contains(comp, name.c_str(), type.c_str());
}

/**
 * Avoids having to instantiate temporary std::string in method below when
 * called with a string directly
 * @param comp :: The component to be searched as a c-string
 * @param name :: The name of the parameter
 * @param type :: The type of the component
 * @return A boolean indicating if the map contains the named parameter.
 */
bool ParameterMap::contains(const IComponent *comp, const char *name,
                            const char *type) const {
  if (m_map.empty())
    return false;
  const ComponentID id = comp->getComponentID();
  std::pair<pmap_cit, pmap_cit> components = m_map.equal_range(id);
  bool anytype = (strlen(type) == 0);
  for (pmap_cit itr = components.first; itr != components.second; ++itr) {
    boost::shared_ptr<Parameter> param = itr->second;
    if (boost::iequals(param->name(), name) &&
        (anytype || param->type() == type)) {
      return true;
    }
  }
  return false;
}

/**
 * @param comp A pointer to a component
 * @param parameter A Parameter object
 * @return true if the combination exists in the map, false otherwise
 */
bool ParameterMap::contains(const IComponent *comp,
                            const Parameter &parameter) const {
  if (m_map.empty() || !comp)
    return false;

  const ComponentID id = comp->getComponentID();
  pmap_cit it_found = m_map.find(id);
  if (it_found != m_map.end()) {
    pmap_cit itr = m_map.lower_bound(id);
    pmap_cit itr_end = m_map.upper_bound(id);
    for (; itr != itr_end; ++itr) {
      const Parameter_sptr &param = itr->second;
      if (*param == parameter)
        return true;
    }
    return false;
  } else
    return false;
}

/** Return a named parameter of a given type
 * @param comp :: Component to which parameter is related
 * @param name :: Parameter name
 * @param type :: An optional type string
 * @returns The named parameter of the given type if it exists or a NULL shared
 * pointer if not
 */
Parameter_sptr ParameterMap::get(const IComponent *comp,
                                 const std::string &name,
                                 const std::string &type) const {
  return get(comp, name.c_str(), type.c_str());
}

/** Return a named parameter of a given type. Avoids allocating std::string
 * temporaries
 * @param comp :: Component to which parameter is related
 * @param name :: Parameter name
 * @param type :: An optional type string
 * @returns The named parameter of the given type if it exists or a NULL shared
 * pointer if not
 */
boost::shared_ptr<Parameter> ParameterMap::get(const IComponent *comp,
                                               const char *name,
                                               const char *type) const {
  Parameter_sptr result;
  if (!comp)
    return result;

  PARALLEL_CRITICAL(m_mapAccess) {
    auto itr = positionOf(comp, name, type);
    if (itr != m_map.end())
      result = itr->second;
  }
  return result;
}

/**Return an iterator pointing to a named parameter of a given type.
 * @param comp :: Component to which parameter is related
 * @param name :: Parameter name
 * @param type :: An optional type string. If empty, any type is returned
 * @returns The iterator parameter of the given type if it exists or a NULL
 * shared pointer if not
*/
component_map_it ParameterMap::positionOf(const IComponent *comp,
                                          const char *name, const char *type) {
  pmap_it result = m_map.end();
  if (!comp)
    return result;
  const bool anytype = (strlen(type) == 0);
  if (!m_map.empty()) {
    const ComponentID id = comp->getComponentID();
    pmap_it it_found = m_map.find(id);
    if (it_found != m_map.end()) {
      pmap_it itr = m_map.lower_bound(id);
      pmap_it itr_end = m_map.upper_bound(id);
      for (; itr != itr_end; ++itr) {
        Parameter_sptr param = itr->second;
        if (boost::iequals(param->nameAsCString(), name) &&
            (anytype || param->type() == type)) {
          result = itr;
          break;
        }
      }
    }
  }
  return result;
}

/**Return a const iterator pointing to a named parameter of a given type.
 * @param comp :: Component to which parameter is related
 * @param name :: Parameter name
 * @param type :: An optional type string. If empty, any type is returned
 * @returns The iterator parameter of the given type if it exists or a NULL
 * shared pointer if not
*/
component_map_cit ParameterMap::positionOf(const IComponent *comp,
                                           const char *name,
                                           const char *type) const {
  pmap_cit result = m_map.end();
  if (!comp)
    return result;
  const bool anytype = (strlen(type) == 0);
  if (!m_map.empty()) {
    const ComponentID id = comp->getComponentID();
    pmap_cit it_found = m_map.find(id);
    if (it_found != m_map.end()) {
      pmap_cit itr = m_map.lower_bound(id);
      pmap_cit itr_end = m_map.upper_bound(id);
      for (; itr != itr_end; ++itr) {
        Parameter_sptr param = itr->second;
        if (boost::iequals(param->nameAsCString(), name) &&
            (anytype || param->type() == type)) {
          result = itr;
          break;
        }
      }
    }
  }
  return result;
}

/** Look for a parameter in the given component by the type of the parameter.
* @param comp :: Component to which parameter is related
* @param type :: Parameter type
* @returns The typed parameter if it exists or a NULL shared pointer if not
*/
Parameter_sptr ParameterMap::getByType(const IComponent *comp,
                                       const std::string &type) const {
  Parameter_sptr result = Parameter_sptr();
  PARALLEL_CRITICAL(m_mapAccess) {
    if (!m_map.empty()) {
      const ComponentID id = comp->getComponentID();
      pmap_cit it_found = m_map.find(id);
      if (it_found != m_map.end()) {
        if (it_found->first) {
          pmap_cit itr = m_map.lower_bound(id);
          pmap_cit itr_end = m_map.upper_bound(id);
          for (; itr != itr_end; ++itr) {
            Parameter_sptr param = itr->second;
            if (boost::iequals(param->type(), type)) {
              result = param;
              break;
            }
          }
        } // found->firdst
      }   // it_found != m_map.end()
    }     //!m_map.empty()
  }       // PARALLEL_CRITICAL(m_map_access)
  return result;
}

/** Looks recursively upwards in the component tree for the first instance of a
* component with a matching type.
* @param comp :: The component to start the search with
* @param type :: Parameter type
* @returns the first matching parameter.
*/
Parameter_sptr ParameterMap::getRecursiveByType(const IComponent *comp,
                                                const std::string &type) const {
  boost::shared_ptr<const IComponent> compInFocus(comp, NoDeleting());
  while (compInFocus != NULL) {
    Parameter_sptr param = getByType(compInFocus.get(), type);
    if (param) {
      return param;
    }
    compInFocus = compInFocus->getParent();
  }
  // Nothing was found!
  return Parameter_sptr();
}

/**
 * Find a parameter by name, recursively going up the component tree
 * to higher parents.
 * @param comp :: The component to start the search with
 * @param name :: Parameter name
 * @param type :: An optional type string
 * @returns the first matching parameter.
 */
Parameter_sptr ParameterMap::getRecursive(const IComponent *comp,
                                          const std::string &name,
                                          const std::string &type) const {
  return getRecursive(comp, name.c_str(), type.c_str());
}

/**
 * Find a parameter by name, recursively going up the component tree
 * to higher parents.
 * @param comp :: The component to start the search with
 * @param name :: Parameter name
 * @param type :: An optional type string
 * @returns the first matching parameter.
 */
Parameter_sptr ParameterMap::getRecursive(const IComponent *comp,
                                          const char *name,
                                          const char *type) const {
  Parameter_sptr result = this->get(comp->getComponentID(), name, type);
  if (result)
    return result;

  auto parent = comp->getParent();
  while (parent) {
    result = this->get(parent->getComponentID(), name, type);
    if (result)
      return result;
    parent = parent->getParent();
  }
  return result;
}

/**
 * Return the value of a parameter as a string
 * @param comp :: Component to which parameter is related
 * @param name :: Parameter name
 * @param recursive :: Whether to travel up the instrument tree if not found at
 * this level
 * @return string representation of the parameter
 */
std::string ParameterMap::getString(const IComponent *comp,
                                    const std::string &name,
                                    bool recursive) const {
  Parameter_sptr param = get(comp, name);
  if (recursive) {
    param = getRecursive(comp, name);
  } else {
    param = get(comp, name);
  }
  if (!param)
    return "";
  return param->asString();
}

/**
 * Returns a set with all the parameter names for the given component
 * @param comp :: A pointer to the component of interest
 * @returns A set of names of parameters for the given component
 */
std::set<std::string> ParameterMap::names(const IComponent *comp) const {
  std::set<std::string> paramNames;
  const ComponentID id = comp->getComponentID();
  pmap_cit it_found = m_map.find(id);
  if (it_found == m_map.end()) {
    return paramNames;
  }

  pmap_cit itr = m_map.lower_bound(id);
  pmap_cit itr_end = m_map.upper_bound(id);
  for (pmap_cit it = itr; it != itr_end; ++it) {
    paramNames.insert(it->second->name());
  }

  return paramNames;
}

/**
 * Return a string representation of the parameter map. The format is either:
 * |detID:id-value;param-type;param-name;param-value| for a detector or
 * |comp-name;param-type;param-name;param-value| for other components.
 * @returns A string containing the contents of the parameter map.
 */
std::string ParameterMap::asString() const {
  std::stringstream out;
  for (pmap_cit it = m_map.begin(); it != m_map.end(); it++) {
    boost::shared_ptr<Parameter> p = it->second;
    if (p && it->first) {
      const IComponent *comp = (const IComponent *)(it->first);
      const IDetector *det = dynamic_cast<const IDetector *>(comp);
      if (det) {
        out << "detID:" << det->getID();
      } else {
        out << comp->getFullName(); // Use full path name to ensure unambiguous
                                    // naming
      }
      out << ';' << p->type() << ';' << p->name() << ';' << p->asString()
          << '|';
    }
  }
  return out.str();
}

/**
 * Clears the location, rotation & bounding box caches
 */
void ParameterMap::clearPositionSensitiveCaches() {
  m_cacheLocMap.clear();
  m_cacheRotMap.clear();
  m_boundingBoxMap.clear();
}

/// Sets a cached location on the location cache
/// @param comp :: The Component to set the location of
/// @param location :: The location
void ParameterMap::setCachedLocation(const IComponent *comp,
                                     const V3D &location) const {
  // Call to setCachedLocation is a write so not thread-safe
  PARALLEL_CRITICAL(positionCache) {
    m_cacheLocMap.setCache(comp->getComponentID(), location);
  }
}

/// Attempts to retrieve a location from the location cache
/// @param comp :: The Component to find the location of
/// @param location :: If the location is found it's value will be set here
/// @returns true if the location is in the map, otherwise false
bool ParameterMap::getCachedLocation(const IComponent *comp,
                                     V3D &location) const {
  bool inMap(false);
  PARALLEL_CRITICAL(positionCache) {
    inMap = m_cacheLocMap.getCache(comp->getComponentID(), location);
  }
  return inMap;
}

/// Sets a cached rotation on the rotation cache
/// @param comp :: The Component to set the rotation of
/// @param rotation :: The rotation as a quaternion
void ParameterMap::setCachedRotation(const IComponent *comp,
                                     const Quat &rotation) const {
  // Call to setCachedRotation is a write so not thread-safe
  PARALLEL_CRITICAL(rotationCache) {
    m_cacheRotMap.setCache(comp->getComponentID(), rotation);
  }
}

/// Attempts to retrieve a rotation from the rotation cache
/// @param comp :: The Component to find the rotation of
/// @param rotation :: If the rotation is found it's value will be set here
/// @returns true if the rotation is in the map, otherwise false
bool ParameterMap::getCachedRotation(const IComponent *comp,
                                     Quat &rotation) const {
  bool inMap(false);
  PARALLEL_CRITICAL(rotationCache) {
    inMap = m_cacheRotMap.getCache(comp->getComponentID(), rotation);
  }
  return inMap;
}

/// Sets a cached bounding box
/// @param comp :: The Component to set the rotation of
/// @param box :: A reference to the bounding box
void ParameterMap::setCachedBoundingBox(const IComponent *comp,
                                        const BoundingBox &box) const {
  // Call to setCachedRotation is a write so not thread-safe
  PARALLEL_CRITICAL(boundingBoxCache) {
    m_boundingBoxMap.setCache(comp->getComponentID(), box);
  }
}

/// Attempts to retrieve a bounding box from the cache
/// @param comp :: The Component to find the bounding box of
/// @param box :: If the bounding box is found it's value will be set here
/// @returns true if the bounding is in the map, otherwise false
bool ParameterMap::getCachedBoundingBox(const IComponent *comp,
                                        BoundingBox &box) const {
  return m_boundingBoxMap.getCache(comp->getComponentID(), box);
}

/**
 * Copy pairs (oldComp->id,Parameter) to the m_map
 * assigning the new newComp->id
 * @param oldComp :: Old component
 * @param newComp :: New component
 * @param oldPMap :: Old map corresponding to the Old component
 */
void ParameterMap::copyFromParameterMap(const IComponent *oldComp,
                                        const IComponent *newComp,
                                        const ParameterMap *oldPMap) {

  std::set<std::string> oldParameterNames = oldPMap->names(oldComp);

  for (auto it = oldParameterNames.begin(); it != oldParameterNames.end();
       ++it) {
    Parameter_sptr thisParameter = oldPMap->get(oldComp, *it);
    // Insert the fetched parameter in the m_map
    m_map.insert(std::make_pair(newComp->getComponentID(), thisParameter));
  }
}

//--------------------------------------------------------------------------------------------
/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to create
 */
void ParameterMap::saveNexus(::NeXus::File *file,
                             const std::string &group) const {
  file->makeGroup(group, "NXnote", true);
  file->putAttr("version", 1);
  file->writeData("author", "");
  file->writeData("date",
                  Kernel::DateAndTime::getCurrentTime().toISO8601String());
  file->writeData("description", "A string representation of the parameter "
                                 "map. The format is either: "
                                 "|detID:id-value;param-type;param-name;param-"
                                 "value| for a detector or  "
                                 "|comp-name;param-type;param-name;param-value|"
                                 " for other components.");
  file->writeData("type", "text/plain");
  std::string s = this->asString();
  file->writeData("data", s);
  file->closeGroup();
}

/** Returns a list of all the parameter files loaded
* @returns a vector of the filenames
*/
const std::vector<std::string> &ParameterMap::getParameterFilenames() const {
  return m_parameterFileNames;
}
///
/** adds a parameter filename that has been loaded
* @param filename the filename to add
*/
void ParameterMap::addParameterFilename(const std::string &filename) {
  m_parameterFileNames.push_back(filename);
}

} // Namespace Geometry
} // Namespace Mantid
