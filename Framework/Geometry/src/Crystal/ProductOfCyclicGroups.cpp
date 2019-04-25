// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/ProductOfCyclicGroups.h"

#include "MantidGeometry/Crystal/CyclicGroup.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

namespace Mantid {
namespace Geometry {

/// String constructor with semicolon-separated symmetry operations
ProductOfCyclicGroups::ProductOfCyclicGroups(const std::string &generators)
    : Group(*(getGeneratedGroup(generators))) {}

/// Constructor which directly takes a list of factor groups to form the product
ProductOfCyclicGroups::ProductOfCyclicGroups(
    const std::vector<Group_const_sptr> &factorGroups)
    : Group(*(getProductOfCyclicGroups(factorGroups))) {}

/// Generates symmetry operations from the string, creates a CyclicGroup from
/// each operation and multiplies them to form a factor group.
Group_const_sptr
ProductOfCyclicGroups::getGeneratedGroup(const std::string &generators) const {
  std::vector<SymmetryOperation> operations =
      SymmetryOperationFactory::Instance().createSymOps(generators);
  std::vector<Group_const_sptr> factorGroups = getFactorGroups(operations);

  return getProductOfCyclicGroups(factorGroups);
}

/// Returns a vector of cyclic groups for the given vector of symmetry
/// operations
std::vector<Group_const_sptr> ProductOfCyclicGroups::getFactorGroups(
    const std::vector<SymmetryOperation> &symmetryOperations) const {
  std::vector<Group_const_sptr> groups;
  groups.reserve(symmetryOperations.size());
  for (const auto &symmetryOperation : symmetryOperations) {
    groups.emplace_back(
        GroupFactory::create<CyclicGroup>(symmetryOperation.identifier()));
  }
  return groups;
}

/// Multiplies all supplied groups and returns the result
Group_const_sptr ProductOfCyclicGroups::getProductOfCyclicGroups(
    const std::vector<Group_const_sptr> &factorGroups) const {
  Group_const_sptr productGroup =
      boost::make_shared<const Group>(*(factorGroups.front()));

  for (size_t i = 1; i < factorGroups.size(); ++i) {
    productGroup = productGroup * factorGroups[i];
  }

  return productGroup;
}

} // namespace Geometry
} // namespace Mantid
