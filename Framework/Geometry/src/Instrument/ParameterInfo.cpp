// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ParameterInfo.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/FitParameter.h"

#include <numeric>

namespace Mantid::Geometry {

namespace {
/// Shared empty result for parameters() on a component that carries none, so callers can take
/// a reference without a null check.
ParameterInfo::ComponentParameters const &emptyParameters() {
  static ParameterInfo::ComponentParameters const empty;
  return empty;
}

/** Type matching as the legacy ParameterMap performed it.
 *
 * Note the deliberate asymmetry with matchesType() below: ParameterMap::positionOf() and
 * contains() compare types case-sensitively with operator==, while getByType() compares them
 * case-insensitively with strcasecmp. Both behaviours are preserved rather than unified, since
 * unifying them would change lookup results for existing instrument definition files.
 */
bool matchesNamedType(Parameter const &parameter, std::string const &type) {
  return type.empty() || parameter.type() == type;
}

/// Case-insensitive type matching, as used by ParameterMap::getByType(). See matchesNamedType().
/// Expressed through the comparator rather than a direct strcasecmp call so that the Windows
/// (_stricmp) and POSIX (strcasecmp) spellings stay in one place.
bool matchesType(Parameter const &parameter, std::string const &type) {
  Kernel::CaseInsensitiveStringComparator const less;
  return !less(parameter.type(), type) && !less(type, parameter.type());
}
} // namespace

void ParameterInfo::add(size_t const componentIndex, std::shared_ptr<Parameter> const &parameter) {
  if (!parameter) {
    return;
  }
  auto &componentParameters = m_store[componentIndex];
  // Add-or-replace, matching legacy ParameterMap::add(). The name comparison is the
  // multimap's own case-insensitive comparator, so "Alpha" replaces an existing "alpha".
  auto const existing = componentParameters.find(parameter->name());
  if (existing != componentParameters.end()) {
    existing->second = parameter;
  } else {
    componentParameters.emplace(parameter->name(), parameter);
  }
}

void ParameterInfo::addFittingParameter(size_t const componentIndex, std::shared_ptr<Parameter> const &parameter,
                                        std::string const &fittingFunction) {
  if (!parameter) {
    return;
  }
  auto &componentParameters = m_store[componentIndex];
  // Deduplicate by (name, function) rather than by name alone, so that two functions on the
  // same component may each declare a parameter of the same short name.
  auto const range = componentParameters.equal_range(parameter->name());
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second->type() != "fitting") {
      continue;
    }
    try {
      if (it->second->value<FitParameter>().getFunction() == fittingFunction) {
        it->second = parameter;
        return;
      }
    } catch (...) {
      // Tagged "fitting" but the value is not a FitParameter; not a match, keep looking.
    }
  }
  componentParameters.emplace(parameter->name(), parameter);
}

void ParameterInfo::insert(size_t const componentIndex, std::shared_ptr<Parameter> const &parameter) {
  if (!parameter) {
    return;
  }
  m_store[componentIndex].emplace(parameter->name(), parameter);
}

bool ParameterInfo::contains(size_t const componentIndex, std::string const &name, std::string const &type) const {
  return static_cast<bool>(get(componentIndex, name, type));
}

std::shared_ptr<Parameter> ParameterInfo::get(size_t const componentIndex, std::string const &name,
                                              std::string const &type) const {
  std::shared_ptr<Parameter> result;
  auto const component = m_store.find(componentIndex);
  if (component != m_store.end()) {
    auto const range = component->second.equal_range(name);
    for (auto it = range.first; it != range.second; ++it) {
      if (matchesNamedType(*it->second, type)) {
        result = it->second;
        break;
      }
    }
  }
  return result;
}

std::shared_ptr<Parameter> ParameterInfo::getByType(size_t const componentIndex, std::string const &type) const {
  std::shared_ptr<Parameter> result;
  auto const component = m_store.find(componentIndex);
  if (component != m_store.end()) {
    for (auto const &[name, parameter] : component->second) {
      static_cast<void>(name);
      if (matchesType(*parameter, type)) {
        result = parameter;
        break;
      }
    }
  }
  return result;
}

std::shared_ptr<Parameter> ParameterInfo::getRecursive(ComponentInfo const &componentInfo, size_t const componentIndex,
                                                       std::string const &name, std::string const &type) const {
  std::shared_ptr<Parameter> result;
  for (size_t index = componentIndex;;) {
    result = get(index, name, type);
    if (result || !componentInfo.hasParent(index)) {
      break;
    }
    index = componentInfo.parent(index);
  }
  return result;
}

std::shared_ptr<Parameter> ParameterInfo::getRecursiveByType(ComponentInfo const &componentInfo,
                                                             size_t const componentIndex,
                                                             std::string const &type) const {
  std::shared_ptr<Parameter> result;
  for (size_t index = componentIndex;;) {
    result = getByType(index, type);
    if (result || !componentInfo.hasParent(index)) {
      break;
    }
    index = componentInfo.parent(index);
  }
  return result;
}

std::shared_ptr<Parameter> ParameterInfo::getRecursiveFittingParameter(ComponentInfo const &componentInfo,
                                                                       size_t const componentIndex,
                                                                       std::string const &name,
                                                                       std::string const &fittingFunction) const {
  std::shared_ptr<Parameter> result;
  for (size_t index = componentIndex;;) {
    auto const component = m_store.find(index);
    if (component != m_store.end()) {
      auto const range = component->second.equal_range(name);
      for (auto it = range.first; it != range.second; ++it) {
        if (it->second->type() != "fitting") {
          continue;
        }
        try {
          if (it->second->value<FitParameter>().getFunction() == fittingFunction) {
            result = it->second;
            break;
          }
        } catch (...) {
          // Tagged "fitting" but the value is not a FitParameter; keep looking.
        }
      }
    }
    if (result || !componentInfo.hasParent(index)) {
      break;
    }
    index = componentInfo.parent(index);
  }
  return result;
}

std::set<std::string> ParameterInfo::names(size_t const componentIndex) const {
  std::set<std::string> result;
  auto const component = m_store.find(componentIndex);
  if (component != m_store.end()) {
    for (auto const &[name, parameter] : component->second) {
      static_cast<void>(parameter);
      result.insert(name);
    }
  }
  return result;
}

ParameterInfo::ComponentParameters const &ParameterInfo::parameters(size_t const componentIndex) const {
  auto const component = m_store.find(componentIndex);
  return component == m_store.end() ? emptyParameters() : component->second;
}

void ParameterInfo::clearParametersByName(std::string const &name) {
  for (auto component = m_store.begin(); component != m_store.end();) {
    component->second.erase(name);
    // Drop components that no longer carry anything, so the store stays sparse.
    if (component->second.empty()) {
      component = m_store.erase(component);
    } else {
      ++component;
    }
  }
}

void ParameterInfo::clearParametersByName(size_t const componentIndex, std::string const &name) {
  auto const component = m_store.find(componentIndex);
  if (component != m_store.end()) {
    component->second.erase(name);
    if (component->second.empty()) {
      m_store.erase(component);
    }
  }
}

void ParameterInfo::clear() { m_store.clear(); }

bool ParameterInfo::empty() const { return m_store.empty(); }

size_t ParameterInfo::size() const {
  return std::accumulate(m_store.cbegin(), m_store.cend(), size_t{0},
                         [](size_t acc, auto const &component) { return acc + component.second.size(); });
}

size_t ParameterInfo::getMemorySize() const {
  size_t storeMem = 0;
  size_t parameterObjectsMem = 0;
  for (auto const &[componentIndex, componentParameters] : m_store) {
    static_cast<void>(componentIndex);
    // Outer std::map node: key + value + colour bit + 3 pointers.
    storeMem += sizeof(decltype(m_store)::value_type) + 4 * sizeof(void *);
    for (auto const &[name, parameter] : componentParameters) {
      // Inner std::multimap node: same shape.
      storeMem += sizeof(ComponentParameters::value_type) + 4 * sizeof(void *) + name.capacity();
      if (parameter) {
        parameterObjectsMem += sizeof(Parameter) + parameter->m_name.capacity() + parameter->m_type.capacity() +
                               parameter->m_str_value.capacity() + parameter->m_description.capacity();
      }
    }
  }
  return sizeof(*this) + storeMem + parameterObjectsMem;
}

} // namespace Mantid::Geometry
