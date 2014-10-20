#include "MantidGeometry/Crystal/RigidAtomScatterer.h"
#include "MantidKernel/Atom.h"

namespace Mantid
{
namespace Geometry
{

RigidAtomScatterer::RigidAtomScatterer(const std::string &element, const Kernel::V3D &position, double occupancy) :
    IScatterer(position),
    m_occupancy(occupancy)
{
    setElement(element);
}

void RigidAtomScatterer::setElement(const std::string &element)
{
    PhysicalConstants::Atom atom = PhysicalConstants::getAtom(element);

    m_atom = atom.neutron;
    m_label = atom.symbol;
}

std::string RigidAtomScatterer::getElement() const
{
    return m_label;
}

PhysicalConstants::NeutronAtom RigidAtomScatterer::getNeutronAtom() const
{
    return m_atom;
}

void RigidAtomScatterer::setOccupancy(double occupancy)
{
    if(occupancy < 0.0 || occupancy > 1.0) {
        throw std::invalid_argument("Allowed occupancies are on the interval [0, 1].");
    }

    m_occupancy = occupancy;
}

double RigidAtomScatterer::getOccupancy() const
{
    return m_occupancy;
}

StructureFactor RigidAtomScatterer::calculateStructureFactor(const Kernel::V3D &hkl) const
{
    double phase = 2.0 * M_PI * m_position.scalar_prod(hkl);

    return m_occupancy * m_atom.coh_scatt_length_real * StructureFactor(cos(phase), sin(phase));
}

} // namespace Geometry
} // namespace Mantid
