// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidKernel/CaseInsensitiveMap.h"

#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace Mantid {
namespace Geometry {

class ComponentInfo;

/** ParameterInfo : the named parameters of an instrument, addressed by component index.
 *
 * This is the Instrument 2.0 replacement for ParameterMap's storage.
 *
 * It uses the same index keys as ComponentInfo.
 *
 * Recursive (parent-inheriting) lookups take a ComponentInfo reference and walk
 * ComponentInfo::parent(), because the hierarchy is not stored here.
 */
class MANTID_GEOMETRY_DLL ParameterInfo {
public:
  /** Parameters of a single component, ordered by name.
   *
   * A multimap because two parameters on one component may share a short name: fitting
   * parameters are deduplicated by (name, function), so e.g. IkedaCarpenterPV:Alpha0 and
   * IkedaCarpenterMD:Alpha0 coexist.
   *
   * The comparator is case-insensitive to match the legacy lookup semantics, which compare
   * parameter names with strcasecmp throughout.
   */
  using ComponentParameters =
      std::multimap<std::string, std::shared_ptr<Parameter>, Kernel::CaseInsensitiveStringComparator>;

  /// Add a parameter, replacing any existing parameter of the same name on the same component.
  /// Matches the legacy ParameterMap::add() add-or-replace behaviour.
  void add(size_t const componentIndex, std::shared_ptr<Parameter> const &parameter);

  /// Add a fitting parameter, deduplicating by (name, function) rather than by name alone so
  /// that two functions on one component may each declare a parameter of the same short name.
  void addFittingParameter(size_t const componentIndex, std::shared_ptr<Parameter> const &parameter,
                           std::string const &fittingFunction);

  /// Insert a parameter without add()'s add-or-replace deduplication.
  /// For rekeying an existing ParameterMap, whose contents have already had the legacy
  /// deduplication rules applied: re-running them here would collapse the same-named fitting
  /// parameters that addFittingParameter() deliberately keeps distinct. Prefer add() elsewhere.
  void insert(size_t const componentIndex, std::shared_ptr<Parameter> const &parameter);

  /// Does a parameter of this name (and optionally this type) exist on this component?
  bool contains(size_t const componentIndex, std::string const &name, std::string const &type = "") const;

  /// The named parameter on this component, or a null shared pointer if there is none.
  std::shared_ptr<Parameter> get(size_t const componentIndex, std::string const &name,
                                 std::string const &type = "") const;

  /// The first parameter of this type on this component, or a null shared pointer.
  std::shared_ptr<Parameter> getByType(size_t const componentIndex, std::string const &type) const;

  /// As get(), but walking up the component hierarchy until a match is found.
  std::shared_ptr<Parameter> getRecursive(ComponentInfo const &componentInfo, size_t const componentIndex,
                                          std::string const &name, std::string const &type = "") const;

  /// As getByType(), but walking up the component hierarchy until a match is found.
  std::shared_ptr<Parameter> getRecursiveByType(ComponentInfo const &componentInfo, size_t const componentIndex,
                                                std::string const &type) const;

  /// Find a fitting parameter by (short name, function) walking up the component hierarchy.
  /// Examines every entry on each level.
  std::shared_ptr<Parameter> getRecursiveFittingParameter(ComponentInfo const &componentInfo,
                                                          size_t const componentIndex, std::string const &name,
                                                          std::string const &fittingFunction) const;

  /// The names of the parameters on this component. Names differing only in case collapse to
  /// one entry, consistent with add()'s case-insensitive deduplication.
  std::set<std::string> names(size_t const componentIndex) const;

  /// All parameters on one component, non-recursive, ordered by name.
  /// Returns a reference to a static empty container when the component has no parameters.
  ComponentParameters const &parameters(size_t const componentIndex) const;

  /// Remove every parameter of this name from every component.
  void clearParametersByName(std::string const &name);

  /// Remove every parameter of this name from one component.
  void clearParametersByName(size_t const componentIndex, std::string const &name);

  /// Remove all parameters.
  void clear();

  bool empty() const;

  /// The total number of parameters across all components.
  size_t size() const;

  size_t getMemorySize() const;

  /// Iteration over (component index, parameters) pairs, in ascending component-index order.
  /// The ordering is relied upon to make serialization deterministic.
  auto begin() const { return m_store.begin(); }
  auto end() const { return m_store.end(); }

private:
  std::map<size_t, ComponentParameters> m_store;
};

} // namespace Geometry
} // namespace Mantid
