// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Tolerance.h"

#include <set>
#include <vector>

#include <memory>

namespace Mantid {
namespace Geometry {

/// Equality-functor for comparison of atom positions with specifiable precision
class MANTID_GEOMETRY_DLL AtomPositionsEqual {
public:
  AtomPositionsEqual(double precision = 1.e-4) : m_precision(precision) {}

  bool operator()(const Kernel::V3D &lhs, const Kernel::V3D &rhs) const {
    return !(fabs(lhs.X() - rhs.X()) > m_precision || fabs(lhs.Y() - rhs.Y()) > m_precision ||
             fabs(lhs.Z() - rhs.Z()) > m_precision);
  }

private:
  double m_precision;
};

/// Less-than-functor for comparison of atom positions with specifiable
/// precision
class MANTID_GEOMETRY_DLL AtomPositionsLessThan {
public:
  AtomPositionsLessThan(double precision = 1.e-4) : m_precision(precision) {}

  bool operator()(const Kernel::V3D &lhs, const Kernel::V3D &rhs) const {
    if (fabs(lhs.X() - rhs.X()) > m_precision) {
      return lhs.X() < rhs.X();
    }

    if (fabs(lhs.Y() - rhs.Y()) > m_precision) {
      return lhs.Y() < rhs.Y();
    }

    if (fabs(lhs.Z() - rhs.Z()) > m_precision) {
      return lhs.Z() < rhs.Z();
    }

    return false;
  }

private:
  double m_precision;
};

/**
    @class Group

    The class Group represents a set of symmetry operations (or
    symmetry group). It can be constructed by providing a vector
    of the symmetry operations it consists of. Another possibility
    is using a string (see SymmetryOperationFactory for format).

    Upon construction of Group, the vector of symmetry operations
    is potentially reduced to a set of unique operations, because
    each operation may occur only once.

    The number of symmetry operations in the group determines its
    so-called order, it can be queried with the member function
    Group::order(). If one needs to process the symmetry operation
    themselves, they can be obtained by Group::getSymmetryOperations().

    A common task is to apply all symmetry operations of a group to
    a point (given in the form of a Kernel::V3D-object). The multiplication
    operator for carrying out this operation has been overloaded
    to perform this task:

        vector<V3D> coordinates = Group * V3D

    Please note that a set of unique coordinates is produced, which means
    that the number of coordinates in the vector may be smaller than the
    order of the group, depending on the input. This is because the
    components of V3D are mapped onto the interval [0, 1).

    Two groups A and B can be combined by a multiplication operation, provided
    by the corresponding overloaded operator:

        Group A, B;
        Group C = A * B

    In this operation each element of A is multiplied with each element of B
    and from the resulting list a new group is constructed. For better
    illustration, an example is provided. Group A has two symmetry operations:
    identity ("x,y,z") and inversion ("-x,-y,-z"). Group B also consists of
    two operations: identity ("x,y,z") and a rotation around the y-axis
    ("-x,y,-z"). In terms of symmetry elements, the groups are defined like so:

        A := { 1, -1 }; B := { 1, 2 [010] }

    The following table shows all multiplications that are carried out and their
    results (for multiplication of symmetry operations see SymmetryOperation)

                   |    x,y,z   |  -x,-y,-z
          -------- | ---------- | -----------
            x,y,z  |    x,y,z   |  -x,-y,-z
          -x,y,-z  |  -x,y,-z   |    x,-y,z

    The resulting group contains the three elements of A and B (1, -1, 2 [010]),
    but also one new element that is the result of multiplying "x,y,z" and
    "-x,y,-z", which is "x,-y,z" - the operation resulting from a mirror plane
    perpendicular to the y-axis. In fact, this example demonstrated how the
    combination of two crystallographic point groups (see PointGroup
    documentation and wiki) "-1" and "2" results in a new point group "2/m".

    Most of the time it's not required to use Group directly, there are several
    sub-classes that implement different behavior (CenteringGroup, CyclicGroup,
    ProductOfCyclicGroups) and are easier to handle. For construction there is a
    simple "factory function", that works for all Group-based classes which
    provide a string-based constructor:

        Group_const_sptr group = GroupFactory::create<CyclicGroup>("-x,-y,-z");

    However, the most useful sub-class is SpaceGroup, which comes with its
    own factory. For detailed information about the respective sub-classes,
    please consult their documentation.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 03/10/2014
  */
class MANTID_GEOMETRY_DLL Group {
public:
  enum CoordinateSystem { Orthogonal, Hexagonal };

  enum GroupAxiom { Closure, Identity, Inversion, Associativity };

  Group();
  Group(const std::string &symmetryOperationString);
  Group(const std::vector<SymmetryOperation> &symmetryOperations);
  virtual ~Group() = default;

  size_t order() const;
  CoordinateSystem getCoordinateSystem() const;
  const std::vector<SymmetryOperation> &getSymmetryOperations() const;
  bool containsOperation(const SymmetryOperation &operation) const;

  Group operator*(const Group &other) const;

  std::vector<Kernel::V3D> operator*(const Kernel::V3D &vector) const;

  bool isInvariant(const Kernel::DblMatrix &tensor, double tolerance = 1e-8) const;

  bool operator==(const Group &other) const;
  bool operator!=(const Group &other) const;

  bool fulfillsAxiom(GroupAxiom axiom) const;
  bool isGroup() const;

protected:
  void setSymmetryOperations(const std::vector<SymmetryOperation> &symmetryOperations);

  CoordinateSystem getCoordinateSystemFromOperations(const std::vector<SymmetryOperation> &symmetryOperations) const;

  bool isClosed() const;
  bool hasIdentity() const;
  bool eachElementHasInverse() const;
  bool associativityHolds() const;

  std::vector<SymmetryOperation> m_allOperations;
  std::set<SymmetryOperation> m_operationSet;
  CoordinateSystem m_axisSystem;
};

using Group_sptr = std::shared_ptr<Group>;
using Group_const_sptr = std::shared_ptr<const Group>;

namespace GroupFactory {
/// Creates a Group sub-class of type T if T has a constructor that takes a
/// string.
template <typename T> Group_const_sptr create(const std::string &initializationString) {
  return std::make_shared<const T>(initializationString);
}

/// Creates a Group sub-class of type T if T has a constructor that takes a
/// vector of SymmetryOperations.
template <typename T> Group_const_sptr create(const std::vector<SymmetryOperation> &symmetryOperations) {
  return std::make_shared<const T>(symmetryOperations);
}
} // namespace GroupFactory

MANTID_GEOMETRY_DLL Group_const_sptr operator*(const Group_const_sptr &lhs, const Group_const_sptr &rhs);
MANTID_GEOMETRY_DLL std::vector<Kernel::V3D> operator*(const Group_const_sptr &lhs, const Kernel::V3D &rhs);
MANTID_GEOMETRY_DLL bool operator==(const Group_const_sptr &lhs, const Group_const_sptr &rhs);
MANTID_GEOMETRY_DLL bool operator!=(const Group_const_sptr &lhs, const Group_const_sptr &rhs);

} // namespace Geometry
} // namespace Mantid
