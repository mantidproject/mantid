#include "MantidGeometry/Crystal/IsotropicAtomScatterer.h"
#include "MantidKernel/Atom.h"

namespace Mantid
{
namespace Geometry
{

IsotropicAtomScatterer::IsotropicAtomScatterer(const std::string &element,
                                               const Kernel::V3D &position,
                                               double U, double occupancy) :
    IScatterer(position),
    m_occupancy(occupancy),
    m_U(U)
{
    setElement(element);
}

IsotropicAtomScatterer_sptr IsotropicAtomScatterer::create(const std::string &element, const Kernel::V3D &position, double U, double occupancy)
{
    return boost::make_shared<IsotropicAtomScatterer>(element, position, U, occupancy);
}

IScatterer_sptr IsotropicAtomScatterer::clone() const
{
    IsotropicAtomScatterer_sptr clone = boost::make_shared<IsotropicAtomScatterer>(getElement(), getPosition(), getU(), getOccupancy());
    clone->setPosition(getPosition());
    clone->setCell(getCell());
    clone->setSpaceGroup(getSpaceGroup());

    return clone;
}

void IsotropicAtomScatterer::setElement(const std::string &element)
{
    PhysicalConstants::Atom atom = PhysicalConstants::getAtom(element);

    m_atom = atom.neutron;
    m_label = atom.symbol;
}

std::string IsotropicAtomScatterer::getElement() const
{
    return m_label;
}

PhysicalConstants::NeutronAtom IsotropicAtomScatterer::getNeutronAtom() const
{
    return m_atom;
}

void IsotropicAtomScatterer::setOccupancy(double occupancy)
{
    if(occupancy < 0.0 || occupancy > 1.0) {
        throw std::invalid_argument("Allowed occupancies are on the interval [0, 1].");
    }

    m_occupancy = occupancy;
}

double IsotropicAtomScatterer::getOccupancy() const
{
    return m_occupancy;
}

void IsotropicAtomScatterer::setU(double U)
{
    if(U < 0.0) {
        throw std::invalid_argument("Negative atomic displacement parameter is not allowed.");
    }

    m_U = U;
}

double IsotropicAtomScatterer::getU() const
{
    return m_U;
}

StructureFactor IsotropicAtomScatterer::calculateStructureFactor(const Kernel::V3D &hkl) const
{
    double amplitude = getOccupancy() * getDebyeWallerFactor(hkl) * getScatteringLength();

    StructureFactor sum(0.0, 0.0);

    std::vector<Kernel::V3D> equivalentPositions = getEquivalentPositions();
    for(auto pos = equivalentPositions.begin(); pos != equivalentPositions.end(); ++pos) {
        double phase = 2.0 * M_PI * (*pos).scalar_prod(hkl);
        sum += amplitude * StructureFactor(cos(phase), sin(phase));
    }

    return sum;
}

double IsotropicAtomScatterer::getDebyeWallerFactor(const Kernel::V3D &hkl) const
{
    Kernel::V3D dstar = getCell().getB() * hkl;

    return exp(-2.0 * M_PI * M_PI * m_U * dstar.norm2());
}

double IsotropicAtomScatterer::getScatteringLength() const
{
    return m_atom.coh_scatt_length_real;
}



} // namespace Geometry
} // namespace Mantid
