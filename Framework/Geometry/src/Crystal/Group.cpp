#include "MantidGeometry/Crystal/Group.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

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
Group::CoordinateSystem Group::getCoordinateSystem() const {
  return m_axisSystem;
}

/// Returns a vector with all symmetry operations.
std::vector<SymmetryOperation> Group::getSymmetryOperations() const {
  return m_allOperations;
}

/// Returns true if the group contains the supplied operation.
bool Group::containsOperation(const SymmetryOperation &operation) const {
  return std::find(m_allOperations.begin(), m_allOperations.end(), operation) !=
         m_allOperations.end();
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
  std::vector<Kernel::V3D> result;

  for (auto op = m_allOperations.begin(); op != m_allOperations.end(); ++op) {
    result.push_back(Geometry::getWrappedVector((*op) * vector));
  }

  std::sort(result.begin(), result.end(), FuzzyV3DLessThan());
  result.erase(std::unique(result.begin(), result.end()), result.end());

  return result;
}

/// Returns true if both groups contain the same set of symmetry operations.
bool Group::operator==(const Group &other) const {
  return m_operationSet == other.m_operationSet;
}

/// Returns true if groups are different from eachother.
bool Group::operator!=(const Group &other) const {
  return !(this->operator==(other));
}

/// Checks whether a certain group axiom is fulfilled, can be used as a more
/// fine-grained alternative to isGroup().
bool Group::fulfillsAxiom(GroupAxiom axiom) const {
  switch (axiom) {
  case Closure:
    return isClosed();
  case Identity:
    return hasIdentity();
  case Inversion:
    return eachElementHasInverse();
  case Associativity:
    return associativityHolds();
  default:
    return false;
  }
}

/**
 * Returns whether the group fulfills the four group axioms
 *
 * When a Group is constructed from a list of symmetry operations, no checks
 * are performed whether this set of operations fulfills the four group axioms,
 * i.e. whether it is a group at all. If you are not sure whether a set of
 * symmetry operations is a group, construct one and look at the return value
 * of this method.
 *
 * @return True if group axioms are fulfilled, false otherwise.
 */
bool Group::isGroup() const {
  return isClosed() && hasIdentity() && eachElementHasInverse() &&
         associativityHolds();
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

/// Returns true if the group is closed, i.e. all elements of G * G are in G.
bool Group::isClosed() const {
  Group result = (*this) * (*this);

  // If the order is different, there are additional or fewer elements
  if (result.order() != order()) {
    return false;
  }

  // Also, all operations need to be equal.
  std::vector<SymmetryOperation> ops = result.getSymmetryOperations();
  for (size_t i = 0; i < ops.size(); ++i) {
    if (ops[i] != m_allOperations[i]) {
      return false;
    }
  }

  return true;
}

/// Returns true if the group has the identity element.
bool Group::hasIdentity() const {
  // Since the identity element does not change, this is an easy check.
  return containsOperation(SymmetryOperation());
}

/// Returns true if the inverse of each element is in the group
bool Group::eachElementHasInverse() const {
  // Iterate through all operations, check that the inverse is in the group.
  for (auto op = m_allOperations.begin(); op != m_allOperations.end(); ++op) {
    if (!containsOperation((*op).inverse())) {
      return false;
    }
  }

  return true;
}

/**
 * Checks that associativity holds, i.e. (a*b)*c == a*(b*c)
 *
 * In fact, this method returns always true, because associativity is implicitly
 * contained in the definition of the binary operator of SymmetryOperation:
 *
 *      (S1 * S2) * S3
 *      = (W1*W2, W1*w2 + w1) * S3
 *      = (W1*W2*W3, W1*W2*w3 + W1*w2 + w1)
 *
 *      S1 * (S2 * S3)
 *      = S1 * (W2*W3, W2*w3 + w2)
 *      = (W1*W2*W3, W1*W2*w3 + W1*w2 + w1)
 *
 * No matter what symmetry operations are chosen, this always holds. However,
 * for completeness the method has been added anyway.
 *
 * @return True if associativity holds
 */
bool Group::associativityHolds() const { return true; }

/// Convenience operator* for directly multiplying groups using shared
/// pointers.
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
