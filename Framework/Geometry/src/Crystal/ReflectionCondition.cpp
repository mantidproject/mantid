#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidKernel/System.h"
#include <algorithm>
#include <iterator>

namespace Mantid {
namespace Geometry {

/** @return a vector with all possible ReflectionCondition objects */
std::vector<ReflectionCondition_sptr> getAllReflectionConditions() {
  std::vector<ReflectionCondition_sptr> out;
  out.push_back(ReflectionCondition_sptr(new ReflectionConditionPrimitive()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionCFaceCentred()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionAFaceCentred()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionBFaceCentred()));
  out.push_back(ReflectionCondition_sptr(new ReflectionConditionBodyCentred()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionAllFaceCentred()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionRhombohedrallyObverse()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionRhombohedrallyReverse()));
  out.push_back(
      ReflectionCondition_sptr(new ReflectionConditionHexagonallyReverse()));
  return out;
}

/// Helper function that transforms all ReflectionConditions to strings.
std::vector<std::string> transformReflectionConditions(
    const std::function<std::string(const ReflectionCondition_sptr &)> &fn) {
  auto conditions = getAllReflectionConditions();

  std::vector<std::string> names;
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
  auto conditions = getAllReflectionConditions();

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
