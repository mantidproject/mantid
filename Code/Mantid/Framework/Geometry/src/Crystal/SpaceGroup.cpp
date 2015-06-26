#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"

namespace Mantid {
namespace Geometry {

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
SpaceGroup::SpaceGroup(size_t itNumber, const std::string &hmSymbol,
                       const Group &group)
    : Group(group), m_number(itNumber), m_hmSymbol(hmSymbol) {}

/// Copy constructor
SpaceGroup::SpaceGroup(const SpaceGroup &other)
    : Group(other), m_number(other.m_number), m_hmSymbol(other.m_hmSymbol) {}

/// Assignment operator, utilizes Group's assignment operator
SpaceGroup &SpaceGroup::operator=(const SpaceGroup &other) {
  Group::operator=(other);

  m_number = other.m_number;
  m_hmSymbol = other.m_hmSymbol;

  return *this;
}

/// Returns the stored space group number
size_t SpaceGroup::number() const { return m_number; }

/// Returns the stored Hermann-Mauguin symbol
std::string SpaceGroup::hmSymbol() const { return m_hmSymbol; }

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
  for (auto op = m_allOperations.begin(); op != m_allOperations.end(); ++op) {
    if ((*op).hasTranslation()) {
      /* Floating point precision problem:
       *    (H . v) % 1.0 is not always exactly 0, so instead:
       *    | [(H . v) + delta] % 1.0 | > 1e-14 is checked
       * The transformation is only performed if necessary.
       */
      if ((fabs(fmod(fabs(hkl.scalar_prod((*op).reducedVector())) + 1e-15,
                     1.0)) > 1e-14) &&
          ((*op).transformHKL(hkl) == hkl)) {
        return false;
      }
    }
  }

  return true;
}

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

  for (auto op = m_allOperations.begin(); op != m_allOperations.end(); ++op) {
    if (Geometry::getWrappedVector((*op) * wrappedPosition) ==
        wrappedPosition) {
      siteSymmetryOps.push_back(*op);
    }
  }

  return GroupFactory::create<Group>(siteSymmetryOps);
}

} // namespace Geometry
} // namespace Mantid
