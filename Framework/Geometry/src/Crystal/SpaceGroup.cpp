// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include <algorithm>
#include <utility>

namespace Mantid::Geometry {

using namespace Kernel;

/**
 * Constructor
 *
 * This constructor creates a space group with the symmetry operations contained
 * in the Group-parameter and assigns the given number and symbol.
 *
 * @param itNumber :: Space group number (ITA)
 * @param hmSymbol :: Herman-Mauguin symbol for the space group
 * @param group :: Group that contains all symmetry operations (including
 *centering).
 */
SpaceGroup::SpaceGroup(size_t itNumber, std::string hmSymbol, const Group &group)
    : Group(group), m_number(itNumber), m_hmSymbol(std::move(hmSymbol)) {}

/// Returns the stored space group number
size_t SpaceGroup::number() const { return m_number; }

/// Returns the stored Hermann-Mauguin symbol
const std::string &SpaceGroup::hmSymbol() const { return m_hmSymbol; }

/**
 * Returns whether the given reflection is allowed or not in this space group
 *
 * Space groups that contain translational symmetry cause certain reflections
 * to be absent due to the contributions of symmetry equivalent atoms to the
 * structure factor cancelling out. This method implements the procedure
 * described in ITA [1] to check whether a reflection is allowed or not
 * according to the symmetry operations in the space group. Please note that
 * certain arrangements of atoms can lead to additional conditions that can not
 * be determined using a space group's symmetry operations alone. For these
 * situations, Geometry::CrystalStructure can help.
 *
 * [1] International Tables for Crystallography (2006). Vol. A, ch. 12.3, p. 832
 *
 * @param hkl :: HKL to be checked.
 * @return :: true if the reflection is allowed, false otherwise.
 */
bool SpaceGroup::isAllowedReflection(const Kernel::V3D &hkl) const {
  return std::none_of(m_allOperations.cbegin(), m_allOperations.cend(), [&](const auto &operation) {
    /* Floating point precision problem:
     *    (H . v) % 1.0 is not always exactly 0, so instead:
     *    | [(H . v) + delta] % 1.0 | > 1e-14 is checked
     * The transformation is only performed if necessary.
     */
    return (operation.hasTranslation()) &&
           (fabs(fmod(fabs(hkl.scalar_prod(operation.reducedVector())) + 1e-15, 1.0)) > 1e-14) &&
           (operation.transformHKL(hkl) == hkl);
  });
}

/// Convenience function for checking compatibility of a cell metric with the
/// space group, see Group::isInvariant.
bool SpaceGroup::isAllowedUnitCell(const UnitCell &cell) const { return isInvariant(cell.getG()); }

/**
 * Returns the point group of the space group
 *
 * This method uses PointGroupFactory to create the point group of the space-
 * group. Becausethe factory is used for construction, a new object is returned
 * each time this method is called.
 *
 * @return :: PointGroup-object.
 */
PointGroup_sptr SpaceGroup::getPointGroup() const {
  return PointGroupFactory::Instance().createPointGroupFromSpaceGroup(*this);
}

/**
 * Returns the site symmetry group
 *
 * The site symmetry group contains all symmetry operations of a space group
 * that leave a point unchanged. This method probes the symmetry operations
 * of the space group and constructs a Group-object using them.
 *
 * @param position :: Coordinates of the site.
 * @return :: Site symmetry group.
 */
Group_const_sptr SpaceGroup::getSiteSymmetryGroup(const V3D &position) const {
  V3D wrappedPosition = Geometry::getWrappedVector(position);

  std::vector<SymmetryOperation> siteSymmetryOps;
  AtomPositionsEqual comparator;
  std::copy_if(
      m_allOperations.begin(), m_allOperations.end(), std::inserter(siteSymmetryOps, siteSymmetryOps.begin()),
      [&](const SymmetryOperation &op) { return Geometry::getWrappedVector(op * wrappedPosition) == wrappedPosition; });

  return GroupFactory::create<Group>(siteSymmetryOps);
}

std::ostream &operator<<(std::ostream &stream, const SpaceGroup &self) {
  stream << "Space group with Hermann-Mauguin symbol: " << self.hmSymbol();
  return stream;
}

} // namespace Mantid::Geometry
