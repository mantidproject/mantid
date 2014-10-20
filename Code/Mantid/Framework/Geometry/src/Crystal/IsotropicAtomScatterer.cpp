#include "MantidGeometry/Crystal/IsotropicAtomScatterer.h"

namespace Mantid
{
namespace Geometry
{

IsotropicAtomScatterer::IsotropicAtomScatterer(const std::string &element, const Kernel::V3D &position, const UnitCell &cell, double U, double occupancy) :
    RigidAtomScatterer(element, position, occupancy),
    m_U(U),
    m_cell(cell)
{

}

void IsotropicAtomScatterer::setU(double U)
{
    m_U = U;
}

double IsotropicAtomScatterer::getU() const
{
    return m_U;
}

void IsotropicAtomScatterer::setCell(const UnitCell &cell)
{
    m_cell = cell;
}

UnitCell IsotropicAtomScatterer::getCell() const
{
    return m_cell;
}

StructureFactor IsotropicAtomScatterer::calculateStructureFactor(const Kernel::V3D &hkl) const
{
    double dStar = m_cell.dstar(hkl);

    return exp(-2.0 * M_PI * M_PI * m_U * dStar * dStar) * RigidAtomScatterer::calculateStructureFactor(hkl);
}



} // namespace Geometry
} // namespace Mantid
