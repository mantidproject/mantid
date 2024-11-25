// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include "MantidKernel/Cache.h"
#include "MantidKernel/MultiThreaded.h"
#include <boost/algorithm/string.hpp>
#include <cstring>
#include <nexus/NeXusFile.hpp>

#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include "strings.h"
#endif

namespace Mantid::Geometry {
using Kernel::Quat;
using Kernel::V3D;

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

const std::string SCALE_PARAM_NAME = "sca";

// static logger reference
Kernel::Logger g_log("ParameterMap");

void checkIsNotMaskingParameter(const std::string &name) {
  if (name == std::string("masked"))
    throw std::runtime_error("Masking data (\"masked\") cannot be stored in "
                             "ParameterMap. Use DetectorInfo instead");
}
} // namespace
/**
 * Default constructor
 */
ParameterMap::ParameterMap()
    : m_cacheLocMap(std::make_unique<Kernel::Cache<const ComponentID, Kernel::V3D>>()),
      m_cacheRotMap(std::make_unique<Kernel::Cache<const ComponentID, Kernel::Quat>>()) {}

ParameterMap::ParameterMap(const ParameterMap &other)
    : m_parameterFileNames(other.m_parameterFileNames), m_map(other.m_map),
      m_cacheLocMap(std::make_unique<Kernel::Cache<const ComponentID, Kernel::V3D>>(*other.m_cacheLocMap)),
      m_cacheRotMap(std::make_unique<Kernel::Cache<const ComponentID, Kernel::Quat>>(*other.m_cacheRotMap)),
      m_instrument(other.m_instrument) {
  if (m_instrument)
    std::tie(m_componentInfo, m_detectorInfo) = m_instrument->makeBeamline(*this, &other);
}

// Defined as default in source for forward declaration with std::unique_ptr.
ParameterMap::~ParameterMap() = default;

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

// Scale
const std::string &ParameterMap::scale() { return SCALE_PARAM_NAME; }

/**
 * Compares the values in this object with that given for inequality
 * The order of the values in the map is not important.
 * @param rhs A reference to a ParameterMap object to compare it to
 * @return true if the objects are considered not equal, false otherwise
 */
bool ParameterMap::operator!=(const ParameterMap &rhs) const { return !(this->operator==(rhs)); }

/**
 * Compares the values in this object with that given for equality
 * The order of the values in the map is not important.
 * @param rhs A reference to a ParameterMap object to compare it to
 * @return true if the objects are considered equal, false otherwise
 */
bool ParameterMap::operator==(const ParameterMap &rhs) const { return diff(rhs, true, false, 0.).empty(); }

/** Get the component description by name
 *  @param compName :: The name of the component
 *  @param name :: The name of the parameter
 *  @return :: the description for the first parameter found and
 *  having non-empty description,
 *  or empty string if no description found.
 */
const std::string ParameterMap::getDescription(const std::string &compName, const std::string &name) const {
  pmap_cit it;
  std::string result;
  for (it = m_map.begin(); it != m_map.end(); ++it) {
    if (compName == it->first->getName()) {
      std::shared_ptr<Parameter> param = get(it->first, name);
      if (param) {
        result = param->getDescription();
        if (!result.empty())
          return result;
      }
    }
  }
  return result;
}
/** Get the component short description by name
 *  @param compName :: The name of the component
 *  @param name :: The name of the parameter
 *  @return :: the short description for the first parameter
 *  found and having non-empty description,
 *  or empty string if no description found.
 */
const std::string ParameterMap::getShortDescription(const std::string &compName, const std::string &name) const {
  pmap_cit it;
  std::string result;
  for (it = m_map.begin(); it != m_map.end(); ++it) {
    if (compName == it->first->getName()) {
      std::shared_ptr<Parameter> param = get(it->first, name);
      if (param) {
        result = param->getShortDescription();
        if (!result.empty())
          return result;
      }
    }
  }
  return result;
}

//------------------------------------------------------------------------------------------------
/** Function which calculates relative error between two values and analyses if
this error is within the limits
* requested. When the absolute value of the difference is smaller then the value
of the error requested,
* absolute error is used instead of relative error.

@param x1       -- first value to check difference
@param x2       -- second value to check difference
@param errorVal -- the value of the error, to check against. Should  be large
then 0

@returns true if error or false if the value is within the limits requested
*/
bool ParameterMap::relErr(double x1, double x2, double errorVal) const {
  double num = std::fabs(x1 - x2);
  // how to treat x1<0 and x2 > 0 ?  probably this way
  double den = 0.5 * (std::fabs(x1) + std::fabs(x2));
  if (den < errorVal)
    return (num > errorVal);

  return (num / den > errorVal);
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
 * @param relative Indicates whether to treat the error as relative or absolute
 * @param doubleTolerance The tolerance to use when comparing parameter values
 * of type double
 * @return diff as a string
 */
const std::string ParameterMap::diff(const ParameterMap &rhs, const bool &firstDiffOnly, const bool relative,
                                     const double doubleTolerance) const {
  if (this == &rhs)
    return std::string(""); // True for the same object

  // Quick size check
  if (this->size() != rhs.size()) {
    return std::string("Number of parameters does not match: ") + std::to_string(this->size()) + " not equal to " +
           std::to_string(rhs.size());
  }

  // The map is unordered and the key is only valid at runtime. The
  // asString method turns the ComponentIDs to full-qualified name identifiers
  // so we will use the same approach to compare them

  std::unordered_multimap<std::string, Parameter_sptr> thisMap, rhsMap;
  for (auto &mappair : this->m_map) {
    thisMap.emplace(mappair.first->getFullName(), mappair.second);
  }
  for (auto &mappair : rhs.m_map) {
    rhsMap.emplace(mappair.first->getFullName(), mappair.second);
  }

  std::stringstream strOutput;
  for (auto thisIt = thisMap.cbegin(); thisIt != thisMap.cend(); ++thisIt) {
    const std::string fullName = thisIt->first;
    const auto &param = thisIt->second;
    bool match(false);
    for (auto rhsIt = rhsMap.cbegin(); rhsIt != rhsMap.cend(); ++rhsIt) {
      const std::string rhsFullName = rhsIt->first;
      const auto &rhsParam = rhsIt->second;
      if ((fullName == rhsFullName) && (param->name() == (rhsParam->name()))) {
        if ((param->type() == rhsParam->type()) && (rhsParam->type() == "double")) {
          if (relative) {
            if (!relErr(param->value<double>(), rhsParam->value<double>(), doubleTolerance))
              match = true;
          } else if (std::abs(param->value<double>() - rhsParam->value<double>()) <= doubleTolerance)
            match = true;
        } else if (param->asString() == rhsParam->asString()) {
          match = true;
        }
        if (match)
          break;
      }
    }

    if (!match) {
      // output some information that helps with understanding the mismatch
      strOutput << "Parameter mismatch LHS=RHS for LHS parameter in component "
                   "with name: "
                << fullName << ". Parameter name is: " << (*param).name() << " and value: " << (*param).asString()
                << '\n';
      bool componentWithSameNameRHS = false;
      bool parameterWithSameNameRHS = false;
      for (auto rhsIt = rhsMap.cbegin(); rhsIt != rhsMap.cend(); ++rhsIt) {
        const std::string rhsFullName = rhsIt->first;
        if (fullName == rhsFullName) {
          componentWithSameNameRHS = true;
          if ((*param).name() == (*rhsIt->second).name()) {
            parameterWithSameNameRHS = true;
            strOutput << "RHS param with same name has value: " << (*rhsIt->second).asString() << '\n';
          }
        }
      }
      if (!componentWithSameNameRHS) {
        strOutput << "No matching RHS component name\n";
      }
      if (componentWithSameNameRHS && !parameterWithSameNameRHS) {
        strOutput << "Found matching RHS component name but not parameter name\n";
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
  checkIsNotMaskingParameter(name);
  // Key is component ID so have to search through whole lot
  for (auto itr = m_map.begin(); itr != m_map.end();) {
    if (itr->second->name() == name) {
      PARALLEL_CRITICAL(unsafe_erase) { itr = m_map.unsafe_erase(itr); }
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
void ParameterMap::clearParametersByName(const std::string &name, const IComponent *comp) {
  checkIsNotMaskingParameter(name);
  if (!m_map.empty()) {
    const ComponentID id = comp->getComponentID();
    auto itrs = m_map.equal_range(id);
    for (auto it = itrs.first; it != itrs.second;) {
      if (it->second->name() == name) {
        PARALLEL_CRITICAL(unsafe_erase) { it = m_map.unsafe_erase(it); }
      } else {
        ++it;
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
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameters memory
 * @param pVisible :: Whether the parameter should be visible in InstrumentViewer
 */
void ParameterMap::add(const std::string &type, const IComponent *comp, const std::string &name,
                       const std::string &value, const std::string *const pDescription, const std::string &pVisible) {
  auto param = ParameterFactory::create(type, name, pVisible);
  param->fromString(value);
  this->add(comp, param, pDescription);
}

/** Method for adding/replacing a parameter providing shared pointer to it.
 * @param comp :: A pointer to the component that this parameter is attached to
 * @param par  :: a shared pointer to existing parameter. The ParameterMap stores share pointer and increment ref count
 * to it
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameters memory
 */
void ParameterMap::add(const IComponent *comp, const std::shared_ptr<Parameter> &par,
                       const std::string *const pDescription) {
  if (!par)
    return;
  checkIsNotMaskingParameter(par->name());
  if (pDescription)
    par->setDescription(*pDescription);

  auto existing_par = positionOf(comp, par->name().c_str(), "");
  // As this is only an add method it should really throw if it already
  // exists.
  // However, this is old behavior and many things rely on this actually be
  // an
  // add/replace-style function
  if (existing_par != m_map.end()) {
    std::atomic_store(&(existing_par->second), par);
  } else {
// When using Clang & Linux, TBB 4.4 doesn't detect C++11 features.
// https://software.intel.com/en-us/forums/intel-threading-building-blocks/topic/641658
#if defined(__clang__) && !defined(__APPLE__)
#define CLANG_ON_LINUX true
#else
#define CLANG_ON_LINUX false
#endif
#if TBB_VERSION_MAJOR >= 4 && TBB_VERSION_MINOR >= 4 && !CLANG_ON_LINUX
    m_map.emplace(comp->getComponentID(), par);
#else
    m_map.insert(std::make_pair(comp->getComponentID(), par));
#endif
  }
}

/** Create or adjust "pos" parameter for a component
 * Assumed that name either equals "x", "y" or "z" otherwise this
 * method will not add or modify "pos" parameter
 * @param comp :: Component
 * @param name :: name of the parameter
 * @param value :: value
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameters memory
 */
void ParameterMap::addPositionCoordinate(const IComponent *comp, const std::string &name, const double value,
                                         const std::string *const pDescription) {
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

  if (name == posx())
    position.setX(value);
  else if (name == posy())
    position.setY(value);
  else if (name == posz())
    position.setZ(value);
  else {
    g_log.warning() << "addPositionCoordinate() called with unrecognized "
                       "coordinate symbol: "
                    << name;
    // set description if one is provided
    if (pDescription) {
      param->setDescription(*pDescription);
    }
    return;
  }

  // clear the position cache
  clearPositionSensitiveCaches();
  // finally add or update "pos" parameter
  addV3D(comp, pos(), position, pDescription);
}

/** Create or adjust "rot" parameter for a component
 * Assumed that name either equals "rotx", "roty" or "rotz" otherwise this
 * method will not add/modify "rot" parameter
 * @param comp :: Component
 * @param name :: Parameter name
 * @param deg :: Parameter value in degrees
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameter's memory
 */
void ParameterMap::addRotationParam(const IComponent *comp, const std::string &name, const double deg,
                                    const std::string *const pDescription) {
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
  if (name == rotx()) {
    addDouble(comp, rotx(), deg);
    quat = Quat(deg, V3D(1, 0, 0)) * Quat(rotY, V3D(0, 1, 0)) * Quat(rotZ, V3D(0, 0, 1));
  } else if (name == roty()) {
    addDouble(comp, roty(), deg);
    quat = Quat(rotX, V3D(1, 0, 0)) * Quat(deg, V3D(0, 1, 0)) * Quat(rotZ, V3D(0, 0, 1));
  } else if (name == rotz()) {
    addDouble(comp, rotz(), deg);
    quat = Quat(rotX, V3D(1, 0, 0)) * Quat(rotY, V3D(0, 1, 0)) * Quat(deg, V3D(0, 0, 1));
  } else {
    g_log.warning() << "addRotationParam() called with unrecognized coordinate symbol: " << name;
    return;
  }

  // clear the position cache
  clearPositionSensitiveCaches();

  // finally add or update "pos" parameter
  addQuat(comp, rot(), quat, pDescription);
}

/**
 * Adds a double value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a string
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameter's memory
 * @param pVisible :: Whether the parameter should be visible in InstrumentViewer
 */
void ParameterMap::addDouble(const IComponent *comp, const std::string &name, const std::string &value,
                             const std::string *const pDescription, const std::string &pVisible) {
  add(pDouble(), comp, name, value, pDescription, pVisible);
}

/**
 * Adds a double value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a double
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameter's memory
 * @param pVisible :: Whether the parameter should be visible in InstrumentViewer
 */
void ParameterMap::addDouble(const IComponent *comp, const std::string &name, double value,
                             const std::string *const pDescription, const std::string &pVisible) {
  add(pDouble(), comp, name, value, pDescription, pVisible);
}

/**
 * Adds an int value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a string
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameters memory
 * @param pVisible :: Whether the parameter should be visible in InstrumentViewer
 */
void ParameterMap::addInt(const IComponent *comp, const std::string &name, const std::string &value,
                          const std::string *const pDescription, const std::string &pVisible) {
  add(pInt(), comp, name, value, pDescription, pVisible);
}

/**
 * Adds an int value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as an int
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameters memory
 * @param pVisible :: Whether the parameter should be visible in InstrumentViewer
 */
void ParameterMap::addInt(const IComponent *comp, const std::string &name, int value,
                          const std::string *const pDescription, const std::string &pVisible) {
  add(pInt(), comp, name, value, pDescription, pVisible);
}

/**
 * Adds a bool value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a string
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameters memory
 * @param pVisible :: Whether the parameter should be visible in InstrumentViewer
 */
void ParameterMap::addBool(const IComponent *comp, const std::string &name, const std::string &value,
                           const std::string *const pDescription, const std::string &pVisible) {
  add(pBool(), comp, name, value, pDescription, pVisible);
}
/**
 * Adds a bool value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a bool
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameter's memory
 * @param pVisible :: Whether the parameter should be visible in InstrumentViewer
 */
void ParameterMap::addBool(const IComponent *comp, const std::string &name, bool value,
                           const std::string *const pDescription, const std::string &pVisible) {
  add(pBool(), comp, name, value, pDescription, pVisible);
}

/** Force adding masking information. ONLY FOR INTERNAL USE by class Instrument.
 *
 * ParameterMap usually rejects "legacy style" masking information since it is
 * now stored in DetectorInfo. However, for the purpose of writing files class
 * Instrument needs to insert masking information. This method is only for
 * internal use by class Instrument. ParameterMaps modified by this method are
 * only for use as a temporary. */
void ParameterMap::forceUnsafeSetMasked(const IComponent *comp, bool value) {
  const std::string name("masked");
  auto param = create(pBool(), name);
  auto typedParam = std::dynamic_pointer_cast<ParameterType<bool>>(param);
  typedParam->setValue(value);

// When using Clang & Linux, TBB 4.4 doesn't detect C++11 features.
// https://software.intel.com/en-us/forums/intel-threading-building-blocks/topic/641658
#if defined(__clang__) && !defined(__APPLE__)
#define CLANG_ON_LINUX true
#else
#define CLANG_ON_LINUX false
#endif
#if TBB_VERSION_MAJOR >= 4 && TBB_VERSION_MINOR >= 4 && !CLANG_ON_LINUX
  m_map.emplace(comp->getComponentID(), param);
#else
  m_map.insert(std::make_pair(comp->getComponentID(), param));
#endif
}

/**
 * Adds a std::string value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameter's memory
 * @param pVisible :: Whether the parameter should be visible in InstrumentViewer
 */
void ParameterMap::addString(const IComponent *comp, const std::string &name, const std::string &value,
                             const std::string *const pDescription, const std::string &pVisible) {
  add<std::string>(pString(), comp, name, value, pDescription, pVisible);
}

/**
 * Adds a V3D value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a string
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameter's memory
 */
void ParameterMap::addV3D(const IComponent *comp, const std::string &name, const std::string &value,
                          const std::string *const pDescription) {
  add(pV3D(), comp, name, value, pDescription);
  clearPositionSensitiveCaches();
}

/**
 * Adds a V3D value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a V3D
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameter's memory
 */
void ParameterMap::addV3D(const IComponent *comp, const std::string &name, const V3D &value,
                          const std::string *const pDescription) {
  add(pV3D(), comp, name, value, pDescription);
  clearPositionSensitiveCaches();
}

/**
 * Adds a Quat value to the parameter map.
 * @param comp :: Component to which the new parameter is related
 * @param name :: Name for the new parameter
 * @param value :: Parameter value as a Quat
 * @param pDescription :: a pointer (may be NULL) to a string, containing parameter's description. If provided, the
 * contents of the string is copied to the parameter's memory
 */
void ParameterMap::addQuat(const IComponent *comp, const std::string &name, const Quat &value,
                           const std::string *const pDescription) {
  add(pQuat(), comp, name, value, pDescription);
  clearPositionSensitiveCaches();
}

/**
 * Does the named parameter exist for the given component and given type
 * @param comp :: The component to be searched
 * @param name :: The name of the parameter
 * @param type :: The type of the component as a string
 * @returns A boolean indicating if the map contains the named parameter. If the type is given then this must also match
 */
bool ParameterMap::contains(const IComponent *comp, const std::string &name, const std::string &type) const {
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
bool ParameterMap::contains(const IComponent *comp, const char *name, const char *type) const {
  checkIsNotMaskingParameter(name);
  if (m_map.empty())
    return false;
  const ComponentID id = comp->getComponentID();
  std::pair<pmap_cit, pmap_cit> components = m_map.equal_range(id);
  bool anytype = (strlen(type) == 0);
  for (auto itr = components.first; itr != components.second; ++itr) {
    const auto &param = itr->second;
    if (strcasecmp(param->nameAsCString(), name) == 0 && (anytype || param->type() == type)) {
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
bool ParameterMap::contains(const IComponent *comp, const Parameter &parameter) const {
  checkIsNotMaskingParameter(parameter.name());
  if (m_map.empty() || !comp)
    return false;

  const ComponentID id = comp->getComponentID();
  auto it_found = m_map.find(id);
  if (it_found != m_map.end()) {
    auto itrs = m_map.equal_range(id);
    for (auto itr = itrs.first; itr != itrs.second; ++itr) {
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
 * @returns The named parameter of the given type if it exists or a NULL shared pointer if not
 */
Parameter_sptr ParameterMap::get(const IComponent *comp, const std::string &name, const std::string &type) const {
  return get(comp, name.c_str(), type.c_str());
}

/** Return a named parameter of a given type. Avoids allocating std::string
 * temporaries
 * @param comp :: Component to which parameter is related
 * @param name :: Parameter name
 * @param type :: An optional type string
 * @returns The named parameter of the given type if it exists or a NULL shared pointer if not
 */
std::shared_ptr<Parameter> ParameterMap::get(const IComponent *comp, const char *name, const char *type) const {
  checkIsNotMaskingParameter(name);
  Parameter_sptr result;
  if (!comp)
    return result;

  auto itr = positionOf(comp, name, type);
  if (itr != m_map.end())
    result = std::atomic_load(&itr->second);
  return result;
}

/** Return an iterator pointing to a named parameter of a given type.
 * @param comp :: Component to which parameter is related
 * @param name :: Parameter name
 * @param type :: An optional type string. If empty, any type is returned
 * @returns The iterator parameter of the given type if it exists or a NULL
 * shared pointer if not
 */
component_map_it ParameterMap::positionOf(const IComponent *comp, const char *name, const char *type) {
  auto result = m_map.end();
  if (!comp)
    return result;
  const bool anytype = (strlen(type) == 0);
  if (!m_map.empty()) {
    const ComponentID id = comp->getComponentID();
    auto it_found = m_map.find(id);
    if (it_found != m_map.end()) {
      auto itrs = m_map.equal_range(id);
      for (auto itr = itrs.first; itr != itrs.second; ++itr) {
        const auto &param = itr->second;
        if (strcasecmp(param->nameAsCString(), name) == 0 && (anytype || param->type() == type)) {
          result = itr;
          break;
        }
      }
    }
  }
  return result;
}

/** Return a const iterator pointing to a named parameter of a given type.
 * @param comp :: Component to which parameter is related
 * @param name :: Parameter name
 * @param type :: An optional type string. If empty, any type is returned
 * @returns The iterator parameter of the given type if it exists or a NULL
 * shared pointer if not
 */
component_map_cit ParameterMap::positionOf(const IComponent *comp, const char *name, const char *type) const {
  auto result = m_map.end();
  if (!comp)
    return result;
  const bool anytype = (strlen(type) == 0);
  if (!m_map.empty()) {
    const ComponentID id = comp->getComponentID();
    auto it_found = m_map.find(id);
    if (it_found != m_map.end()) {
      auto itrs = m_map.equal_range(id);
      for (auto itr = itrs.first; itr != itrs.second; ++itr) {
        const auto &param = itr->second;
        if (strcasecmp(param->nameAsCString(), name) == 0 && (anytype || param->type() == type)) {
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
Parameter_sptr ParameterMap::getByType(const IComponent *comp, const std::string &type) const {
  Parameter_sptr result;
  if (!m_map.empty()) {
    const ComponentID id = comp->getComponentID();
    auto it_found = m_map.find(id);
    if (it_found != m_map.end() && it_found->first) {
      auto itrs = m_map.equal_range(id);
      for (auto itr = itrs.first; itr != itrs.second; ++itr) {
        const auto &param = itr->second;
        if (strcasecmp(param->type().c_str(), type.c_str()) == 0) {
          result = std::atomic_load(&param);
          break;
        }
      } // found->firdst
    } // it_found != m_map.end()
  } //! m_map.empty()
  return result;
}

/** Looks recursively upwards in the component tree for the first instance of a
 * component with a matching type.
 * @param comp :: The component to start the search with
 * @param type :: Parameter type
 * @returns the first matching parameter.
 */
Parameter_sptr ParameterMap::getRecursiveByType(const IComponent *comp, const std::string &type) const {
  std::shared_ptr<const IComponent> compInFocus(comp, NoDeleting());
  while (compInFocus != nullptr) {
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
Parameter_sptr ParameterMap::getRecursive(const IComponent *comp, const std::string &name,
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
Parameter_sptr ParameterMap::getRecursive(const IComponent *comp, const char *name, const char *type) const {
  checkIsNotMaskingParameter(name);
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
std::string ParameterMap::getString(const IComponent *comp, const std::string &name, bool recursive) const {
  Parameter_sptr param;
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
  auto it_found = m_map.find(id);
  if (it_found == m_map.end()) {
    return paramNames;
  }

  auto itrs = m_map.equal_range(id);
  for (auto it = itrs.first; it != itrs.second; ++it) {
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
  for (const auto &mappair : m_map) {
    const std::shared_ptr<Parameter> &p = mappair.second;
    if (p && mappair.first) {
      const auto *comp = dynamic_cast<const IComponent *>(mappair.first);
      const auto *det = dynamic_cast<const IDetector *>(comp);
      if (det) {
        out << "detID:" << det->getID();
      } else if (comp) {
        out << comp->getFullName(); // Use full path name to ensure unambiguous
                                    // naming
      }
      const auto paramVisible = "visible:" + std::string(p->visible() == 1 ? "true" : "false");
      out << ';' << p->type() << ';' << p->name() << ';' << p->asString() << ';' << paramVisible << '|';
    }
  }
  return out.str();
}

/**
 * Clears the location, rotation & bounding box caches
 */
void ParameterMap::clearPositionSensitiveCaches() {
  m_cacheLocMap->clear();
  m_cacheRotMap->clear();
}

/// Sets a cached location on the location cache
/// @param comp :: The Component to set the location of
/// @param location :: The location
void ParameterMap::setCachedLocation(const IComponent *comp, const V3D &location) const {
  m_cacheLocMap->setCache(comp->getComponentID(), location);
}

/// Attempts to retrieve a location from the location cache
/// @param comp :: The Component to find the location of
/// @param location :: If the location is found it's value will be set here
/// @returns true if the location is in the map, otherwise false
bool ParameterMap::getCachedLocation(const IComponent *comp, V3D &location) const {
  return m_cacheLocMap->getCache(comp->getComponentID(), location);
}

/// Sets a cached rotation on the rotation cache
/// @param comp :: The Component to set the rotation of
/// @param rotation :: The rotation as a quaternion
void ParameterMap::setCachedRotation(const IComponent *comp, const Quat &rotation) const {
  m_cacheRotMap->setCache(comp->getComponentID(), rotation);
}

/// Attempts to retrieve a rotation from the rotation cache
/// @param comp :: The Component to find the rotation of
/// @param rotation :: If the rotation is found it's value will be set here
/// @returns true if the rotation is in the map, otherwise false
bool ParameterMap::getCachedRotation(const IComponent *comp, Quat &rotation) const {
  return m_cacheRotMap->getCache(comp->getComponentID(), rotation);
}

/**
 * Copy pairs (oldComp->id,Parameter) to the m_map
 * assigning the new newComp->id
 * @param oldComp :: Old component
 * @param newComp :: New component
 * @param oldPMap :: Old map corresponding to the Old component
 */
void ParameterMap::copyFromParameterMap(const IComponent *oldComp, const IComponent *newComp,
                                        const ParameterMap *oldPMap) {

  auto oldParameterNames = oldPMap->names(oldComp);
  for (const auto &oldParameterName : oldParameterNames) {
    Parameter_sptr thisParameter = oldPMap->get(oldComp, oldParameterName);
// Insert the fetched parameter in the m_map
#if TBB_VERSION_MAJOR >= 4 && TBB_VERSION_MINOR >= 4 && !CLANG_ON_LINUX
    m_map.emplace(newComp->getComponentID(), std::move(thisParameter));
#else
    m_map.insert(std::make_pair(newComp->getComponentID(), std::move(thisParameter)));
#endif
  }
}

//--------------------------------------------------------------------------------------------
/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to create
 */
void ParameterMap::saveNexus(::NeXus::File *file, const std::string &group) const {
  file->makeGroup(group, "NXnote", true);
  file->putAttr("version", 1);
  file->writeData("author", "");
  file->writeData("date", Types::Core::DateAndTime::getCurrentTime().toISO8601String());
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
const std::vector<std::string> &ParameterMap::getParameterFilenames() const { return m_parameterFileNames; }
///
/** adds a parameter filename that has been loaded
 * @param filename the filename to add
 */
void ParameterMap::addParameterFilename(const std::string &filename) { m_parameterFileNames.emplace_back(filename); }

/// Wrapper for ParameterFactory::create to avoid include in header
std::shared_ptr<Parameter> ParameterMap::create(const std::string &className, const std::string &name,
                                                const std::string &visible) const {
  return ParameterFactory::create(className, name, visible);
}

/** Only for use by ExperimentInfo. Returns returns true if this instrument
 contains a DetectorInfo.

 The `instrument` argument is needed for the special case of having a neutronic
 *and* a physical instrument. `Instrument` uses the same parameter map for both,
 but the DetectorInfo is only for the neutronic instrument. */
bool ParameterMap::hasDetectorInfo(const Instrument *instrument) const {
  if (instrument != m_instrument)
    return false;
  return static_cast<bool>(m_detectorInfo);
}

/** Only for use by ExperimentInfo. Returns returns true if this instrument
 contains a ComponentInfo.
*/
bool ParameterMap::hasComponentInfo(const Instrument *instrument) const {
  if (instrument != m_instrument)
    return false;
  return static_cast<bool>(m_componentInfo);
}

/// Only for use by ExperimentInfo. Returns a reference to the DetectorInfo.
const Geometry::DetectorInfo &ParameterMap::detectorInfo() const {
  if (!hasDetectorInfo(m_instrument))
    throw std::runtime_error("Cannot return reference to NULL DetectorInfo");
  return *m_detectorInfo;
}

/// Only for use by ExperimentInfo. Returns a reference to the DetectorInfo.
Geometry::DetectorInfo &ParameterMap::mutableDetectorInfo() {
  if (!hasDetectorInfo(m_instrument))
    throw std::runtime_error("Cannot return reference to NULL DetectorInfo");
  return *m_detectorInfo;
}

/// Only for use by ExperimentInfo. Returns a reference to the ComponentInfo.
const Geometry::ComponentInfo &ParameterMap::componentInfo() const {
  if (!hasComponentInfo(m_instrument)) {
    throw std::runtime_error("Cannot return reference to NULL ComponentInfo");
  }
  return *m_componentInfo;
}

/// Only for use by ExperimentInfo. Returns a reference to the ComponentInfo.
Geometry::ComponentInfo &ParameterMap::mutableComponentInfo() {
  if (!hasComponentInfo(m_instrument)) {
    throw std::runtime_error("Cannot return reference to NULL ComponentInfo");
  }
  return *m_componentInfo;
}

/// Only for use by Detector. Returns a detector index for a detector ID.
size_t ParameterMap::detectorIndex(const detid_t detID) const { return m_instrument->detectorIndex(detID); }

size_t ParameterMap::componentIndex(const ComponentID componentId) const {
  return m_componentInfo->indexOf(componentId);
}

/// Only for use by Instrument. Sets the pointer to the owning instrument.
void ParameterMap::setInstrument(const Instrument *instrument) {
  if (instrument == m_instrument)
    return;
  if (!instrument) {
    m_componentInfo = nullptr;
    m_detectorInfo = nullptr;
    return;
  }
  if (m_instrument)
    throw std::logic_error("ParameterMap::setInstrument: Cannot change "
                           "instrument once it has been set.");
  if (instrument->isParametrized())
    throw std::logic_error("ParameterMap::setInstrument must be called with "
                           "base instrument, not a parametrized instrument");
  m_instrument = instrument;
  std::tie(m_componentInfo, m_detectorInfo) = m_instrument->makeBeamline(*this);
}

} // namespace Mantid::Geometry
