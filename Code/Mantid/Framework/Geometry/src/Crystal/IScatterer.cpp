#include "MantidGeometry/Crystal/IScatterer.h"

namespace Mantid
{
namespace Geometry
{

/// Default constructor, vector is wrapped to interval [0, 1).
IScatterer::IScatterer(const Kernel::V3D &position) :
    m_position(getWrappedVector(position)),
    m_cell(),
    m_spaceGroup()
{
    recalculateEquivalentPositions();
}

/// Sets the position of the scatterer to the supplied coordinates - vector is wrapped to [0, 1) and equivalent positions are recalculated.
void IScatterer::setPosition(const Kernel::V3D &position)
{
    m_position = getWrappedVector(position);

    recalculateEquivalentPositions();
}

/// Returns the position of the scatterer.
Kernel::V3D IScatterer::getPosition() const
{
    return m_position;
}

/// Returns all equivalent positions of the scatterer according to the assigned space group.
std::vector<Kernel::V3D> IScatterer::getEquivalentPositions() const
{
    return m_equivalentPositions;
}

/// Assigns a unit cell, which may be required for certain calculations.
void IScatterer::setCell(const UnitCell &cell)
{
    m_cell = cell;
}

/// Returns the cell which is currently set.
UnitCell IScatterer::getCell() const
{
    return m_cell;
}

/// Sets the space group, which is required for calculation of equivalent positions.
void IScatterer::setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup)
{
    m_spaceGroup = spaceGroup;

    recalculateEquivalentPositions();
}

SpaceGroup_const_sptr IScatterer::getSpaceGroup() const
{
    return m_spaceGroup;
}

void IScatterer::recalculateEquivalentPositions()
{
    m_equivalentPositions.clear();

    if(m_spaceGroup) {
        m_equivalentPositions = m_spaceGroup * m_position;
    } else {
        m_equivalentPositions.push_back(m_position);
    }
}



} // namespace Geometry
} // namespace Mantid
