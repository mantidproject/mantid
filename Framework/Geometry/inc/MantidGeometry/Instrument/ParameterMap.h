// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDTypes.h" //For specnum_t
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Parameter.h"

#include "tbb/concurrent_unordered_map.h"

#include <memory>
#include <typeinfo>
#include <vector>

namespace Mantid {
namespace Kernel {
template <class KEYTYPE, class VALUETYPE> class Cache;
}
namespace Geometry {
class ComponentInfo;
class DetectorInfo;
class Instrument;

/** @class ParameterMap ParameterMap.h

  ParameterMap class. Holds the parameters of modified (parametrized) instrument
  components. ParameterMap has a number of 'add' methods for adding parameters
  of different types.

  @author Roman Tolchenov, Tessella Support Services plc
  @date 2/12/2008
*/
/// Parameter map iterator typedef
using component_map_it = tbb::concurrent_unordered_multimap<ComponentID, std::shared_ptr<Parameter>>::iterator;
using component_map_cit = tbb::concurrent_unordered_multimap<ComponentID, std::shared_ptr<Parameter>>::const_iterator;

class MANTID_GEOMETRY_DLL ParameterMap {
public:
  /// Parameter map typedef
  using pmap = tbb::concurrent_unordered_multimap<ComponentID, std::shared_ptr<Parameter>>;
  /// Parameter map iterator typedef
  using pmap_it = tbb::concurrent_unordered_multimap<ComponentID, std::shared_ptr<Parameter>>::iterator;
  /// Parameter map iterator typedef
  using pmap_cit = tbb::concurrent_unordered_multimap<ComponentID, std::shared_ptr<Parameter>>::const_iterator;
  /// Default constructor
  ParameterMap();
  /// Const constructor
  ParameterMap(const ParameterMap &other);
  ~ParameterMap();
  /// Returns true if the map is empty, false otherwise
  inline bool empty() const { return m_map.empty(); }
  /// Return the size of the map
  inline int size() const { return static_cast<int>(m_map.size()); }
  /// Return string to be used in the map
  static const std::string &pos();
  static const std::string &posx();
  static const std::string &posy();
  static const std::string &posz();
  static const std::string &rot();
  static const std::string &rotx();
  static const std::string &roty();
  static const std::string &rotz();
  static const std::string &pDouble(); // p prefix to avoid name clash
  static const std::string &pInt();
  static const std::string &pBool();
  static const std::string &pString();
  static const std::string &pV3D();
  static const std::string &pQuat();
  static const std::string &scale();

  const std::string diff(const ParameterMap &rhs, const bool &firstDiffOnly = false, const bool relative = false,
                         const double doubleTolerance = Kernel::Tolerance) const;

  /// Inquality comparison operator
  bool operator!=(const ParameterMap &rhs) const;
  /// Equality comparison operator
  bool operator==(const ParameterMap &rhs) const;

  /// Clears the map
  inline void clear() {
    m_map.clear();
    clearPositionSensitiveCaches();
  }
  /// method swaps two parameter maps contents  each other. All caches contents
  /// is nullified (TO DO: it can be efficiently swapped too)
  void swap(ParameterMap &other) {
    m_map.swap(other.m_map);
    clearPositionSensitiveCaches();
  }
  /// Clear any parameters with the given name
  void clearParametersByName(const std::string &name);

  /// Clear any parameters with the given name for a specified component
  void clearParametersByName(const std::string &name, const IComponent *comp);

  /// Method for adding a parameter providing its value as a string
  void add(const std::string &type, const IComponent *comp, const std::string &name, const std::string &value,
           const std::string *const pDescription = nullptr, const std::string &visible = "true");

  /**
   * Method for adding a parameter providing its value of a particular type.
   * If a parameter already exists then it is replaced with a new one of the
   * given type and value
   * @tparam T The concrete type
   * @param type :: A string denoting the type, e.g. double, string, fitting
   * @param comp :: A pointer to the component that this parameter is attached to
   * @param name :: The name of the parameter
   * @param value :: The parameter's value
   * @param pDescription :: if present, the constant pointer to a constant
   * string, containing parameter's description.
   * @param pVisible :: if present, defines whether the parameter should be visible
   * in InstrumentViewer
   */
  template <class T>
  void add(const std::string &type, const IComponent *comp, const std::string &name, const T &value,
           const std::string *const pDescription = nullptr, const std::string &pVisible = "true") {
    auto param = create(type, name, pVisible);
    auto typedParam = std::dynamic_pointer_cast<ParameterType<T>>(param);
    assert(typedParam); // If not true the factory has created the wrong type
    typedParam->setValue(value);
    this->add(comp, param, pDescription);
  }
  /// Method for adding a parameter providing shared pointer to it. The class
  /// stores share pointer and increment ref count to it
  void add(const IComponent *comp, const std::shared_ptr<Parameter> &par,
           const std::string *const pDescription = nullptr);

  /** @name Helper methods for adding and updating parameter types  */
  /// Create or adjust "pos" parameter for a component
  void addPositionCoordinate(const IComponent *comp, const std::string &name, const double value,
                             const std::string *const pDescription = nullptr);
  /// Create or adjust "rot" parameter for a component
  void addRotationParam(const IComponent *comp, const std::string &name, const double deg,
                        const std::string *const pDescription = nullptr);
  /// Adds a double value to the parameter map.
  void addDouble(const IComponent *comp, const std::string &name, const std::string &value,
                 const std::string *const pDescription = nullptr, const std::string &pVisible = "true");
  /// Adds a double value to the parameter map.
  void addDouble(const IComponent *comp, const std::string &name, double value,
                 const std::string *const pDescription = nullptr, const std::string &pVisible = "true");
  /// Adds an int value to the parameter map.
  void addInt(const IComponent *comp, const std::string &name, const std::string &value,
              const std::string *const pDescription = nullptr, const std::string &pVisible = "true");
  /// Adds an int value to the parameter map.
  void addInt(const IComponent *comp, const std::string &name, int value,
              const std::string *const pDescription = nullptr, const std::string &pVisible = "true");
  /// Adds a bool value to the parameter map.
  void addBool(const IComponent *comp, const std::string &name, const std::string &value,
               const std::string *const pDescription = nullptr, const std::string &pVisible = "true");
  /// Adds a bool value to the parameter map.
  void addBool(const IComponent *comp, const std::string &name, bool value,
               const std::string *const pDescription = nullptr, const std::string &pVisible = "true");
  /// Adds a std::string value to the parameter map.
  void addString(const IComponent *comp, const std::string &name, const std::string &value,
                 const std::string *const pDescription = nullptr, const std::string &pVisible = "true");
  /// Adds a Kernel::V3D value to the parameter map.
  void addV3D(const IComponent *comp, const std::string &name, const std::string &value,
              const std::string *const pDescription = nullptr);
  /// Adds a Kernel::V3D value to the parameter map.
  void addV3D(const IComponent *comp, const std::string &name, const Kernel::V3D &value,
              const std::string *const pDescription = nullptr);
  /// Adds a Kernel::Quat value to the parameter map.
  void addQuat(const IComponent *comp, const std::string &name, const Kernel::Quat &value,
               const std::string *const pDescription = nullptr);
  void forceUnsafeSetMasked(const IComponent *comp, bool value);
  //@}

  /// Does the named parameter exist for the given component and type
  /// (std::string version)
  bool contains(const IComponent *comp, const std::string &name, const std::string &type = "") const;
  /// Does the named parameter exist for the given component (c-string version)
  bool contains(const IComponent *comp, const char *name, const char *type = "") const;
  /// Does the given parameter & component combination exist
  bool contains(const IComponent *comp, const Parameter &parameter) const;
  /// Get a parameter with a given name and type (std::string version)
  std::shared_ptr<Parameter> get(const IComponent *comp, const std::string &name, const std::string &type = "") const;
  /// Get a parameter with a given name and type (c-string version)
  std::shared_ptr<Parameter> get(const IComponent *comp, const char *name, const char *type = "") const;
  /// Finds the parameter in the map via the parameter type.
  std::shared_ptr<Parameter> getByType(const IComponent *comp, const std::string &type) const;
  /// Use get() recursively to see if can find param in all parents of comp and
  /// given type (std::string version)
  std::shared_ptr<Parameter> getRecursive(const IComponent *comp, const std::string &name,
                                          const std::string &type = "") const;
  /// Use get() recursively to see if can find param in all parents of comp and
  /// given type (const char type)
  std::shared_ptr<Parameter> getRecursive(const IComponent *comp, const char *name, const char *type = "") const;
  /// Looks recursively upwards in the component tree for the first instance of
  /// a parameter with a specified type.
  std::shared_ptr<Parameter> getRecursiveByType(const IComponent *comp, const std::string &type) const;

  /** Get the values of a given parameter of all the components that have the
   * name: compName
   *  @tparam The parameter type
   *  @param compName :: The name of the component
   *  @param name :: The name of the parameter
   *  @return all component values from the given component name
   */
  template <class T> std::vector<T> getType(const std::string &compName, const std::string &name) const {
    std::vector<T> retval;

    pmap_cit it;
    for (it = m_map.begin(); it != m_map.end(); ++it) {
      if (compName == it->first->getName()) {
        std::shared_ptr<Parameter> param = get(it->first, name);
        if (param)
          retval.emplace_back(param->value<T>());
      }
    }
    return retval;
  }
  /** Get the component description by name */
  const std::string getDescription(const std::string &compName, const std::string &name) const;
  /** Get the component tooltip by name */
  const std::string getShortDescription(const std::string &compName, const std::string &name) const;

  /// Return the value of a parameter as a string
  std::string getString(const IComponent *comp, const std::string &name, bool recursive = false) const;
  /// Returns a string parameter as vector's first element if exists and an
  /// empty vector if it doesn't
  std::vector<std::string> getString(const std::string &compName, const std::string &name) const {
    return getType<std::string>(compName, name);
  }
  /**
   * Returns a double parameter as vector's first element if exists and an empty
   * vector if it doesn't
   * @param compName :: Component name
   * @param name :: Parameter name
   * @return a double parameter from component with the requested name
   */
  std::vector<double> getDouble(const std::string &compName, const std::string &name) const {
    return getType<double>(compName, name);
  }
  /**
   * Returns a Kernel::V3D parameter as vector's first element if exists and an
   * empty vector if it doesn't
   * @param compName :: Component name
   * @param name :: Parameter name
   * @return a Kernel::V3D parameter from component with the requested name
   */
  std::vector<Kernel::V3D> getV3D(const std::string &compName, const std::string &name) const {
    return getType<Kernel::V3D>(compName, name);
  }

  /// Returns a set with all parameter names for component
  std::set<std::string> names(const IComponent *comp) const;
  /// Returns a string with all component names, parameter names and values
  std::string asString() const;

  /// Clears the location, rotation & bounding box caches
  void clearPositionSensitiveCaches();
  /// Sets a cached location on the location cache
  void setCachedLocation(const IComponent *comp, const Kernel::V3D &location) const;
  /// Attempts to retrieve a location from the location cache
  bool getCachedLocation(const IComponent *comp, Kernel::V3D &location) const;
  /// Sets a cached rotation on the rotation cache
  void setCachedRotation(const IComponent *comp, const Kernel::Quat &rotation) const;
  /// Attempts to retrieve a rotation from the rotation cache
  bool getCachedRotation(const IComponent *comp, Kernel::Quat &rotation) const;
  /// Persist a representation of the Parameter map to the open Nexus file
  void saveNexus(Nexus::File *file, const std::string &group) const;
  /// Copy pairs (oldComp->id,Parameter) to the m_map assigning the new
  /// newComp->id
  void copyFromParameterMap(const IComponent *oldComp, const IComponent *newComp, const ParameterMap *oldPMap);

  /// Returns a list of all the parameter files loaded
  const std::vector<std::string> &getParameterFilenames() const;
  /// adds a parameter filename that has been loaded
  void addParameterFilename(const std::string &filename);

  /// access iterators. begin;
  pmap_it begin() { return m_map.begin(); }
  pmap_cit begin() const { return m_map.begin(); }
  /// access iterators. end;
  pmap_it end() { return m_map.end(); }
  pmap_cit end() const { return m_map.end(); }

  bool hasDetectorInfo(const Instrument *instrument) const;
  bool hasComponentInfo(const Instrument *instrument) const;
  const Geometry::DetectorInfo &detectorInfo() const;
  Geometry::DetectorInfo &mutableDetectorInfo();
  const Geometry::ComponentInfo &componentInfo() const;
  Geometry::ComponentInfo &mutableComponentInfo();
  size_t detectorIndex(const detid_t detID) const;
  size_t componentIndex(const Geometry::ComponentID componentId) const;
  const std::vector<Geometry::ComponentID> &componentIds() const;
  void setInstrument(const Instrument *instrument);

private:
  std::shared_ptr<Parameter> create(const std::string &className, const std::string &name,
                                    const std::string &visible = "true") const;

  /// Assignment operator
  ParameterMap &operator=(ParameterMap *rhs);
  /// internal function to get position of the parameter in the parameter map
  component_map_it positionOf(const IComponent *comp, const char *name, const char *type);
  /// const version of the internal function to get position of the parameter in
  /// the parameter map
  component_map_cit positionOf(const IComponent *comp, const char *name, const char *type) const;
  /// calculate relative error for use in diff
  bool relErr(double x1, double x2, double errorVal) const;

  /// internal list of parameter files loaded
  std::vector<std::string> m_parameterFileNames;

  /// internal parameter map instance
  pmap m_map;
  /// internal cache map instance for cached position values
  std::unique_ptr<Kernel::Cache<const ComponentID, Kernel::V3D>> m_cacheLocMap;
  /// internal cache map instance for cached rotation values
  std::unique_ptr<Kernel::Cache<const ComponentID, Kernel::Quat>> m_cacheRotMap;

  /// Pointer to the DetectorInfo wrapper. NULL unless the instrument is
  /// associated with an ExperimentInfo object.
  std::unique_ptr<Geometry::DetectorInfo> m_detectorInfo;

  /// Pointer to the ComponentInfo wrapper. NULL unless the instrument is
  /// associated with an ExperimentInfo object.
  std::unique_ptr<Geometry::ComponentInfo> m_componentInfo;

  /// Pointer to the owning instrument for translating detector IDs into
  /// detector indices when accessing the DetectorInfo object. If the workspace
  /// distinguishes between a neutronic instrument and a physical instrument
  /// the owning instrument is the neutronic one.
  const Instrument *m_instrument{nullptr};
};

/// ParameterMap shared pointer typedef
using ParameterMap_sptr = std::shared_ptr<ParameterMap>;
/// ParameterMap constant shared pointer typedef
using ParameterMap_const_sptr = std::shared_ptr<const ParameterMap>;

} // Namespace Geometry

} // Namespace Mantid
