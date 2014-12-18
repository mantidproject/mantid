#include "MantidGeometry/Crystal/SpaceGroup.h"

namespace Mantid {
namespace Geometry {

/**
 * Constructor
 *
 * This constructor creates a space group with the symmetry operations contained
 * in the Group-parameter and assigns the given number and symbol.
 *
 * @param itNumber :: Space group number according to International Tables for
 *Crystallography A
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

} // namespace Geometry
} // namespace Mantid
