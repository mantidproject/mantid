// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include <algorithm>
#include <iterator>

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {

// General template definition for RegisterConditions
template <typename... Args> struct RegisterConditions;

// Specialisation for recursive case
template <typename R, typename... Args> struct RegisterConditions<R, Args...> {
  static void run(ReflectionConditions &container) {
    container.emplace_back(boost::make_shared<R>());
    RegisterConditions<Args...>::run(container);
  }
};

// Specialisation to end recursion
template <> struct RegisterConditions<> {
  static void run(ReflectionConditions &) {}
};

/** @return a vector with all possible ReflectionCondition objects */
const ReflectionConditions &getAllReflectionConditions() {
  static ReflectionConditions conditions;
  if (conditions.empty()) {
    RegisterConditions<
        ReflectionConditionPrimitive, ReflectionConditionCFaceCentred,
        ReflectionConditionAFaceCentred, ReflectionConditionBFaceCentred,
        ReflectionConditionBodyCentred, ReflectionConditionAllFaceCentred,
        ReflectionConditionRhombohedrallyObverse,
        ReflectionConditionRhombohedrallyReverse,
        ReflectionConditionHexagonallyReverse>::run(conditions);
  }
  return conditions;
}

/// Helper function that transforms all ReflectionConditions to strings.
std::vector<std::string> transformReflectionConditions(
    const std::function<std::string(const ReflectionCondition_sptr &)> &fn) {
  const auto &conditions = getAllReflectionConditions();

  std::vector<std::string> names;
  names.reserve(conditions.size());
  std::transform(conditions.cbegin(), conditions.cend(),
                 std::back_inserter(names), fn);

  return names;
}

/// Returns all ReflectionCondition names.
std::vector<std::string> getAllReflectionConditionNames() {
  return transformReflectionConditions(
      [](const ReflectionCondition_sptr &condition) {
        return condition->getName();
      });
}

/// Returns all centering symbols.
std::vector<std::string> getAllReflectionConditionSymbols() {
  return transformReflectionConditions(
      [](const ReflectionCondition_sptr &condition) {
        return condition->getSymbol();
      });
}

/**
 * @brief Returns a reflection condition according to a filter function
 *
 * This small helper function returns a ReflectionCondition_sptr for which
 * the supplied function returns true. If no ReflectionCondition is found,
 * an std::invalid_argument exception is thrown. The message of the exception
 * contains the hint-parameter, which could be string that was used as a
 * matching criterion to find the ReflectionCondition.
 *
 * @param fn :: Unary predicate for matching ReflectionCondition
 * @param hint :: Hint to include in exception message. Name or symbol.
 * @return ReflectionCondition for which fn matches.
 */
ReflectionCondition_sptr getReflectionConditionWhere(
    const std::function<bool(const ReflectionCondition_sptr &)> &fn,
    const std::string &hint) {
  const auto &conditions = getAllReflectionConditions();

  auto it = std::find_if(conditions.cbegin(), conditions.cend(), fn);

  if (it == conditions.cend()) {
    throw std::invalid_argument("No ReflectionCondition found that matches '" +
                                hint + "'.");
  }

  return *it;
}

/// Returns the requested ReflectionCondition, see
/// getAllReflectionConditionNames for possible names.
ReflectionCondition_sptr getReflectionConditionByName(const std::string &name) {
  return getReflectionConditionWhere(
      [=](const ReflectionCondition_sptr &condition) {
        return condition->getName() == name;
      },
      name);
}

/// Returns the ReflectionCondition for the specified centering symbol, see
/// getAllReflectionConditionSymbols for possible symbols.
ReflectionCondition_sptr
getReflectionConditionBySymbol(const std::string &symbol) {
  return getReflectionConditionWhere(
      [=](const ReflectionCondition_sptr &condition) {
        return condition->getSymbol() == symbol;
      },
      symbol);
}

} // namespace Geometry
} // namespace Mantid
