#include "MantidGeometry/Crystal/IsotropicAtomScatterer.h"
#include "MantidKernel/Atom.h"
#include <stdexcept>

namespace Mantid
{
namespace Geometry
{

/// Constructor which takes an element symbol, fractional coordinates, isotropic atomic displacement parameter and occupancy.
IsotropicAtomScatterer::IsotropicAtomScatterer(const std::string &element,
                                               const Kernel::V3D &position,
                                               double U, double occupancy) :
    IScatterer(position)
{
    setU(U);
    setOccupancy(occupancy);
    setElement(element);
}

/// Creates a new instance with the given parameters.
IsotropicAtomScatterer_sptr IsotropicAtomScatterer::create(const std::string &element, const Kernel::V3D &position, double U, double occupancy)
{
    return boost::make_shared<IsotropicAtomScatterer>(element, position, U, occupancy);
}

/// Clones the instance.
IScatterer_sptr IsotropicAtomScatterer::clone() const
{
    IsotropicAtomScatterer_sptr clone = boost::make_shared<IsotropicAtomScatterer>(getElement(), getPosition(), getU(), getOccupancy());
    clone->setPosition(getPosition());
    clone->setCell(getCell());
    clone->setSpaceGroup(getSpaceGroup());

    return clone;
}

/// Tries to obtain element specific data for the given symbol using PhysicalConstants::getAtom.
void IsotropicAtomScatterer::setElement(const std::string &element)
{
    PhysicalConstants::Atom atom = PhysicalConstants::getAtom(element);

    m_atom = atom.neutron;
    m_label = atom.symbol;
}

/// Returns the string representation of the contained element.
std::string IsotropicAtomScatterer::getElement() const
{
    return m_label;
}

/// Returns the internally stored NeutronAtom that holds element specific data.
PhysicalConstants::NeutronAtom IsotropicAtomScatterer::getNeutronAtom() const
{
    return m_atom;
}

/// Set occupancy, which must be on the interval [0,1] - otherwise std::invalid_argument is thrown.
void IsotropicAtomScatterer::setOccupancy(double occupancy)
{
    if(occupancy < 0.0 || occupancy > 1.0) {
        throw std::invalid_argument("Allowed occupancies are on the interval [0, 1].");
    }

    m_occupancy = occupancy;
}

/// Returns the occupancy.
double IsotropicAtomScatterer::getOccupancy() const
{
    return m_occupancy;
}

/// Sets the isotropic atomic displacement parameter in Angstrom^2, values less than zero cause an std::invalid_argument expression to be thrown.
void IsotropicAtomScatterer::setU(double U)
{
    if(U < 0.0) {
        throw std::invalid_argument("Negative atomic displacement parameter is not allowed.");
    }

    m_U = U;
}

/// Returns the isotropic atomic displacement parameter.
double IsotropicAtomScatterer::getU() const
{
    return m_U;
}

/**
 * Calculates the structure factor
 *
 * This method calculates the structure factor, taking into account contributions from all
 * atoms on the stored position _and all symmetrically equivalent_.
 * For details, please refer to the class documentation in the header file.
 *
 * @param hkl :: HKL for which the structure factor should be calculated
 * @return Structure factor (complex).
 */
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

/// Returns the Debye-Waller factor, using an isotropic atomic displacement and the stored unit cell.
double IsotropicAtomScatterer::getDebyeWallerFactor(const Kernel::V3D &hkl) const
{
    Kernel::V3D dstar = getCell().getB() * hkl;

    return exp(-2.0 * M_PI * M_PI * m_U * dstar.norm2());
}

/// Returns the scattering length of the stored element.
double IsotropicAtomScatterer::getScatteringLength() const
{
    return m_atom.coh_scatt_length_real;
}



} // namespace Geometry
} // namespace Mantid
