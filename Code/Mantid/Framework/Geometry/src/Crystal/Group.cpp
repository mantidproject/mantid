#include "MantidGeometry/Crystal/Group.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {

/// Default constructor. Creates a group with one symmetry operation (identity).
Group::Group() : m_allOperations(), m_operationSet(), m_axisSystem() {
  std::vector<SymmetryOperation> operation(1);
  setSymmetryOperations(operation);
}

/// Uses SymmetryOperationFactory to create a vector of symmetry operations from
/// the string.
Group::Group(const std::string &symmetryOperationString)
    : m_allOperations(), m_operationSet(), m_axisSystem() {
  setSymmetryOperations(SymmetryOperationFactory::Instance().createSymOps(
      symmetryOperationString));
}

/// Constructs a group from the symmetry operations in the vector, duplicates
/// are removed.
Group::Group(const std::vector<SymmetryOperation> &symmetryOperations)
    : m_allOperations(), m_operationSet(), m_axisSystem() {
  setSymmetryOperations(symmetryOperations);
}

/// Copy constructor.
Group::Group(const Group &other)
    : m_allOperations(other.m_allOperations),
      m_operationSet(other.m_operationSet), m_axisSystem(other.m_axisSystem) {}

/// Assignment operator.
Group &Group::operator=(const Group &other) {
  m_allOperations = other.m_allOperations;
  m_operationSet = other.m_operationSet;
  m_axisSystem = other.m_axisSystem;

  return *this;
}

/// Returns the order of the group, which is the number of symmetry operations.
size_t Group::order() const { return m_allOperations.size(); }

/// Returns the axis system of the group (either orthogonal or hexagonal).
Group::CoordinateSystem Group::getCoordinateSystem() const { return m_axisSystem; }

/// Returns a vector with all symmetry operations.
std::vector<SymmetryOperation> Group::getSymmetryOperations() const {
  return m_allOperations;
}

/**
 * Multiplication operator of two groups.
 *
 * Multiplies each element of this group with each element of the other
 * group, as described in the class documentation.
 *
 * @param other :: A group.
 * @return The product resulting from the group multiplication.
 */
Group Group::operator*(const Group &other) const {
  std::vector<SymmetryOperation> result;
  result.reserve(order() * other.order());

  for (auto selfOp = m_allOperations.begin(); selfOp != m_allOperations.end();
       ++selfOp) {
    for (auto otherOp = other.m_allOperations.begin();
         otherOp != other.m_allOperations.end(); ++otherOp) {
      result.push_back((*selfOp) * (*otherOp));
    }
  }

  return Group(result);
}

/// Returns a unique set of Kernel::V3D resulting from applying all symmetry
/// operations, vectors are wrapped to [0, 1).
std::vector<Kernel::V3D> Group::operator*(const Kernel::V3D &vector) const {
  std::set<Kernel::V3D> result;

  for (auto op = m_allOperations.begin(); op != m_allOperations.end(); ++op) {
    result.insert(Geometry::getWrappedVector((*op) * vector));
  }

  return std::vector<Kernel::V3D>(result.begin(), result.end());
}

/// Returns true if both groups contain the same set of symmetry operations.
bool Group::operator==(const Group &other) const {
  return m_operationSet == other.m_operationSet;
}

/// Returns true if groups are different from eachother.
bool Group::operator!=(const Group &other) const {
  return !(this->operator==(other));
}

/// Assigns symmetry operations, throws std::invalid_argument if vector is
/// empty.
void Group::setSymmetryOperations(
    const std::vector<SymmetryOperation> &symmetryOperations) {
  if (symmetryOperations.size() < 1) {
    throw std::invalid_argument("Group needs at least one element.");
  }

  m_operationSet = std::set<SymmetryOperation>(symmetryOperations.begin(),
                                               symmetryOperations.end());
  m_allOperations = std::vector<SymmetryOperation>(m_operationSet.begin(),
                                                   m_operationSet.end());
  m_axisSystem = getCoordinateSystemFromOperations(m_allOperations);
}

/// Returns the axis system based on the given symmetry operations. Hexagonal
/// systems have 4 non-zero elements in the matrix, orthogonal have 6.
Group::CoordinateSystem Group::getCoordinateSystemFromOperations(
    const std::vector<SymmetryOperation> &symmetryOperations) const {
  for (auto op = symmetryOperations.begin(); op != symmetryOperations.end();
       ++op) {
    std::vector<int> matrix = (*op).matrix();
    if (std::count(matrix.begin(), matrix.end(), 0) == 5) {
      return Group::Hexagonal;
    }
  }

  return Group::Orthogonal;
}

/// Convenience operator* for directly multiplying groups using shared pointers.
Group_const_sptr operator*(const Group_const_sptr &lhs,
                           const Group_const_sptr &rhs) {
  if (!lhs || !rhs) {
    throw std::invalid_argument("One of the operands is null. Aborting.");
  }

  return boost::make_shared<const Group>((*lhs) * (*rhs));
}

/// Convenience operator* for getting a vector of V3D using shared pointers.
std::vector<Kernel::V3D> operator*(const Group_const_sptr &lhs,
                                   const Kernel::V3D &rhs) {
  if (!lhs) {
    throw std::invalid_argument("Cannot use null pointer for multiplication.");
  }

  return (*lhs) * rhs;
}

/// Equality operator for shared pointers.
bool operator==(const Group_const_sptr &lhs, const Group_const_sptr &rhs) {
  if (!lhs || !rhs) {
    throw std::invalid_argument("One of the operands is null. Aborting.");
  }

  return (*lhs) == (*rhs);
}

/// Inequality operator for shared pointers.
bool operator!=(const Group_const_sptr &lhs, const Group_const_sptr &rhs) {
  return !(operator==(lhs, rhs));
}

} // namespace Geometry
} // namespace Mantid
